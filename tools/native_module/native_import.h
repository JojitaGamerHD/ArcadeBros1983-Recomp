#ifndef ARCADE_NATIVE_IMPORT_H
#define ARCADE_NATIVE_IMPORT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ARCADE_IMPORT_ERROR = -1,
    ARCADE_IMPORT_MISSING = 0,
    ARCADE_IMPORT_READY = 1
} ArcadeImportStatus;

typedef struct {
    const char *module;
    const char *rom;
    const char *config;
    const char *data;
} ArcadeImportPaths;

/* Paths use UTF-8 and remain valid until the next import call. A cache
 * lookup also returns the settings and data paths when there is no ROM. */
ArcadeImportStatus arcade_import_cached(ArcadeImportPaths *paths);
ArcadeImportStatus arcade_import_restore(void *owner, ArcadeImportPaths *paths);
ArcadeImportStatus arcade_import_select(void *owner, ArcadeImportPaths *paths);

/* Returns -1 on normal startup, or the exit code for --prepare-only. */
int arcade_import_prepare_only(void);

#ifdef __cplusplus
}
#endif

#endif
