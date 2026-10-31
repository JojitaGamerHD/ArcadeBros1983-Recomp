#include "protocol.h"

static bool never_has(uint16_t address) { (void)address; return false; }
static void never_run(void) { }

/* Negative integration fixture: exported entry exists, but ABI is wrong. */
__declspec(dllexport) const ArcadeNativeModuleV1 *arcade_module_get_v1(void) {
    static const ArcadeNativeModuleV1 descriptor = {
        ARCADE_NATIVE_ABI_VERSION + 1,
        (uint32_t)sizeof(ArcadeNativeModuleV1),
        ARCADE_NATIVE_PRG_HASH, ARCADE_NATIVE_CART_HASH, never_has, never_run
    };
    return &descriptor;
}
