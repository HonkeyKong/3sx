#include "platform/app/libretro/online_start.h"

#include "main.h"
#include "port/utils.h"
#include "sf33rd/Source/Game/init3rd.h"
#include "sf33rd/Source/Game/menu/menu.h"
#include "sf33rd/Source/Game/system/work_sys.h"

static bool requested;
static bool completed;

void LibretroOnlineStart_SetEnabled(bool enabled) {
    if (requested == enabled) {
        return;
    }

    requested = enabled;
    completed = false;
}

void LibretroOnlineStart_Reset(void) {
    requested = false;
    completed = false;
}

void LibretroOnlineStart_Tick(void) {
    if (!requested || completed) {
        return;
    }

    if (task[TASK_INIT].condition != 1 || task[TASK_INIT].r_no[0] != 1) {
        return;
    }

    Init_Task_SkipOpening();
    Start_TwoPlayer_Arcade_Select();
    completed = true;
    Log_Write(LOG_LEVEL_INFO, "[ONLINE_START] initialized two-player arcade character select");
}
