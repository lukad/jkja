// Loading of the game module (docs/decisions/0010-project-and-binary-names.md,
// docs/decisions/0013-typed-cgame-interface.md).

#include "sys_local.h"
#include "sys_public.h"

#include <SDL3/SDL.h>

#include <string>

namespace {

SDL_SharedObject *gameModule = NULL;

// The module lies next to the executable. Its file name comes from the build, because
// it contains the architecture: jagamex86_64.so, jagamearm64.dylib, jagamex86_64.dll.
std::string GameModulePath() {
	const char *directory = SDL_GetBasePath();
	return std::string(directory ? directory : "") + JASP_GAME_MODULE_FILE;
}

} // namespace

sysFunction_t Sys_GetGameFunction(const char *name) {
	if (!gameModule) {
		const std::string path = GameModulePath();
		gameModule = SDL_LoadObject(path.c_str());
		if (!gameModule) {
			Com_Printf("Could not load the game module %s: %s\n", path.c_str(), SDL_GetError());
			return NULL;
		}
	}

	const SDL_FunctionPointer function = SDL_LoadFunction(gameModule, name);
	if (!function) {
		Com_Printf("The game module has no function %s: %s\n", name, SDL_GetError());
	}
	return function;
}

void Sys_UnloadGame(void) {
	if (gameModule) {
		SDL_UnloadObject(gameModule);
		gameModule = NULL;
	}
}
