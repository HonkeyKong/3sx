#ifndef NETPLAY_BASE_H
#define NETPLAY_BASE_H

#if NETPLAY_ENABLED

#include "core/rollback_state.h"
#include "types.h"

#define PLAYER_COUNT 2

typedef struct State { GameState gs; EffectState es; } State;

#if DEBUG
#define STATE_BUFFER_MAX 20
#endif

void NetplayBase_SetupVsMode();
void NetplayBase_StartStressSession();

#if DEBUG
void NetplayBase_DumpDesyncPair(int frame);
#endif

#endif // NETPLAY_ENABLED

#endif
