/* Copyright (C) 2026 3SX contributors - AGPL-3.0-or-later */
#ifndef LIBRETRO_INPUT_H
#define LIBRETRO_INPUT_H

#include "core/input.h"

void LibretroInput_SetState(unsigned port, const Input_ButtonState* state);
void LibretroInput_Reset(void);
bool LibretroInput_IsGamepadConnected(int id);
void LibretroInput_GetButtonState(int id, Input_ButtonState* state);

#endif
