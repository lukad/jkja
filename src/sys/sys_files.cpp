// Directories and directory listings.

#include "sys_local.h"

#include "../qcommon/files.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace {

constexpr std::size_t kMaxFoundFiles = 0x1000;

// Copies a path into a buffer that lives as long as the process, without a trailing
// separator, which the file system code does not expect.
char *StaticPath(char *buffer, std::size_t size, const std::string &path) {
	std::string trimmed = path;
	while (trimmed.size() > 1 && (trimmed.back() == '/' || trimmed.back() == '\\')) {
		trimmed.pop_back();
	}
	Q_strncpyz(buffer, trimmed.c_str(), static_cast<int>(size));
	return buffer;
}

bool EndsWithIgnoringCase(const std::string &name, const char *ending) {
	const std::size_t length = std::strlen(ending);
	if (name.size() < length) {
		return false;
	}
	return Q_stricmp(name.c_str() + (name.size() - length), ending) == 0;
}

} // namespace

/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir(const char *path) {
	std::error_code ignored;
	std::filesystem::create_directory(path, ignored);
}

/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd(void) {
	static char cwd[MAX_OSPATH];

	std::error_code error;
	const std::filesystem::path current = std::filesystem::current_path(error);
	return StaticPath(cwd, sizeof(cwd), error ? std::string(".") : current.string());
}

/*
==============
Sys_DefaultBasePath

The directory that contains base/, unless fs_basepath on the command line says otherwise
(docs/decisions/0025-game-data-asked-for-once.md): the directory remembered from an
earlier start, else the directory of the executable as SDL names it if base/ is there,
else the one the player chooses, which is then remembered. Without a player to ask, the
directory of the executable, where the files the engine needs are then not found.
==============
*/
char *Sys_DefaultBasePath(void) {
	static char basePath[MAX_OSPATH];

	const char *base = SDL_GetBasePath();
	const std::string besideExecutable = base ? base : Sys_Cwd();

	// Given on the command line: this is only the cvar's default.
	if (Cvar_VariableString("fs_basepath")[0]) {
		return StaticPath(basePath, sizeof(basePath), besideExecutable);
	}

	const std::filesystem::path remembered =
	    std::filesystem::path(Sys_DefaultHomePath()) / "gamedata.txt";
	std::string directory;
	if (std::ifstream file(remembered, std::ios::binary); file) {
		std::getline(file, directory);
		while (!directory.empty() && (directory.back() == '\r' || directory.back() == '\n')) {
			directory.pop_back();
		}
	}
	if (Sys_HasGameData(directory)) {
		return StaticPath(basePath, sizeof(basePath), directory);
	}

	if (Sys_HasGameData(besideExecutable)) {
		return StaticPath(basePath, sizeof(basePath), besideExecutable);
	}

	directory = Sys_ChooseGameData();
	if (directory.empty()) {
		return StaticPath(basePath, sizeof(basePath), besideExecutable);
	}
	if (std::ofstream file(remembered, std::ios::binary | std::ios::trunc); file) {
		file << directory << '\n';
	}
	if (!std::filesystem::exists(remembered)) {
		Com_Printf("Could not remember the game data's directory in %s\n",
		           remembered.string().c_str());
	}
	return StaticPath(basePath, sizeof(basePath), directory);
}

/*
==============
Sys_HasGameData
==============
*/
bool Sys_HasGameData(const std::string &directory) {
	std::error_code ignored;
	return !directory.empty() &&
	       std::filesystem::is_directory(std::filesystem::path(directory) / BASEGAME, ignored);
}

/*
==============
Sys_DefaultHomePath

The per-user directory that configuration, saves and screenshots are written to, unless
fs_homepath says otherwise. The game data is never written to
(docs/decisions/0025-game-data-asked-for-once.md).
==============
*/
char *Sys_DefaultHomePath(void) {
	static char homePath[MAX_OSPATH];

	char *directory = SDL_GetPrefPath(NULL, "jasp");
	if (!directory) {
		return Sys_Cwd();
	}
	StaticPath(homePath, sizeof(homePath), directory);
	SDL_free(directory);
	return homePath;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

// Lists the names in a directory that end with the extension, compared without regard
// to case as the original platform did. With wantsubs, or with "/" as the extension,
// directories are listed instead of files. The result ends with a NULL entry and is
// freed with Sys_FreeFileList; it is NULL when nothing was found.
char **Sys_ListFiles(const char *directory, const char *extension, int *numfiles,
                     qboolean wantsubs) {
	if (!extension) {
		extension = "";
	}

	// passing a slash as extension will find directories
	bool wantDirectories = wantsubs != qfalse;
	if (extension[0] == '/' && extension[1] == 0) {
		extension = "";
		wantDirectories = true;
	}
	// ".*" stood for "anything" in the pattern the original passed to the system
	if (std::strcmp(extension, ".*") == 0) {
		extension = "";
	}

	std::vector<std::string> names;
	std::error_code error;
	for (std::filesystem::directory_iterator entry(directory, error), end; !error && entry != end;
	     entry.increment(error)) {
		std::error_code statusError;
		if (entry->is_directory(statusError) != wantDirectories) {
			continue;
		}
		const std::string name = entry->path().filename().string();
		if (!EndsWithIgnoringCase(name, extension)) {
			continue;
		}
		if (names.size() == kMaxFoundFiles - 1) {
			break;
		}
		names.push_back(name);
	}

	*numfiles = static_cast<int>(names.size());
	if (names.empty()) {
		return NULL;
	}

	char **list = static_cast<char **>(
	    Z_Malloc(static_cast<int>((names.size() + 1) * sizeof(char *)), TAG_LISTFILES, qfalse));
	for (std::size_t i = 0; i < names.size(); i++) {
		list[i] = CopyString(names[i].c_str());
	}
	list[names.size()] = NULL;
	return list;
}

void Sys_FreeFileList(char **filelist) {
	if (!filelist) {
		return;
	}
	for (int i = 0; filelist[i]; i++) {
		Z_Free(filelist[i]);
	}
	Z_Free(filelist);
}
