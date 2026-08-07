#include "platform/app/libretro/online_start.h"
#include "main.h"
#include "sf33rd/Source/Game/engine/grade.h"
#include "sf33rd/Source/Game/engine/plcnt.h"
#include "sf33rd/Source/Game/engine/workuser.h"
#include "sf33rd/Source/Game/stage/bg.h"
#include "sf33rd/Source/Game/system/sys_sub.h"
#include "sf33rd/Source/Game/system/work_sys.h"
#include <string.h>

#define LIBRETRO_PAD_CONNECTED 2
static bool requested;
static bool completed;

static void clear_input_buffers(void) {
    p1sw_0 = p2sw_0 = p1sw_1 = p2sw_1 = 0;
    p1sw_buff = p2sw_buff = 0;
    memset(PLsw, 0, sizeof(PLsw));
    memset(plsw_00, 0, sizeof(plsw_00));
    memset(plsw_01, 0, sizeof(plsw_01));
}

void LibretroOnlineStart_SetEnabled(bool enabled) { requested = enabled; completed = false; }
void LibretroOnlineStart_Reset(void) { requested = false; completed = false; }

void LibretroOnlineStart_Tick(void) {
    if (!requested || completed || task[TASK_MENU].condition != 1) return;
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
    G_No[1] = 12;
    G_No[2] = 1;
    Mode_Type = MODE_NETWORK;
    cpExitTask(TASK_MENU);
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
}
