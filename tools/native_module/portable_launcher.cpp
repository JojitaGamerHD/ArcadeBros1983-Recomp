/* Original portable launcher: prepare a private native module on first import.
 * This executable contains no game instructions or extracted ROM resources. */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <objbase.h>
#include <bcrypt.h>

#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr wchar_t kTitle[] = L"Arcade Bros 1983 Recomp";
constexpr char kRomSha[] = "a2a0aa437a735cd36ed29a8e7c5b3dfa10590c6203e893f1668c7995bad2b309";
constexpr char kReceiptMarker[] = "ARCADE_BROS_IMPORT_V1";

struct Failure {
    int code;
    std::wstring message;
    fs::path log;
};
struct Cancelled {};

class Handle {
    HANDLE value_ = INVALID_HANDLE_VALUE;
public:
    explicit Handle(HANDLE value = INVALID_HANDLE_VALUE) : value_(value) {}
    ~Handle() { reset(); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    HANDLE get() const { return value_; }
    bool valid() const { return value_ && value_ != INVALID_HANDLE_VALUE; }
    void reset(HANDLE value = INVALID_HANDLE_VALUE) {
        if (valid()) CloseHandle(value_);
        value_ = value;
    }
};

std::wstring windows_error(DWORD error) {
    return L" (codigo de Windows: " + std::to_wstring(error) + L")";
}

std::string utf8(const std::wstring& text) {
    if (text.empty()) return {};
    int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        nullptr, 0, nullptr, nullptr);
    if (!length) return "No se pudo convertir el mensaje a UTF-8.";
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        result.data(), length, nullptr, nullptr);
    return result;
}

/* Windows CRT quoting, including quotes, empty strings and trailing slashes. */
std::wstring quote(const std::wstring& value) {
    std::wstring result = L"\"";
    size_t slashes = 0;
    for (wchar_t c : value) {
        if (c == L'\\') { ++slashes; continue; }
        result.append(slashes * (c == L'\"' ? 2 : 1), L'\\');
        slashes = 0;
        if (c == L'\"') result.push_back(L'\\');
        result.push_back(c);
    }
    result.append(slashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
}

fs::path executable_directory() {
    std::wstring path(32768, L'\0');
    DWORD count = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!count || count >= path.size()) throw Failure{1, L"No se pudo localizar la carpeta del juego.", {}};
    path.resize(count);
    return fs::path(path).parent_path();
}

std::wstring unique_token() {
    GUID guid{};
    if (FAILED(CoCreateGuid(&guid))) throw Failure{1, L"No se pudo reservar una preparacion nueva.", {}};
    std::wostringstream text;
    text << std::hex << std::setfill(L'0') << std::setw(8) << guid.Data1
         << std::setw(4) << guid.Data2 << std::setw(4) << guid.Data3;
    for (unsigned char byte : guid.Data4) text << std::setw(2) << static_cast<unsigned>(byte);
    return text.str();
}

bool plain_file(const fs::path& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
        !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}

bool plain_directory(const fs::path& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) &&
        !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

std::optional<std::string> read_small(const fs::path& path, size_t maximum) {
    if (!plain_file(path)) return {};
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    std::streamoff size = file.tellg();
    if (size < 0 || static_cast<uint64_t>(size) > maximum) return {};
    file.seekg(0);
    std::string result(static_cast<size_t>(size), '\0');
    if (!result.empty() && !file.read(result.data(), static_cast<std::streamsize>(result.size()))) return {};
    return result;
}

bool hex64(const std::string& value) {
    if (value.size() != 64) return false;
    for (unsigned char c : value) if (!std::isxdigit(c)) return false;
    return true;
}

std::string kit_id(const fs::path& kit) {
    auto text = read_small(kit / L"kit-id.txt", 128);
    if (!text) throw Failure{1, L"Falta la identificacion del paquete. Extrae de nuevo el ZIP completo.", {}};
    while (!text->empty() && (text->back() == '\n' || text->back() == '\r')) text->pop_back();
    if (!hex64(*text)) throw Failure{1, L"La identificacion del paquete no es valida. Extrae de nuevo el ZIP completo.", {}};
    for (char& c : *text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return *text;
}

std::string sha256(const fs::path& path) {
    Handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
    if (!file.valid()) throw Failure{1, L"No se pudo leer un archivo necesario: " + path.wstring(), {}};
    struct HashState {
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        std::vector<UCHAR> object;
        ~HashState() {
            if (hash) BCryptDestroyHash(hash);
            if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
        }
    } state;
    auto checked = [](NTSTATUS status) {
        if (status < 0) throw Failure{1, L"No se pudo verificar la integridad de los archivos.", {}};
    };
    checked(BCryptOpenAlgorithmProvider(&state.algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
    ULONG object_size = 0, copied = 0;
    checked(BCryptGetProperty(state.algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &copied, 0));
    state.object.resize(object_size);
    checked(BCryptCreateHash(state.algorithm, &state.hash, state.object.data(), object_size, nullptr, 0, 0));
    std::array<UCHAR, 65536> buffer{};
    for (;;) {
        DWORD count = 0;
        if (!ReadFile(file.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr))
            throw Failure{1, L"Fallo la lectura de un archivo necesario." + windows_error(GetLastError()), {}};
        if (!count) break;
        checked(BCryptHashData(state.hash, buffer.data(), count, 0));
    }
    std::array<UCHAR, 32> digest{};
    checked(BCryptFinishHash(state.hash, digest.data(), static_cast<ULONG>(digest.size()), 0));
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (UCHAR byte : digest) { result.push_back(digits[byte >> 4]); result.push_back(digits[byte & 15]); }
    return result;
}

bool compatible_rom(const fs::path& path) {
    try { return fs::is_regular_file(path) && sha256(path) == kRomSha; }
    catch (...) { return false; }
}

void ensure_directory(const fs::path& path) {
    std::error_code error;
    fs::create_directories(path, error);
    if (error || !fs::is_directory(path))
        throw Failure{1, L"No se pudo preparar la carpeta de datos. Coloca el juego en una carpeta con permiso de escritura.", {}};
}

void write_new(const fs::path& path, const std::string& value) {
    Handle file(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file.valid()) throw Failure{1, L"No se pudo guardar la preparacion del juego." + windows_error(GetLastError()), {}};
    size_t offset = 0;
    while (offset < value.size()) {
        DWORD written = 0;
        DWORD count = static_cast<DWORD>(value.size() - offset);
        if (!WriteFile(file.get(), value.data() + offset, count, &written, nullptr) || !written)
            throw Failure{1, L"No se pudo completar un archivo de preparacion." + windows_error(GetLastError()), {}};
        offset += written;
    }
    if (!FlushFileBuffers(file.get())) throw Failure{1, L"No se pudo completar la escritura de la preparacion.", {}};
}

const std::string& expected_manifest() {
    static const std::string text =
        "{\n  \"format\": \"arcade-native-local-v1\",\n"
        "  \"abi\": 1,\n"
        "  \"rom_sha256\": \"a2a0aa437a735cd36ed29a8e7c5b3dfa10590c6203e893f1668c7995bad2b309\",\n"
        "  \"rom\": \"game.nes\",\n"
        "  \"module\": \"ArcadeLocalModule.dll\"\n}\n";
    return text;
}

bool imported_files_valid(const fs::path& cache) {
    if (!plain_directory(cache) || !plain_file(cache / L"game.nes") ||
        !plain_file(cache / L"ArcadeLocalModule.dll")) return false;
    auto manifest = read_small(cache / L"complete.json", 4096);
    return manifest && *manifest == expected_manifest() && compatible_rom(cache / L"game.nes");
}

bool safe_leaf(const std::string& leaf, const std::string& prefix) {
    if (leaf.size() != prefix.size() + 8 || leaf.compare(0, prefix.size(), prefix) != 0) return false;
    if (leaf.find("..") != std::string::npos || leaf.find_first_of("/\\\r\n\0") != std::string::npos) return false;
    for (size_t i = prefix.size(); i < leaf.size(); ++i)
        if (!((leaf[i] >= '0' && leaf[i] <= '9') || (leaf[i] >= 'a' && leaf[i] <= 'f'))) return false;
    return true;
}

std::optional<fs::path> reusable_cache(const fs::path& cache_root, const std::string& id) {
    const std::string prefix = id.substr(0, 16) + "-";
    auto pointer = read_small(cache_root / fs::path("current-" + id.substr(0, 16) + ".txt"), 128);
    if (!pointer || pointer->empty() || pointer->back() != '\n') return {};
    pointer->pop_back();
    if (!safe_leaf(*pointer, prefix)) return {};
    fs::path cache = cache_root / fs::path(*pointer);
    try {
        if (!imported_files_valid(cache)) return {};
        auto receipt = read_small(cache / L"launcher.receipt", 256);
        if (!receipt) return {};
        const std::string start = std::string(kReceiptMarker) + "\n" + id + "\n";
        if (receipt->compare(0, start.size(), start) != 0 || receipt->size() != start.size() + 65 || receipt->back() != '\n') return {};
        const std::string recorded_hash = receipt->substr(start.size(), 64);
        if (!hex64(recorded_hash) || sha256(cache / L"ArcadeLocalModule.dll") != recorded_hash) return {};
        return cache;
    } catch (...) { return {}; }
}

fs::path pick_rom() {
    std::vector<wchar_t> path(32768, L'\0');
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFile = path.data();
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.lpstrFilter = L"ROM de NES (*.nes)\0*.nes\0Todos los archivos (*.*)\0*.*\0\0";
    dialog.lpstrTitle = L"Selecciona tu ROM de Mario Bros. (1983)";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&dialog)) return fs::path(path.data());
    DWORD code = CommDlgExtendedError();
    if (code) throw Failure{1, L"No se pudo abrir el selector de ROM." + windows_error(code), {}};
    throw Cancelled{};
}

class Progress {
    HWND window_ = nullptr, message_ = nullptr, cancel_ = nullptr;
    bool cancelled_ = false;
    static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        Progress* self = reinterpret_cast<Progress*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<Progress*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self && (message == WM_CLOSE || (message == WM_COMMAND && LOWORD(wparam) == IDCANCEL))) {
            self->cancelled_ = true;
            if (self->cancel_) EnableWindow(self->cancel_, FALSE);
            if (self->message_) SetWindowTextW(self->message_, L"Cancelando la preparacion...");
            return 0;
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }
public:
    explicit Progress(HINSTANCE instance, bool visible) {
        if (!visible) return;
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_PROGRESS_CLASS};
        if (!InitCommonControlsEx(&controls)) throw Failure{1, L"No se pudo abrir la ventana de preparacion.", {}};
        WNDCLASSW type{};
        type.lpfnWndProc = procedure;
        type.hInstance = instance;
        type.lpszClassName = L"ArcadeBrosPrepareWindowV1";
        type.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        type.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        type.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        if (!RegisterClassW(&type) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw Failure{1, L"No se pudo abrir la ventana de preparacion.", {}};
        RECT rectangle{0, 0, 480, 155};
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
        AdjustWindowRect(&rectangle, style, FALSE);
        int width = rectangle.right - rectangle.left, height = rectangle.bottom - rectangle.top;
        window_ = CreateWindowExW(0, type.lpszClassName, kTitle, style,
            (GetSystemMetrics(SM_CXSCREEN) - width) / 2, (GetSystemMetrics(SM_CYSCREEN) - height) / 2,
            width, height, nullptr, nullptr, instance, this);
        if (!window_) throw Failure{1, L"No se pudo abrir la ventana de preparacion.", {}};
        message_ = CreateWindowExW(0, L"STATIC", L"Preparando tu ROM para jugar...", WS_CHILD | WS_VISIBLE,
            24, 22, 430, 24, window_, nullptr, instance, nullptr);
        HWND bar = CreateWindowExW(0, PROGRESS_CLASSW, nullptr, WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
            24, 60, 430, 20, window_, nullptr, instance, nullptr);
        cancel_ = CreateWindowExW(0, L"BUTTON", L"Cancelar", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            354, 105, 100, 28, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDCANCEL)), instance, nullptr);
        if (!message_ || !bar || !cancel_) {
            DestroyWindow(window_); window_ = nullptr;
            throw Failure{1, L"No se pudo completar la ventana de preparacion.", {}};
        }
        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        SendMessageW(message_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(cancel_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(bar, PBM_SETMARQUEE, TRUE, 30);
        ShowWindow(window_, SW_SHOW);
        UpdateWindow(window_);
    }
    ~Progress() { if (window_ && IsWindow(window_)) DestroyWindow(window_); }
    Progress(const Progress&) = delete;
    Progress& operator=(const Progress&) = delete;
    bool cancelled() const { return cancelled_; }
    void pump() {
        MSG message;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) { cancelled_ = true; continue; }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
};

class Attributes {
    std::vector<unsigned char> storage_;
    std::array<HANDLE, 2> handles_{};
    PPROC_THREAD_ATTRIBUTE_LIST list_ = nullptr;
public:
    Attributes(HANDLE output, HANDLE input) {
        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        storage_.resize(size);
        list_ = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(storage_.data());
        if (!InitializeProcThreadAttributeList(list_, 1, 0, &size)) {
            list_ = nullptr;
            throw Failure{1, L"No se pudo iniciar la preparacion del juego.", {}};
        }
        handles_ = {output, input};
        if (!UpdateProcThreadAttribute(list_, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
            handles_.data(), sizeof(HANDLE) * handles_.size(), nullptr, nullptr)) {
            DeleteProcThreadAttributeList(list_); list_ = nullptr;
            throw Failure{1, L"No se pudo iniciar la preparacion del juego.", {}};
        }
    }
    ~Attributes() { if (list_) DeleteProcThreadAttributeList(list_); }
    PPROC_THREAD_ATTRIBUTE_LIST get() const { return list_; }
};

void run_preparer(HINSTANCE instance, bool headless, const fs::path& kit, const fs::path& rom,
    const fs::path& cache, const fs::path& data, const fs::path& log) {
    Progress progress(instance, !headless);
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
    Handle output(CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    Handle input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!output.valid() || !input.valid()) throw Failure{1, L"No se pudo crear el registro de preparacion.", log};
    Handle job(CreateJobObjectW(nullptr, nullptr));
    if (!job.valid()) throw Failure{1, L"No se pudo iniciar la preparacion del juego.", log};
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
        throw Failure{1, L"No se pudo iniciar la preparacion del juego.", log};
    Attributes attributes(output.get(), input.get());
    fs::path executable = kit / L"ArcadePrepare.exe";
    std::wstring command = quote(executable.wstring()) + L" --kit " + quote(kit.wstring()) +
        L" --rom " + quote(rom.wstring()) + L" --cache " + quote(cache.wstring());
    if (command.size() >= 32767) throw Failure{1, L"La ruta del juego es demasiado larga.", log};
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.StartupInfo.wShowWindow = SW_HIDE;
    startup.StartupInfo.hStdInput = input.get();
    startup.StartupInfo.hStdOutput = output.get();
    startup.StartupInfo.hStdError = output.get();
    startup.lpAttributeList = attributes.get();
    PROCESS_INFORMATION information{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
        CREATE_SUSPENDED | CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT,
        nullptr, data.c_str(), &startup.StartupInfo, &information))
        throw Failure{1, L"No se pudo iniciar la preparacion del juego." + windows_error(GetLastError()), log};
    Handle process(information.hProcess), thread(information.hThread);
    if (!AssignProcessToJobObject(job.get(), process.get())) {
        DWORD error = GetLastError();
        TerminateProcess(process.get(), 1); // Only the suspended child just created above.
        WaitForSingleObject(process.get(), INFINITE);
        throw Failure{1, L"No se pudo controlar la preparacion del juego." + windows_error(error), log};
    }
    if (ResumeThread(thread.get()) == static_cast<DWORD>(-1)) {
        job.reset();
        WaitForSingleObject(process.get(), INFINITE);
        throw Failure{1, L"No se pudo iniciar la preparacion del juego.", log};
    }
    for (;;) {
        DWORD wait = MsgWaitForMultipleObjects(1, &information.hProcess, FALSE, 100, QS_ALLINPUT);
        progress.pump();
        if (progress.cancelled()) {
            job.reset();
            WaitForSingleObject(process.get(), INFINITE);
            throw Cancelled{};
        }
        if (wait == WAIT_OBJECT_0) break;
        if (wait == WAIT_FAILED) throw Failure{1, L"Fallo la espera de la preparacion del juego.", log};
    }
    DWORD result = 1;
    if (!GetExitCodeProcess(process.get(), &result) || result != 0)
        throw Failure{1, L"No se pudo preparar la ROM. Comprueba que el ZIP este completo e intenta de nuevo.", log};
}

struct Options {
    bool prepare_only = false;
    fs::path rom;
    fs::path data;
};

Options options() {
    int count = 0;
    wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!arguments) throw Failure{2, L"No se pudieron leer los argumentos.", {}};
    struct LocalArguments { wchar_t** value; ~LocalArguments() { LocalFree(value); } } owned{arguments};
    Options result;
    for (int i = 1; i < count; ++i)
        if (std::wstring(arguments[i]) == L"--prepare-only") result.prepare_only = true;
    if (!result.prepare_only) return result; // Normal product mode uses its own portable data directory.
    bool rom_seen = false, data_seen = false, prepare_seen = false;
    for (int i = 1; i < count; ++i) {
        std::wstring argument = arguments[i];
        if (argument == L"--prepare-only" && !prepare_seen) { prepare_seen = true; continue; }
        if (argument == L"--rom" && !rom_seen && i + 1 < count) { result.rom = arguments[++i]; rom_seen = true; continue; }
        if (argument == L"--data" && !data_seen && i + 1 < count) { result.data = arguments[++i]; data_seen = true; continue; }
        throw Failure{2, L"Uso: --prepare-only --rom ARCHIVO_ABSOLUTO --data CARPETA_ABSOLUTA", {}};
    }
    if (!rom_seen || !data_seen || !result.rom.is_absolute() || !result.data.is_absolute())
        throw Failure{2, L"La prueba requiere --rom y --data con rutas absolutas.", {}};
    if (!compatible_rom(result.rom))
        throw Failure{2, L"La ROM no coincide con la revision compatible de Mario Bros. (1983).", {}};
    std::error_code error;
    auto status = fs::status(result.data, error);
    if ((!error && fs::exists(status) && !fs::is_directory(status)) ||
        (error && error != std::errc::no_such_file_or_directory))
        throw Failure{2, L"La carpeta de datos indicada no es valida.", {}};
    return result;
}

struct Prepared {
    fs::path cache;
    bool reused;
};

Prepared prepare(HINSTANCE instance, const Options& input, const fs::path& kit, const fs::path& data) {
    const std::string id = kit_id(kit);
    if (!plain_file(kit / L"ArcadePrepare.exe") || !plain_file(kit / L"ArcadeNativeHost.exe"))
        throw Failure{1, L"Faltan archivos del juego. Extrae el ZIP completo antes de abrirlo.", {}};
    ensure_directory(data);
    const fs::path cache_root = data / L"Cache";
    ensure_directory(cache_root);
    if (auto cached = reusable_cache(cache_root, id)) return {*cached, true};

    fs::path rom = input.rom;
    if (!input.prepare_only) {
        const fs::path stored = data / L"ROM/Mario Bros..nes";
        rom = compatible_rom(stored) ? stored : pick_rom();
        if (!compatible_rom(rom))
            throw Failure{2, L"ROM no compatible. Selecciona la revision compatible de Mario Bros. (1983).", {}};
    }
    const std::wstring token = unique_token();
    const std::string prefix = id.substr(0, 16) + "-";
    const std::wstring leaf = std::wstring(prefix.begin(), prefix.end()) + token.substr(0, 8);
    const fs::path cache = cache_root / leaf;
    if (GetFileAttributesW(cache.c_str()) != INVALID_FILE_ATTRIBUTES)
        throw Failure{1, L"No se pudo reservar una preparacion nueva. Intenta de nuevo.", {}};
    const fs::path logs = data / L"Logs";
    ensure_directory(logs);
    const fs::path log = logs / (L"prepare-" + token + L".log");
    try {
        run_preparer(instance, input.prepare_only, kit, rom, cache, data, log);
        if (kit_id(kit) != id || !imported_files_valid(cache))
            throw Failure{1, L"La preparacion no quedo completa o el paquete cambio. Intenta de nuevo.", log};
        const std::string module_hash = sha256(cache / L"ArcadeLocalModule.dll");
        write_new(cache / L"launcher.receipt", std::string(kReceiptMarker) + "\n" + id + "\n" + module_hash + "\n");
        const fs::path pointer = cache_root / fs::path("current-" + id.substr(0, 16) + ".txt");
        const fs::path temporary = cache_root / (L"current-" + std::wstring(id.begin(), id.begin() + 16) + L".tmp-" + unique_token());
        write_new(temporary, utf8(leaf) + "\n");
        if (!MoveFileExW(temporary.c_str(), pointer.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw Failure{1, L"No se pudo activar la preparacion completada." + windows_error(GetLastError()), log};
    } catch (Failure& failure) {
        if (failure.log.empty() && plain_file(log)) failure.log = log;
        throw;
    }

    // Retain a private ROM for later kit versions, without replacing any user file.
    std::error_code ignored;
    fs::path stored_directory = data / L"ROM";
    fs::create_directories(stored_directory, ignored);
    fs::path stored = stored_directory / L"Mario Bros..nes";
    if (!ignored && plain_directory(stored_directory) && GetFileAttributesW(stored.c_str()) == INVALID_FILE_ATTRIBUTES) {
        CopyFileW((cache / L"game.nes").c_str(), stored.c_str(), TRUE);
    }
    return {cache, false};
}

void launch_game(const fs::path& kit, const fs::path& data, const fs::path& cache) {
    const fs::path executable = kit / L"ArcadeNativeHost.exe";
    std::wstring command = quote(executable.wstring()) + L" " + quote((cache / L"ArcadeLocalModule.dll").wstring()) +
        L" " + quote((cache / L"game.nes").wstring()) + L" --config " + quote((data / L"config.ini").wstring());
    if (command.size() >= 32767) throw Failure{1, L"La ruta del juego es demasiado larga.", {}};
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION information{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, data.c_str(), &startup, &information))
        throw Failure{1, L"No se pudo abrir el juego." + windows_error(GetLastError()), {}};
    Handle process(information.hProcess), thread(information.hThread);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    bool headless = false;
    // Detect verification mode before parsing so even malformed test options
    // report to stderr instead of opening a product MessageBox.
    if (const wchar_t* command = GetCommandLineW())
        headless = std::wstring(command).find(L"--prepare-only") != std::wstring::npos;
    try {
        Options input = options();
        headless = input.prepare_only;
        const fs::path root = executable_directory();
        const fs::path kit = root / L"Bin";
        const fs::path data = input.prepare_only ? input.data : root / L"Datos";
        Prepared result = prepare(instance, input, kit, data);
        if (input.prepare_only) {
            std::cout << (result.reused ? "[launcher] cache reused\n" : "[launcher] preparation completed\n");
            return 0;
        }
        launch_game(kit, data, result.cache);
        return 0;
    } catch (const Cancelled&) {
        return 0;
    } catch (const Failure& failure) {
        std::wstring message = failure.message;
        if (!failure.log.empty()) message += L"\n\nRegistro de preparacion:\n" + failure.log.wstring();
        if (headless) std::cerr << utf8(message) << '\n';
        else MessageBoxW(nullptr, message.c_str(), kTitle, MB_OK | MB_ICONERROR);
        return failure.code;
    } catch (const std::exception&) {
        const std::wstring message = L"No se pudo completar la operacion. Comprueba los permisos y extrae el ZIP completo.";
        if (headless) std::cerr << utf8(message) << '\n';
        else MessageBoxW(nullptr, message.c_str(), kTitle, MB_OK | MB_ICONERROR);
        return 1;
    }
}
