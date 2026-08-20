#include "main.h"
#include "platform/app/libretro/online_start.h"
#include "port/utils.h"
#include "sf33rd/Source/Game/system/work_sys.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

struct _TASK task[11];

static unsigned skip_opening_calls;
static unsigned start_select_calls;

void Init_Task_SkipOpening(void) {
    skip_opening_calls++;
}

void Start_TwoPlayer_Arcade_Select(void) {
    start_select_calls++;
}

void Log_Write(LogLevel level, const char *fmt, ...) {
    (void)level;
    (void)fmt;
}

static void reset_test(void) {
    memset(task, 0, sizeof(task));
    skip_opening_calls = 0;
    start_select_calls = 0;
    LibretroOnlineStart_Reset();
}

static void test_normal_is_untouched(void) {
    reset_test();
    task[TASK_INIT].condition = 1;
    task[TASK_INIT].r_no[0] = 1;
    LibretroOnlineStart_Tick();
    assert(skip_opening_calls == 0);
    assert(start_select_calls == 0);
}

static void test_online_initializes_once(void) {
    reset_test();
    LibretroOnlineStart_SetEnabled(true);
    LibretroOnlineStart_Tick();
    assert(skip_opening_calls == 0);
    assert(start_select_calls == 0);

    task[TASK_INIT].condition = 1;
    task[TASK_INIT].r_no[0] = 1;
    for (int i = 0; i < 10; i++) {
        LibretroOnlineStart_Tick();
    }
    assert(skip_opening_calls == 1);
    assert(start_select_calls == 1);
}

static void test_reset_allows_clean_reload(void) {
    reset_test();
    LibretroOnlineStart_SetEnabled(true);
    task[TASK_INIT].condition = 1;
    task[TASK_INIT].r_no[0] = 1;
    LibretroOnlineStart_Tick();
    LibretroOnlineStart_Reset();
    LibretroOnlineStart_SetEnabled(true);
    LibretroOnlineStart_Tick();
    assert(skip_opening_calls == 2);
    assert(start_select_calls == 2);
}

int main(void) {
    test_normal_is_untouched();
    test_online_initializes_once();
    test_reset_allows_clean_reload();
    return 0;
}
