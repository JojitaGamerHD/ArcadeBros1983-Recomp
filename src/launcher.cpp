// Arcade Bros 1983 Recomp — Lanzador portable para Windows.
// Este codigo es independiente del codigo generado de la ROM.
// La ROM queda dentro de la carpeta local Datos/ROM y no se redistribuye.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <bcrypt.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {
constexpr wchar_t kTitulo[] = L"Arcade Bros 1983 Recomp";
// SHA-256 de la ROM comprobada para generar la version v0.1.0.
constexpr char kSha256Esperado[] =
    "a2a0aa437a735cd36ed29a8e7c5b3dfa10590c6203e893f1668c7995bad2b309";

void mostrarError(const std::wstring& mensaje) {
    MessageBoxW(nullptr, mensaje.c_str(), kTitulo, MB_OK | MB_ICONERROR);
}

std::wstring errorWindows(DWORD codigo) {
    return L" (codigo de Windows: " + std::to_wstring(codigo) + L")";
}

fs::path carpetaDelLauncher() {
    std::wstring buffer(32768, L'\0');
    const DWORD largo = GetModuleFileNameW(nullptr, buffer.data(),
                                           static_cast<DWORD>(buffer.size()));
    if (largo == 0 || largo >= buffer.size()) return {};
    buffer.resize(largo);
    return fs::path(buffer).parent_path();
}

fs::path seleccionarRom() {
    std::vector<wchar_t> archivo(32768, L'\0');
    OPENFILENAMEW dialogo{};
    dialogo.lStructSize = sizeof(dialogo);
    dialogo.lpstrFile = archivo.data();
    dialogo.nMaxFile = static_cast<DWORD>(archivo.size());
    dialogo.lpstrFilter = L"ROM de NES (*.nes)\0*.nes\0Todos los archivos (*.*)\0*.*\0\0";
    dialogo.nFilterIndex = 1;
    dialogo.lpstrTitle = L"Selecciona tu ROM original de Mario Bros. (1983)";
    dialogo.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&dialogo)) return fs::path(archivo.data());
    return {};
}

// CNG (Windows) para verificar que la ROM sea exactamente la version recompilada.
std::string sha256(const fs::path& ruta) {
    std::ifstream entrada(ruta, std::ios::binary);
    if (!entrada) return {};

    BCRYPT_ALG_HANDLE algoritmo = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD longitudObjeto = 0, longitudHash = 0, recibidos = 0;
    std::string resultado;
    std::vector<UCHAR> objeto;
    std::vector<UCHAR> resumen;

    do {
        if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
                &algoritmo, BCRYPT_SHA256_ALGORITHM, nullptr, 0))) break;
        if (!BCRYPT_SUCCESS(BCryptGetProperty(
                algoritmo, BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PUCHAR>(&longitudObjeto), sizeof(longitudObjeto),
                &recibidos, 0))) break;
        if (!BCRYPT_SUCCESS(BCryptGetProperty(
                algoritmo, BCRYPT_HASH_LENGTH,
                reinterpret_cast<PUCHAR>(&longitudHash), sizeof(longitudHash),
                &recibidos, 0))) break;
        objeto.resize(longitudObjeto);
        resumen.resize(longitudHash);
        if (!BCRYPT_SUCCESS(BCryptCreateHash(algoritmo, &hash,
                objeto.data(), static_cast<ULONG>(objeto.size()), nullptr, 0, 0))) break;

        std::array<char, 8192> bloque{};
        bool correcto = true;
        while (entrada.read(bloque.data(), bloque.size()) || entrada.gcount() > 0) {
            const auto n = static_cast<ULONG>(entrada.gcount());
            if (!BCRYPT_SUCCESS(BCryptHashData(hash,
                    reinterpret_cast<PUCHAR>(bloque.data()), n, 0))) {
                correcto = false;
                break;
            }
        }
        if (entrada.bad() || !correcto) break;
        if (!BCRYPT_SUCCESS(BCryptFinishHash(
                hash, resumen.data(), static_cast<ULONG>(resumen.size()), 0))) break;

        std::ostringstream texto;
        texto << std::hex << std::setfill('0');
        for (const auto byte : resumen) texto << std::setw(2) << static_cast<int>(byte);
        resultado = texto.str();
    } while (false);

    if (hash) BCryptDestroyHash(hash);
    if (algoritmo) BCryptCloseAlgorithmProvider(algoritmo, 0);
    return resultado;
}

bool romCompatible(const fs::path& ruta) {
    return sha256(ruta) == kSha256Esperado;
}

bool ejecutarJuego(const fs::path& exe, const fs::path& rom, const fs::path& carpeta) {
    // CreateProcessW puede modificar el buffer de linea de comandos.
    std::wstring orden = L"\"" + exe.wstring() + L"\" \"" + rom.wstring() + L"\"";
    std::vector<wchar_t> mutableOrden(orden.begin(), orden.end());
    mutableOrden.push_back(L'\0');

    STARTUPINFOW inicio{};
    inicio.cb = sizeof(inicio);
    PROCESS_INFORMATION proceso{};
    if (!CreateProcessW(exe.c_str(), mutableOrden.data(), nullptr, nullptr,
                        FALSE, 0, nullptr, carpeta.c_str(), &inicio, &proceso)) {
        mostrarError(L"No se pudo iniciar el juego." + errorWindows(GetLastError()));
        return false;
    }
    CloseHandle(proceso.hThread);
    CloseHandle(proceso.hProcess);
    return true;
}
} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    const fs::path raiz = carpetaDelLauncher();
    if (raiz.empty()) {
        mostrarError(L"No se pudo determinar la carpeta del launcher.");
        return 1;
    }

    const fs::path juego = raiz / L"Bin" / L"ArcadeBros1983Recomp.exe";
    const fs::path rom = raiz / L"Datos" / L"ROM" / L"Mario Bros..nes";
    const fs::path datos = raiz / L"Datos";
    std::error_code error;

    if (!fs::exists(juego, error)) {
        mostrarError(L"No se encontro Bin\\ArcadeBros1983Recomp.exe. "
                     L"Extrae la carpeta completa del juego.");
        return 1;
    }
    fs::create_directories(rom.parent_path(), error);
    if (error) {
        mostrarError(L"No se pudo crear Datos\\ROM. "
                     L"Coloca el juego en una carpeta con permisos de escritura.");
        return 1;
    }

    // Una ROM copiada previamente solo se acepta si coincide con el juego compilado.
    if (fs::exists(rom, error) && !romCompatible(rom)) {
        mostrarError(L"La ROM almacenada no coincide con la version compatible. "
                     L"Se abrira el selector para reemplazarla.");
        fs::remove(rom, error);
        if (error) {
            mostrarError(L"No se pudo eliminar la ROM incompatible. "
                         L"Revisa los permisos de la carpeta Datos\\ROM.");
            return 1;
        }
    }

    if (!fs::exists(rom, error)) {
        const fs::path elegida = seleccionarRom();
        if (elegida.empty()) return 0; // Cancelar no es un error.
        if (!romCompatible(elegida)) {
            mostrarError(L"ROM no compatible. Debes elegir la misma version "
                         L"de Mario Bros. (1983) utilizada en esta recompilacion.");
            return 1;
        }
        fs::copy_file(elegida, rom, fs::copy_options::overwrite_existing, error);
        if (error || !romCompatible(rom)) {
            mostrarError(L"No se pudo copiar o verificar la ROM dentro de Datos\\ROM.");
            return 1;
        }
    }

    // Directorio de trabajo portable: cualquier ruta relativa queda dentro de Datos/.
    fs::create_directories(datos, error);
    if (error) {
        mostrarError(L"No se pudo preparar la carpeta Datos.");
        return 1;
    }
    return ejecutarJuego(juego, rom, datos) ? 0 : 1;
}
