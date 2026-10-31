/* Original ABI wrapper around private, locally generated translation units. */
#include "protocol.h"
#include "cyc_recomp.h"

__declspec(dllexport) const ArcadeNativeModuleV1 *arcade_module_get_v1(void) {
    static ArcadeNativeModuleV1 descriptor = {
        ARCADE_NATIVE_ABI_VERSION,
        (uint32_t)sizeof(ArcadeNativeModuleV1),
        0, 0, cyc_native_has, cyc_native_run
    };
    descriptor.prg_hash = cyc_native_prg_hash;
    descriptor.cart_hash = cyc_native_cart_hash;
    return &descriptor;
}
