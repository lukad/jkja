// The sound device when there is none: the mixer is told so and stays off.

#include "sys_local.h"

#include "../client/snd_local.h"

qboolean SNDDMA_Init(void) {
	return qfalse;
}

int SNDDMA_GetDMAPos(void) {
	return 0;
}

void SNDDMA_Shutdown(void) {}

void SNDDMA_BeginPainting(void) {}

void SNDDMA_Submit(void) {}
