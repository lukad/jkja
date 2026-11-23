// Declarations shared by the files of the platform layer.

#ifndef SYS_LOCAL_H
#define SYS_LOCAL_H

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"

// Adds an event to the queue that Sys_GetEvent drains. A time of 0 means now. ptr is
// NULL or memory from Z_Malloc that the engine frees.
void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr);

// Input devices.
void IN_Init(void);
void IN_Shutdown(void);
void IN_Frame(void);

// Points every qgl function at a function that does nothing, for running without a
// window (docs/design.md, section 4.5).
void QGL_BindNothing(void);

#endif // SYS_LOCAL_H
