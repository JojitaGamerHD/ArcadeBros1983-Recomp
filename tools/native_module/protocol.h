#ifndef ARCADE_NATIVE_PROTOCOL_H
#define ARCADE_NATIVE_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARCADE_NATIVE_ABI_VERSION UINT32_C(1)
#define ARCADE_NATIVE_PRG_HASH UINT32_C(0x35C376D4)
#define ARCADE_NATIVE_CART_HASH UINT32_C(0x9BB21C24)

/* Original host/module contract. The fingerprints identify the supported
 * cartridge; no game instructions or ROM resources belong in this header.
 * Both sides of this prototype are C11 Windows x64 builds. */
typedef struct ArcadeNativeModuleV1 {
    uint32_t abi_version;
    uint32_t struct_size;
    uint32_t prg_hash;
    uint32_t cart_hash;
    bool (*has)(uint16_t address);
    void (*run)(void);
} ArcadeNativeModuleV1;

typedef const ArcadeNativeModuleV1 *(*ArcadeNativeGetV1)(void);

/* Host-only loader. The queried function is named arcade_module_get_v1.
 * The module remains loaded until arcade_module_close(), after guest exit.
 * The caller must not execute guest code before a successful load. Loading
 * assumes a trusted locally built module; ABI checks are not a sandbox. */
bool arcade_module_load(const char *module_path, char *error, size_t error_size);
void arcade_module_close(void);

#ifdef __cplusplus
}
#endif
#endif
