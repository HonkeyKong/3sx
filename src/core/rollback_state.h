#ifndef CORE_ROLLBACK_STATE_H
#define CORE_ROLLBACK_STATE_H

#include "platform/netplay/game_state.h"
#include "core/input_history.h"
#include "sf33rd/Source/Game/effect/effect.h"
#include <stddef.h>
#include <stdint.h>

#define ROLLBACK_STATE_MAGIC 0x31585333u
#define ROLLBACK_STATE_VERSION 3u

typedef enum RollbackStateKind {
    ROLLBACK_STATE_FULL = 2,
} RollbackStateKind;

typedef struct RollbackRuntimeState {
    uint8_t init_training_data;
    uint8_t in_game;
    int8_t language;
    uint8_t cut_analog_stick_data;
    uint8_t use_analog_stick_data;
    uint8_t reserved[3];
} RollbackRuntimeState;

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
    RollbackRuntimeState runtime;
    GameState game;
    EffectState effects;
} RollbackPayload;

typedef struct RollbackState {
    uint32_t magic;
    uint32_t version;
    uint32_t total_size;
    uint32_t kind;
    RollbackPayload payload;
} RollbackState;

void RollbackRuntimeState_Save(RollbackRuntimeState* state);
void RollbackRuntimeState_Load(const RollbackRuntimeState* state);
void RollbackState_Save(RollbackState* state);
bool RollbackState_Load(const RollbackState* state, size_t size);
RollbackStateKind RollbackState_GetKind(const RollbackState* state, size_t size);
uint32_t RollbackState_Hash(const RollbackState* state, size_t size);

#endif
