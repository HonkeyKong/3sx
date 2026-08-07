#ifndef PORT_PATHS_H
#define PORT_PATHS_H

/// Get app directory path
///
/// This value shouldn't be freed after use
const char* Paths_GetPrefPath();

const char* Paths_GetBasePath();
void Paths_SetOverrides(const char* pref_path, const char* base_path);
void Paths_ResetOverrides();

#endif
