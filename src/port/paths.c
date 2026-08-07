#include "port/paths.h"
#include <stddef.h>

#if !CRS_APP_DRIVER_LIBRETRO
#include <SDL3/SDL.h>
#endif

static const char* pref_path = NULL;
static const char* base_path = NULL;

void Paths_SetOverrides(const char* pref, const char* base) {
    pref_path = pref;
    base_path = base;
}

void Paths_ResetOverrides() {
    pref_path = NULL;
    base_path = NULL;
}

const char* Paths_GetPrefPath() {
    if (pref_path == NULL) {
#if CRS_APP_DRIVER_LIBRETRO
        return "";
#else
        pref_path = SDL_GetPrefPath("CrowdedStreet", "3SX");
#endif
    }

    return pref_path;
}

const char* Paths_GetBasePath() {
    if (base_path != NULL) return base_path;
#if CRS_APP_DRIVER_LIBRETRO
    return "";
#else
    return SDL_GetBasePath();
#endif
}
