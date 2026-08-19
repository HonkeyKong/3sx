#include "core/input_history.h"
#include "core/rollback_state.h"

#include <stdio.h>
#include <string.h>

u16 p1sw_0, p2sw_0, p3sw_0, p4sw_0;
u16 p1sw_1, p2sw_1, p3sw_1, p4sw_1;
u32 Interrupt_Timer;


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

    Interrupt_Timer = 1234;
    RollbackState input_only;
    RollbackState_SaveInputOnly(&input_only);
    if (RollbackState_GetKind(&input_only, sizeof(input_only)) != ROLLBACK_STATE_INPUT_ONLY ||
        input_only.payload.interrupt_timer != Interrupt_Timer ||
        memcmp(&input_only.payload.input_history, &saved, sizeof(saved)) != 0) {
        fprintf(stderr, "input-only rollback state did not capture its declared payload\n");
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
