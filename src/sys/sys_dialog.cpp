// The dialog that asks the player where the game data is
// (docs/decisions/0025-game-data-asked-for-once.md).

#include "sys_local.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <string>

namespace {

// What the folder dialog's callback reports. The callback can run on another thread.
struct FolderChoice {
	std::atomic<bool> answered{false};
	bool chosen = false;
	std::string directory;
	std::string error;
};

void SDLCALL FolderChosen(void *userdata, const char *const *filelist, int) {
	FolderChoice &choice = *static_cast<FolderChoice *>(userdata);
	if (!filelist) {
		choice.error = SDL_GetError();
	} else if (filelist[0]) {
		choice.chosen = true;
		choice.directory = filelist[0];
	}
	choice.answered.store(true, std::memory_order_release);
}

// Shows the folder dialog and waits for the answer. The dialog's work is done in SDL's
// event loop on some platforms, so the loop runs until the callback has. The answer is
// written to a choice that has not been answered yet.
void AskForFolder(FolderChoice &choice) {
	const SDL_PropertiesID properties = SDL_CreateProperties();
	SDL_SetStringProperty(properties, SDL_PROP_FILE_DIALOG_TITLE_STRING,
	                      "Choose the folder of Jedi Academy's game data");
	SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFOLDER, FolderChosen, &choice,
	                                 properties);
	SDL_DestroyProperties(properties);

	while (!choice.answered.load(std::memory_order_acquire)) {
		SDL_PumpEvents();
		SDL_Delay(10);
	}
}

void ShowMessage(SDL_MessageBoxFlags flags, const std::string &text) {
	SDL_ShowSimpleMessageBox(flags, sysWindowTitle, text.c_str(), nullptr);
}

} // namespace

std::string Sys_ChooseGameData(void) {
	if (Sys_Headless()) {
		return std::string();
	}
	// The dialogs need the video system; without it there is no player to ask.
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		Com_Printf("No dialog for the game data's folder: %s\n", SDL_GetError());
		return std::string();
	}

	ShowMessage(SDL_MESSAGEBOX_INFORMATION,
	            "Jedi Academy's game data was not found.\n\n"
	            "Choose the folder of the installed game that contains the folder \"base\". "
	            "In a Steam install it is called \"GameData\". The choice is remembered.");

	std::string directory;
	while (true) {
		FolderChoice choice;
		AskForFolder(choice);
		if (!choice.error.empty()) {
			Com_Printf("The dialog for the game data's folder failed: %s\n",
			           choice.error.c_str());
		}
		if (!choice.chosen) {
			break;
		}
		if (Sys_HasGameData(choice.directory)) {
			directory = choice.directory;
			break;
		}
		ShowMessage(SDL_MESSAGEBOX_WARNING,
		            "There is no folder \"base\" in\n" + choice.directory +
		                "\n\nChoose the folder that contains it.");
	}

	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	if (directory.empty()) {
		Sys_Error("The game cannot start without its data.\n\n"
		          "Start it again and choose the folder that contains \"base\", or name "
		          "that folder on the command line: +set fs_basepath <folder>");
	}
	return directory;
}
