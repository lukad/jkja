// What the platform layer offers the engine beyond the Sys_* functions that
// qcommon/qcommon.h has always declared (docs/design.md, section 4.5).

#ifndef SYS_PUBLIC_H
#define SYS_PUBLIC_H

// A function of the game module. The caller converts it to the function's real type.
typedef void (*sysFunction_t)(void);

// Loads the game module on first use and returns the function it exports under the given
// name. Returns NULL if the module cannot be loaded or exports no such function; the
// reason has then been printed.
sysFunction_t Sys_GetGameFunction(const char *name);

// Lets go of the game module. Functions obtained from it must not be called afterwards.
// The module itself stays loaded until Sys_UnloadRetiredGame or the next load.
void Sys_UnloadGame(void);

// Unloads a module that Sys_UnloadGame let go of. For a point where no function of the
// module can be on the stack: the top of the main loop.
void Sys_UnloadRetiredGame(void);

#endif // SYS_PUBLIC_H
