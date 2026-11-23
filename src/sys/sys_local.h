// Declarations shared by the files of the platform layer.

#ifndef SYS_LOCAL_H
#define SYS_LOCAL_H

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"

// Adds an event to the queue that Sys_GetEvent drains. A time of 0 means now. ptr is
// NULL or memory from Z_Malloc that the engine frees.
void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr);

// True when the engine runs without a window, input devices and a sound device: the
// cvar sys_headless, which is set on the command line.
bool Sys_Headless(void);

// The window, or NULL while there is none.
struct SDL_Window;
SDL_Window *Sys_Window(void);

// Input devices.
void IN_Init(void);
void IN_Shutdown(void);
void IN_Frame(void);

// Turns what the window system reports (keys, text, mouse, focus, close) into engine
// events and commands. Does nothing without a window.
void Sys_PumpEvents(void);

// The gamepad, for the functions above: its cvars and commands, the events of SDL that
// are its own (returns whether the event was one), and its sticks once a frame.
union SDL_Event;
void Gamepad_Init(void);
void Gamepad_Shutdown(void);
bool Gamepad_Event(const SDL_Event &event);
void Gamepad_Frame(void);

// Lines typed on standard input, as console events.
void Sys_StartConsoleInput(void);
void Sys_QueueConsoleInput(void);

// Points every qgl function at the function of the current OpenGL context. Returns
// false, having printed their names, if functions are missing.
bool QGL_Bind(void);

// Points every qgl function at a function that does nothing, for running without a
// window (docs/design.md, section 4.5).
void QGL_BindNothing(void);

// The window when there is none: fills in glConfig and binds the qgl functions to nothing.
void NullWindow_Init(void);

// The sound device when there is none.
qboolean NullSound_Init(void);
int NullSound_GetDMAPos(void);
void NullSound_Shutdown(void);

#endif // SYS_LOCAL_H
