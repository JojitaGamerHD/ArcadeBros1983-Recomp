#include "protocol.h"
#include <stdio.h>

int arcade_host_main(int argc, char **argv);

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <local-game.dll> <rom.nes> [headless host options]\n", argv[0]);
        return 2;
    }
    char error[512];
    if (!arcade_module_load(argv[1], error, sizeof(error))) {
        fprintf(stderr, "[arcade module] %s\n", error);
        return 2;
    }
    fputs("[arcade module] ABI v1 accepted; using locally compiled game code.\n", stderr);
    for (int i = 1; i + 1 < argc; ++i) argv[i] = argv[i + 1];
    argv[argc - 1] = NULL;
    int result = arcade_host_main(argc - 1, argv);
    arcade_module_close();
    return result;
}
