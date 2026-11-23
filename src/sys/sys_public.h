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

// Unloads the game module. Functions obtained from it must not be called afterwards.
void Sys_UnloadGame(void);

#endif // SYS_PUBLIC_H
