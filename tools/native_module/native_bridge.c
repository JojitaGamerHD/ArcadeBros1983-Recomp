/* Original generic-host bridge. Game instructions are loaded from a local
 * module, never linked into this executable. The hash constants are only
 * cartridge identity checks for the supported revision. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "protocol.h"
#include "cyc_recomp.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

const char *cyc_native_display_name = "Arcade Bros 1983 Recomp";
const char *cyc_native_program_name = "game";
const uint32_t cyc_native_prg_hash = ARCADE_NATIVE_PRG_HASH;
const uint32_t cyc_native_cart_hash = ARCADE_NATIVE_CART_HASH;
const uint32_t cyc_native_fds_bios_crc32 = 0;
const char *cyc_native_fds_bios_path = NULL;
const char *cyc_native_fds_image_path = NULL;
const char *cyc_native_fds_hle = NULL;
const uint8_t cyc_native_console = 0;
const CycRamView *const cyc_native_ram_views[1] = { NULL };
const uint32_t cyc_native_ram_view_count = 0;
const CycHookSite cyc_native_hook_sites[1] = { { NULL, 0, 0, 0 } };
const uint32_t cyc_native_hook_site_count = 0;

static HMODULE loaded_module;
static ArcadeNativeModuleV1 active;

static bool report(char *error, size_t capacity, const char *format, ...) {
    if (error && capacity) {
        va_list args;
        va_start(args, format);
        vsnprintf(error, capacity, format, args);
        va_end(args);
        error[capacity - 1] = '\0';
    }
    return false;
}

static wchar_t *absolute_module_path(const char *path, DWORD *failure) {
    UINT code_page = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int length = MultiByteToWideChar(code_page, flags, path, -1, NULL, 0);
    if (!length) {
        code_page = CP_ACP;
        flags = 0;
        length = MultiByteToWideChar(code_page, flags, path, -1, NULL, 0);
    }
    if (!length) { *failure = GetLastError(); return NULL; }
    wchar_t *wide = (wchar_t *)calloc((size_t)length, sizeof(wchar_t));
    if (!wide) { *failure = ERROR_NOT_ENOUGH_MEMORY; return NULL; }
    if (!MultiByteToWideChar(code_page, flags, path, -1, wide, length)) {
        *failure = GetLastError(); free(wide); return NULL;
    }
    DWORD needed = GetFullPathNameW(wide, 0, NULL, NULL);
    if (!needed || needed > 32767) {
        *failure = needed ? ERROR_FILENAME_EXCED_RANGE : GetLastError();
        free(wide); return NULL;
    }
    wchar_t *absolute = (wchar_t *)calloc((size_t)needed + 1, sizeof(wchar_t));
    if (!absolute) { *failure = ERROR_NOT_ENOUGH_MEMORY; free(wide); return NULL; }
    DWORD written = GetFullPathNameW(wide, needed + 1, absolute, NULL);
    free(wide);
    if (!written || written > needed) {
        *failure = written ? ERROR_INSUFFICIENT_BUFFER : GetLastError();
        free(absolute); return NULL;
    }
    return absolute;
}

bool arcade_module_load(const char *module_path, char *error, size_t capacity) {
    if (error && capacity) error[0] = '\0';
    if (loaded_module) return report(error, capacity, "A native module is already loaded.");
    if (!module_path || !module_path[0]) return report(error, capacity, "No native module path was supplied.");
    DWORD failure = 0;
    wchar_t *path = absolute_module_path(module_path, &failure);
    if (!path) return report(error, capacity, "Cannot resolve native module path (Win32 %lu).", (unsigned long)failure);
    DWORD attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        failure = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : ERROR_DIRECTORY;
        free(path);
        return report(error, capacity, "Native module is not an accessible file (Win32 %lu).", (unsigned long)failure);
    }
    HMODULE candidate = LoadLibraryExW(path, NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    failure = candidate ? 0 : GetLastError();
    free(path);
    if (!candidate) return report(error, capacity, "Cannot load native module (Win32 %lu).", (unsigned long)failure);
    ArcadeNativeGetV1 query = (ArcadeNativeGetV1)GetProcAddress(candidate, "arcade_module_get_v1");
    if (!query) {
        FreeLibrary(candidate);
        return report(error, capacity, "Native module does not export arcade_module_get_v1.");
    }
    const ArcadeNativeModuleV1 *descriptor = query();
    if (!descriptor) {
        FreeLibrary(candidate);
        return report(error, capacity, "Native module returned no ABI descriptor.");
    }
    if (descriptor->abi_version != ARCADE_NATIVE_ABI_VERSION ||
        descriptor->struct_size != sizeof(ArcadeNativeModuleV1)) {
        const uint32_t version = descriptor->abi_version;
        const uint32_t size = descriptor->struct_size;
        FreeLibrary(candidate);
        return report(error, capacity, "Native module ABI mismatch (version %u, size %u).", (unsigned)version, (unsigned)size);
    }
    if (descriptor->prg_hash != ARCADE_NATIVE_PRG_HASH ||
        descriptor->cart_hash != ARCADE_NATIVE_CART_HASH) {
        FreeLibrary(candidate);
        return report(error, capacity, "Native module does not match the supported cartridge.");
    }
    if (!descriptor->has || !descriptor->run) {
        FreeLibrary(candidate);
        return report(error, capacity, "Native module has incomplete execution callbacks.");
    }
    active = *descriptor;
    loaded_module = candidate;
    return true;
}

void arcade_module_close(void) {
    HMODULE previous = loaded_module;
    loaded_module = NULL;
    active = (ArcadeNativeModuleV1){0};
    if (previous) FreeLibrary(previous);
}

static void require_module(void) {
    if (!loaded_module) {
        fputs("Native execution requested without a loaded game module.\n", stderr);
        exit(2);
    }
}

bool cyc_native_has(uint16_t address) {
    require_module();
    return active.has(address);
}

void cyc_native_run(void) {
    require_module();
    active.run();
}
