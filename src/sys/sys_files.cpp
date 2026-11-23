// Directories and directory listings.

#include "sys_local.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <filesystem>
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

The directory that contains base/, unless fs_basepath says otherwise: the directory of
the executable (docs/decisions/0007-game-data-location.md).
==============
*/
char *Sys_DefaultBasePath(void) {
	static char basePath[MAX_OSPATH];

	const char *directory = SDL_GetBasePath();
	if (!directory) {
		return Sys_Cwd();
	}
	return StaticPath(basePath, sizeof(basePath), directory);
}

/*
==============
Sys_DefaultHomePath

The per-user directory that configuration, saves and screenshots are written to, unless
fs_homepath says otherwise. The game data is never written to
(docs/decisions/0007-game-data-location.md).
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
