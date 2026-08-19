#include "core/input_history.h"
#include "sf33rd/Source/Game/system/work_sys.h"

void InputHistory_Save(InputHistoryState* state) {
    state->current[0] = p1sw_0;
    state->current[1] = p2sw_0;
    state->current[2] = p3sw_0;
    state->current[3] = p4sw_0;
    state->previous[0] = p1sw_1;
    state->previous[1] = p2sw_1;
    state->previous[2] = p3sw_1;
    state->previous[3] = p4sw_1;
}

void InputHistory_Load(const InputHistoryState* state) {
    p1sw_0 = state->current[0];
    p2sw_0 = state->current[1];
    p3sw_0 = state->current[2];
    p4sw_0 = state->current[3];
    p1sw_1 = state->previous[0];
    p2sw_1 = state->previous[1];
    p3sw_1 = state->previous[2];
    p4sw_1 = state->previous[3];
}

void InputHistory_Advance(const u16 inputs[4]) {
    p1sw_1 = p1sw_0;
    p2sw_1 = p2sw_0;
    p3sw_1 = p3sw_0;
    p4sw_1 = p4sw_0;
    p1sw_0 = inputs[0];
    p2sw_0 = inputs[1];
    p3sw_0 = inputs[2];
    p4sw_0 = inputs[3];
}
