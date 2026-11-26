// Declarations shared by the files of the platform layer.

#ifndef SYS_LOCAL_H
#define SYS_LOCAL_H

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"

#include <string>

// Adds an event to the queue that Sys_GetEvent drains. A time of 0 means now. ptr is
// NULL or memory from Z_Malloc that the engine frees.
void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr);

// True when the engine runs without a window, input devices and a sound device: the
// cvar sys_headless, which is set on the command line.
bool Sys_Headless(void);

// The directory of the executable file, with a separator at the end; empty if it is not
// known. The game module lies there.
std::string Sys_ExecutableDirectory(void);

// True if the directory contains base/, the game data.
bool Sys_HasGameData(const std::string &directory);

// Asks the player, with the platform's folder dialog, for the directory that contains
// base/ (docs/decisions/0025-game-data-asked-for-once.md). Returns the directory, or an
// empty string if there is no player to ask or no dialog to ask with. Ends the engine if
// the player cancels.
std::string Sys_ChooseGameData(void);

// The window, or NULL while there is none.
struct SDL_Window;
SDL_Window *Sys_Window(void);

// Keeps vid_xpos and vid_ypos up to date with where the window system has put the window.
void Sys_WindowMoved(int x, int y);

// The title of the window, and of the window that shows a fatal error.
inline constexpr char sysWindowTitle[] = "Jedi Academy";

// Input devices.
void IN_Init(void);
void IN_Shutdown(void);
void IN_Frame(void);

// True while the window is minimised or covered, as IN_Frame saw it last.
bool Sys_WindowIsHidden(void);

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

// While the game is not active the sound device plays silence; the mixer goes on as
// before, so nothing is behind when the game is active again.
void SNDDMA_Activate(bool active);

// The sound device when there is none.
qboolean NullSound_Init(void);
int NullSound_GetDMAPos(void);
void NullSound_Shutdown(void);

#endif // SYS_LOCAL_H
