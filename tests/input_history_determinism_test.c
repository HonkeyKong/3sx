#include "core/input_history.h"
#include "core/rollback_state.h"

#include <stdio.h>
#include <string.h>

u16 p1sw_0, p2sw_0, p3sw_0, p4sw_0;
u16 p1sw_1, p2sw_1, p3sw_1, p4sw_1;
u32 Interrupt_Timer;
MPP mpp_w;


enum { REPLAY_FRAMES = 240, RESTORE_CYCLES = 5 };

static u32 next_random(u32* state) {
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static u32 state_hash(void) {
    InputHistoryState state;
    InputHistory_Save(&state);
    const u8* bytes = (const u8*)&state;
    u32 hash = 2166136261u;
    for (size_t i = 0; i < sizeof(state); ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

int main(void) {
    u16 inputs[REPLAY_FRAMES][4];
    u32 expected[REPLAY_FRAMES];
    u32 rng = 0x33585242u;
    InputHistoryState saved;

    const u16 initial[4] = { 0x0010, 0x0200, 0x0004, 0x0400 };
    InputHistory_Advance(initial);
    InputHistory_Save(&saved);

    RollbackRuntimeState runtime;
    mpp_w.initTrainingData = true;
    mpp_w.inGame = false;
    mpp_w.language = 1;
    mpp_w.cutAnalogStickData = true;
    mpp_w.useAnalogStickData = false;
    memset(&runtime, 0, sizeof(runtime));
    RollbackRuntimeState_Save(&runtime);

    memset(&mpp_w, 0, sizeof(mpp_w));
    mpp_w.inGame = true;
    RollbackRuntimeState_Load(&runtime);
    if (!mpp_w.initTrainingData || mpp_w.inGame || mpp_w.language != 1 ||
        !mpp_w.cutAnalogStickData || mpp_w.useAnalogStickData) {
        fprintf(stderr, "rollback runtime state did not restore the saved phase\n");
        return 1;
    }

    RollbackState envelope;
    memset(&envelope, 0, sizeof(envelope));
    envelope.magic = ROLLBACK_STATE_MAGIC;
    envelope.version = ROLLBACK_STATE_VERSION;
    envelope.total_size = sizeof(envelope);
    envelope.kind = ROLLBACK_STATE_FULL;
    if (RollbackState_GetKind(&envelope, sizeof(envelope)) != ROLLBACK_STATE_FULL) {
        fprintf(stderr, "full rollback state envelope was rejected\n");
        return 1;
    }
    envelope.version--;
    if (RollbackState_GetKind(&envelope, sizeof(envelope)) != 0) {
        fprintf(stderr, "stale rollback state version was accepted\n");
        return 1;
    }

    for (int frame = 0; frame < REPLAY_FRAMES; ++frame) {
        for (int player = 0; player < 4; ++player) {
            inputs[frame][player] = (u16)next_random(&rng);
        }
        InputHistory_Advance(inputs[frame]);
        expected[frame] = state_hash();
    }

    for (int cycle = 0; cycle < RESTORE_CYCLES; ++cycle) {
        InputHistory_Load(&saved);
        for (int frame = 0; frame < REPLAY_FRAMES; ++frame) {
            InputHistory_Advance(inputs[frame]);
            const u32 actual = state_hash();
            if (actual != expected[frame]) {
                fprintf(stderr, "input history diverged: cycle=%d frame=%d expected=%08x actual=%08x\n",
                        cycle, frame, expected[frame], actual);
                return 1;
            }
        }
    }

    printf("input history replay deterministic: %d frames x %d restore cycles; rollback state size: %zu bytes\n",
           REPLAY_FRAMES, RESTORE_CYCLES, sizeof(RollbackState));
    return 0;
}
