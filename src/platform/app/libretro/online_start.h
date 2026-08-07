#ifndef LIBRETRO_ONLINE_START_H
#define LIBRETRO_ONLINE_START_H
#include <stdbool.h>
void LibretroOnlineStart_SetEnabled(bool enabled);
void LibretroOnlineStart_Reset(void);
void LibretroOnlineStart_Tick(void);
#endif
