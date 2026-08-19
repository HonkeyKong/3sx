#include "platform/app/libretro/online_start.h"
#include "main.h"
#include "sf33rd/Source/Game/engine/grade.h"
#include "sf33rd/Source/Game/engine/plcnt.h"
#include "sf33rd/Source/Game/engine/workuser.h"
#include "sf33rd/Source/Game/init3rd.h"
#include "sf33rd/Source/Game/stage/bg.h"
#include "sf33rd/Source/Game/system/sys_sub.h"
#include "sf33rd/Source/Game/system/work_sys.h"
#include "port/utils.h"
#include <string.h>

#define LIBRETRO_PAD_CONNECTED 2
static bool requested;
static bool completed;
static bool opening_skipped;
static bool character_select_reached;

static void clear_input_buffers(void) {
    p1sw_0 = p2sw_0 = p1sw_1 = p2sw_1 = 0;
    p1sw_buff = p2sw_buff = 0;
    memset(PLsw, 0, sizeof(PLsw));
    memset(plsw_00, 0, sizeof(plsw_00));
    memset(plsw_01, 0, sizeof(plsw_01));
}

void LibretroOnlineStart_SetEnabled(bool enabled) {
    if (requested == enabled) return;
    requested = enabled;
    completed = false;
    opening_skipped = false;
    character_select_reached = false;
}
void LibretroOnlineStart_Reset(void) {
    requested = false;
    completed = false;
    opening_skipped = false;
    character_select_reached = false;
}

static void enter_character_select_transition(void) {
    G_No[0] = 2;
    G_No[1] = 12;
    G_No[2] = 1;
    G_No[3] = 0;
    cpExitTask(TASK_INIT);
    cpExitTask(TASK_MENU);
    cpExitTask(TASK_ENTRY);
}

void LibretroOnlineStart_Tick(void) {
    if (!requested) return;

    if (completed) {
        if (!character_select_reached && G_No[0] == 2 && G_No[1] == 1) {
            if (G_No[2] >= 1) {
                character_select_reached = true;
                Log_Write(LOG_LEVEL_INFO, "[ONLINE_START] character select reached");
            }
            return;
        }

        if (!character_select_reached && G_No[0] == 2 && G_No[1] == 12) {
            return;
        }

        if (!character_select_reached) {
            Log_Write(LOG_LEVEL_WARN, "[ONLINE_START] boot state overwrote transition (%d/%d/%d/%d); restoring it", G_No[0], G_No[1], G_No[2], G_No[3]);
            enter_character_select_transition();
        }
        return;
    }

    if (!opening_skipped && task[TASK_INIT].condition == 1 && task[TASK_INIT].r_no[0] == 1) {
        Log_Write(LOG_LEVEL_INFO, "[ONLINE_START] skipping opening at initialized boot state");
        Init_Task_SkipOpening();
        opening_skipped = true;
        return;
    }

    if (opening_skipped) {
        if (task[TASK_INIT].condition != 0 || task[TASK_GAME].condition != 1) {
            return;
        }
    } else if (task[TASK_MENU].condition != 1) {
        return;
    } else {
        Log_Write(LOG_LEVEL_WARN, "[ONLINE_START] late activation after normal menu initialization");
    }

    task[TASK_MENU].r_no[0] = 5;
    cpExitTask(TASK_SAVER);
    Interface_Type[0] = Interface_Type[1] = LIBRETRO_PAD_CONNECTED;
    plw[0].wu.operator = plw[1].wu.operator = 1;
    Operator_Status[0] = Operator_Status[1] = 1;
    grade_check_work_1st_init(0, 0);
    grade_check_work_1st_init(0, 1);
    grade_check_work_1st_init(1, 0);
    grade_check_work_1st_init(1, 1);
    Setup_Training_Difficulty();
    // Keep the normal cabinet rules while entering with both players active.
    // Arcadia continues to own transport and rollback.
    Mode_Type = MODE_ARCADE;
    enter_character_select_transition();
    E_Timer = 0;
    memset(E_No, 0, sizeof(E_No));
    Next_Demo = 0;
    G_Timer = 0;
    memset(bg_w.bgw, 0, sizeof(bg_w.bgw));
    bg_w.scno = bg_w.scrno = 0;
    Deley_Shot_No[0] = Deley_Shot_No[1] = 0;
    Deley_Shot_Timer[0] = Deley_Shot_Timer[1] = 15;
    Random_ix16 = Random_ix32 = 0;
    Clear_Flash_Init(4);
    clear_input_buffers();
    completed = true;
    Log_Write(LOG_LEVEL_INFO, "[ONLINE_START] entered two-player character-select transition");
}
