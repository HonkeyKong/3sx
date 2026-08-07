/* Copyright (C) 2026 3SX contributors - AGPL-3.0-or-later */
#ifndef LIBRETRO_RENDERER_H
#define LIBRETRO_RENDERER_H
#include "core/render_primitives.h"
#include <stdbool.h>
#include <stdint.h>
bool LibretroRenderer_Init(void);
void LibretroRenderer_Quit(void);
void LibretroRenderer_Present(void);
bool LibretroRenderer_ContextReset(void *(*get_proc_address)(const char *), uintptr_t (*get_framebuffer)(void));
bool LibretroRenderer_ContextReady(void);
void LibretroRenderer_CreateTexture(unsigned int h);
void LibretroRenderer_DestroyTexture(unsigned int h);
void LibretroRenderer_CreatePalette(unsigned int h);
void LibretroRenderer_DestroyPalette(unsigned int h);
void LibretroRenderer_SetTexture(unsigned int h);
void LibretroRenderer_DrawTexturedQuad(const Sprite* s, unsigned int c);
void LibretroRenderer_DrawSprite(const Sprite* s, unsigned int c);
void LibretroRenderer_DrawSprite2(const Sprite2* s);
void LibretroRenderer_DrawSolidQuad(const Quad* q, unsigned int c);
#endif
