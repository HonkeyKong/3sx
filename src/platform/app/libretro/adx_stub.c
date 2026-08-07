#include "port/sound/adx.h"

void ADX_ProcessTracks(void) {}
void ADX_Init(void) {}
void ADX_Exit(void) {}
void ADX_Stop(void) {}
int ADX_IsPaused(void) { return 1; }
void ADX_Pause(int pause) { (void)pause; }
void ADX_StartSeamless(void) {}
void ADX_StartMem(void* buf, size_t size) { (void)buf; (void)size; }
int ADX_GetNumFiles(void) { return 0; }
void ADX_EntryAfs(int file_id) { (void)file_id; }
void ADX_StartAfs(int file_id) { (void)file_id; }
void ADX_ResetEntry(void) {}
void ADX_SetOutVol(int volume) { (void)volume; }
void ADX_SetMono(bool mono) { (void)mono; }
ADXState ADX_GetState(void) { return ADX_STATE_STOP; }
