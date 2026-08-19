#ifndef CORE_INPUT_HISTORY_H
#define CORE_INPUT_HISTORY_H

#include "types.h"

typedef struct InputHistoryState {
    u16 current[4];
    u16 previous[4];
} InputHistoryState;

void InputHistory_Save(InputHistoryState* state);
void InputHistory_Load(const InputHistoryState* state);
void InputHistory_Advance(const u16 inputs[4]);

#endif
