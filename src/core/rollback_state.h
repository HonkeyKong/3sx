#ifndef CORE_ROLLBACK_STATE_H
#define CORE_ROLLBACK_STATE_H

#include "platform/netplay/game_state.h"
#include "core/input_history.h"
#include "sf33rd/Source/Game/effect/effect.h"
#include <stddef.h>
#include <stdint.h>

#define ROLLBACK_STATE_MAGIC 0x31585333u
#define ROLLBACK_STATE_VERSION 2u

typedef enum RollbackStateKind {
    ROLLBACK_STATE_INPUT_ONLY = 1,
    ROLLBACK_STATE_FULL = 2,
} RollbackStateKind;

typedef struct EffectState {
    s16 frwctr;
    s16 frwctr_min;
    s16 head_ix[8];
    s16 tail_ix[8];
    s16 exec_tm[8];
    uintptr_t frw[EFFECT_MAX][448];
    s16 frwque[EFFECT_MAX];
} EffectState;

typedef struct RollbackPayload {
    uint32_t interrupt_timer;
    InputHistoryState input_history;
    GameState game;
    EffectState effects;
} RollbackPayload;

typedef struct RollbackState {
    uint32_t magic;
    uint32_t version;
    uint32_t total_size;
    uint32_t reserved;
    RollbackPayload payload;
} RollbackState;

void RollbackState_Save(RollbackState* state);
void RollbackState_SaveInputOnly(RollbackState* state);
bool RollbackState_Load(const RollbackState* state, size_t size);
RollbackStateKind RollbackState_GetKind(const RollbackState* state, size_t size);
uint32_t RollbackState_Hash(const RollbackState* state, size_t size);

#endif
