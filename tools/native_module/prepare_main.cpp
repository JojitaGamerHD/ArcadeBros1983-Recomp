/* Original developer prototype: prepare a private native module from a local ROM. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
static constexpr const char* kRomSha256 =
    "a2a0aa437a735cd36ed29a8e7c5b3dfa10590c6203e893f1668c7995bad2b309";

class Failure : public std::runtime_error {
public:
    int code;
    Failure(int value, const std::string& text) : std::runtime_error(text), code(value) {}
};

static std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        nullptr, 0, nullptr, nullptr);
    if (!size) throw Failure(1, "No se pudo convertir una ruta a UTF-8.");
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        result.data(), size, nullptr, nullptr);
    return result;
}

static std::wstring from_utf8(const std::string& value) {
    if (value.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0);
    if (!size) throw Failure(1, "La herramienta devolvio una ruta que no es UTF-8.");
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), result.data(), size);
    return result;
}

static std::string win_error(DWORD error) {
    return "Windows error " + std::to_string(error);
}

class Handle {
    HANDLE value_;
public:
    explicit Handle(HANDLE value = INVALID_HANDLE_VALUE) : value_(value) {}
    ~Handle() { if (value_ && value_ != INVALID_HANDLE_VALUE) CloseHandle(value_); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    HANDLE get() const { return value_; }
    bool valid() const { return value_ && value_ != INVALID_HANDLE_VALUE; }
};

static std::string sha256(const fs::path& path) {
    Handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
    if (!file.valid()) throw Failure(2, "No se pudo leer: " + utf8(path.wstring()));
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
        if (status < 0) throw Failure(1, "Fallo el calculo SHA256 de Windows.");
    };
    checked(BCryptOpenAlgorithmProvider(&state.algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
    ULONG object_size = 0, copied = 0;
    checked(BCryptGetProperty(state.algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &copied, 0));
    state.object.resize(object_size);
    checked(BCryptCreateHash(state.algorithm, &state.hash, state.object.data(), object_size,
        nullptr, 0, 0));
    std::array<UCHAR, 65536> buffer{};
    for (;;) {
        DWORD count = 0;
        if (!ReadFile(file.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr))
            throw Failure(2, "Fallo la lectura de la ROM: " + win_error(GetLastError()));
        if (!count) break;
        checked(BCryptHashData(state.hash, buffer.data(), count, 0));
    }
    std::array<UCHAR, 32> digest{};
    checked(BCryptFinishHash(state.hash, digest.data(), static_cast<ULONG>(digest.size()), 0));
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (UCHAR byte : digest) {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 15]);
    }
    return result;
}

/* Quote one CRT argv token, including empty tokens and trailing backslashes. */
static std::wstring quote(const std::wstring& value) {
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

static void run(const fs::path& program, const std::vector<std::wstring>& arguments,
    const fs::path& cwd, const fs::path& log) {
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
    Handle output(CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    Handle input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!output.valid() || !input.valid())
        throw Failure(1, "No se pudo crear el registro: " + utf8(log.wstring()));
    std::wstring command = quote(program.wstring());
    for (const auto& argument : arguments) command += L" " + quote(argument);
    if (command.size() >= 32767) throw Failure(1, "La linea de comandos es demasiado larga.");
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdInput = input.get();
    startup.hStdOutput = output.get();
    startup.hStdError = output.get();
    PROCESS_INFORMATION information{};
    if (!CreateProcessW(program.c_str(), command.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, cwd.c_str(), &startup, &information)) {
        throw Failure(1, "No se pudo iniciar " + utf8(program.filename().wstring()) +
            ": " + win_error(GetLastError()));
    }
    Handle process(information.hProcess), thread(information.hThread);
    if (WaitForSingleObject(process.get(), INFINITE) != WAIT_OBJECT_0)
        throw Failure(1, "Fallo la espera de la herramienta: " + utf8(log.wstring()));
    DWORD code = 0;
    if (!GetExitCodeProcess(process.get(), &code))
        throw Failure(1, "No se pudo consultar el resultado de la herramienta.");
    if (code != 0) throw Failure(1, utf8(program.filename().wstring()) +
        " termino con codigo " + std::to_string(code) + ". Registro: " + utf8(log.wstring()));
}

static void require_file(const fs::path& path) {
    if (!fs::is_regular_file(path))
        throw Failure(2, "Falta un archivo del kit: " + utf8(path.wstring()));
}

static std::wstring resource_directory(const fs::path& log) {
    std::ifstream file(log, std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.pop_back();
    if (text.empty() || text.find_first_of("\r\n\0") != std::string::npos)
        throw Failure(1, "Clang devolvio una ruta de recursos inesperada. Registro: " + utf8(log.wstring()));
    fs::path path(from_utf8(text));
    if (!path.is_absolute()) throw Failure(1, "La ruta de recursos de Clang no es absoluta.");
    require_file(path / L"include" / L"stdint.h");
    return (path / L"include").wstring();
}

static void manifest(const fs::path& path) {
    static constexpr char content[] =
        "{\n  \"format\": \"arcade-native-local-v1\",\n"
        "  \"abi\": 1,\n"
        "  \"rom_sha256\": \"a2a0aa437a735cd36ed29a8e7c5b3dfa10590c6203e893f1668c7995bad2b309\",\n"
        "  \"rom\": \"game.nes\",\n"
        "  \"module\": \"ArcadeLocalModule.dll\"\n}\n";
    Handle file(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file.valid()) throw Failure(1, "No se pudo crear el manifiesto local.");
    DWORD written = 0;
    if (!WriteFile(file.get(), content, sizeof(content) - 1, &written, nullptr) ||
        written != sizeof(content) - 1)
        throw Failure(1, "No se pudo completar el manifiesto local.");
}

static void prepare(const fs::path& kit, const fs::path& rom, const fs::path& cache) {
    const fs::path clang = kit / L"compiler/bin/clang.exe";
    const fs::path linker = kit / L"compiler/bin/lld-link.exe";
    for (const auto& relative : {L"ArcadeNativeHost.exe", L"ArcadeNativeHost.lib",
        L"NESRecomp.exe", L"game.toml", L"module_entry.c", L"protocol.h",
        L"sdk/runner/cyc/cyc_recomp.h"}) require_file(kit / relative);
    require_file(clang);
    require_file(linker);
    if (!fs::is_directory(kit / L"sdk/runner/include"))
        throw Failure(2, "Faltan las cabeceras genericas del kit.");
    if (!fs::is_regular_file(rom)) throw Failure(2, "No se encontro la ROM indicada.");
    if (fs::exists(cache) || fs::is_symlink(fs::symlink_status(cache)))
        throw Failure(2, "La carpeta cache ya existe. Elige una carpeta nueva.");
    if (!fs::is_directory(cache.parent_path()))
        throw Failure(2, "La carpeta padre de la cache no existe.");
    if (sha256(rom) != kRomSha256)
        throw Failure(2, "La ROM no coincide con la revision compatible. No se genero un modulo.");
    if (!fs::create_directory(cache)) throw Failure(2, "No se pudo reservar una cache nueva.");
    fs::create_directory(cache / L"objects");
    fs::create_directory(cache / L"logs");
    const fs::path local_rom = cache / L"game.nes";
    fs::copy_file(rom, local_rom, fs::copy_options::none);
    if (sha256(local_rom) != kRomSha256)
        throw Failure(2, "La ROM cambio mientras se copiaba. No se genero un modulo.");

    std::cout << "1/3 Verificando los recursos del compilador portatil...\n" << std::flush;
    const fs::path resources_log = cache / L"logs/resource.log";
    run(clang, {L"--no-default-config", L"-print-resource-dir"}, cache, resources_log);
    const std::wstring includes = resource_directory(resources_log);

    std::cout << "2/3 Generando el codigo privado directamente desde la ROM...\n" << std::flush;
    run(kit / L"NESRecomp.exe", {local_rom.wstring(), L"--game", (kit / L"game.toml").wstring(),
        L"--cycle-accurate", L"--output-prefix", L"game"}, cache, cache / L"logs/codegen.log");
    const std::vector<std::wstring> expected{L"game_cyc.c", L"game_cyc_b00.c",
        L"game_cyc_b01.c", L"game_cyc_b02.c", L"game_cyc_b03.c"};
    std::set<std::wstring> actual;
    for (const auto& entry : fs::directory_iterator(cache / L"generated")) {
        const auto name = entry.path().filename().wstring();
        if (entry.is_regular_file() && name.rfind(L"game_cyc", 0) == 0 && entry.path().extension() == L".c")
            actual.insert(name);
    }
    if (actual != std::set<std::wstring>(expected.begin(), expected.end()))
        throw Failure(1, "El recompilador produjo una lista de fuentes distinta a la revision soportada.");
    std::vector<fs::path> sources;
    for (const auto& name : expected) sources.push_back(cache / L"generated" / name);
    sources.push_back(kit / L"module_entry.c");

    std::cout << "3/3 Compilando y enlazando el modulo local...\n" << std::flush;
    std::vector<std::wstring> objects;
    for (size_t i = 0; i < sources.size(); ++i) {
        fs::path object = cache / L"objects" / (L"part-" + std::to_wstring(i) + L".obj");
        run(clang, {L"--no-default-config", L"--target=x86_64-pc-windows-msvc", L"-std=c11",
            L"-O2", L"-ffreestanding", L"-fno-stack-protector", L"-nostdinc",
            L"-isystem", includes, L"-I", (kit / L"sdk/runner/cyc").wstring(),
            L"-I", (kit / L"sdk/runner/include").wstring(), L"-I", kit.wstring(),
            L"-c", sources[i].wstring(), L"-o", object.wstring()}, cache,
            cache / L"logs" / (L"compile-" + std::to_wstring(i) + L".log"));
        objects.push_back(object.wstring());
    }
    const fs::path module = cache / L"ArcadeLocalModule.dll";
    std::vector<std::wstring> link{L"/DLL", L"/NOENTRY", L"/NODEFAULTLIB", L"/MACHINE:X64",
        L"/OUT:" + module.wstring()};
    link.insert(link.end(), objects.begin(), objects.end());
    link.push_back((kit / L"ArcadeNativeHost.lib").wstring());
    run(linker, link, cache, cache / L"logs/link.log");
    require_file(module);
    manifest(cache / L"complete.json");
    std::cout << "MODULO LOCAL PREPARADO\n" << utf8(module.wstring()) << '\n';
}

int wmain(int argc, wchar_t** argv) {
    try {
        if (argc == 2 && std::wstring(argv[1]) == L"--version") {
            std::cout << "arcade-native-prepare 1\n";
            return 0;
        }
        fs::path kit, rom, cache;
        for (int i = 1; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (i + 1 >= argc) throw Failure(2, "Falta un valor en la linea de comandos.");
            fs::path* destination = option == L"--kit" ? &kit : option == L"--rom" ? &rom :
                option == L"--cache" ? &cache : nullptr;
            if (!destination || !destination->empty()) throw Failure(2, "Opcion desconocida o repetida.");
            *destination = fs::absolute(argv[++i]).lexically_normal();
        }
        if (kit.empty() || rom.empty() || cache.empty())
            throw Failure(2, "Uso: ArcadePrepare.exe --kit <carpeta> --rom <archivo> --cache <carpeta-nueva>");
        prepare(kit, rom, cache);
        return 0;
    } catch (const Failure& error) {
        std::cerr << "Preparacion fallida: " << error.what() << '\n';
        return error.code;
    } catch (const std::exception& error) {
        std::cerr << "Preparacion fallida: " << error.what() << '\n';
        return 1;
    }
}
