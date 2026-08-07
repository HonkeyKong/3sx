/* Copyright (C) 2026 3SX contributors - AGPL-3.0-or-later */
#include "platform/input/libretro/libretro_input.h"
#include <string.h>

static Input_ButtonState states[2];

void LibretroInput_SetState(unsigned port, const Input_ButtonState* state) {
    if (port < 2) states[port] = *state;
}

void LibretroInput_Reset(void) { memset(states, 0, sizeof(states)); }
bool LibretroInput_IsGamepadConnected(int id) { return id >= 0 && id < 2; }

void LibretroInput_GetButtonState(int id, Input_ButtonState* state) {
    if (id >= 0 && id < 2) *state = states[id];
    else memset(state, 0, sizeof(*state));
}
