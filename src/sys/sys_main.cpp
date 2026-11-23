// Process-level platform services: entry point, time, events, console output.

#include "sys_local.h"
#include "sys_public.h"

#include <SDL3/SDL.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

constexpr int kMaxQueuedEvents = 256;
constexpr int kQueuedEventsMask = kMaxQueuedEvents - 1;

sysEvent_t eventQueue[kMaxQueuedEvents];
int eventHead = 0;
int eventTail = 0;

// Total memory at or below which the engine saves memory at the cost of loading time,
// in megabytes.
constexpr int kLowMemoryThreshold = 128;

// Ends the process without running exit handlers. The thread that reads standard input
// (sys_console.cpp) is usually blocked in a read and holds the lock of the stream while
// it is; exit() would wait for that lock, that is, for the next line to be typed.
[[noreturn]] void EndProcess(int status) {
	std::fflush(stdout);
	std::fflush(stderr);
	std::_Exit(status);
}

} // namespace

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds(void) {
	return static_cast<int>(SDL_GetTicks());
}

/*
================
Sys_Print

The system console is standard output.
================
*/
void Sys_Print(const char *msg) {
	std::fputs(msg, stdout);
}

/*
================
Sys_ShowConsole

There is no console window to show or hide.
================
*/
void Sys_ShowConsole(int, qboolean) {}

/*
================
Sys_Error

Reports an error the engine cannot recover from and ends the process.
================
*/
void QDECL Sys_Error(const char *error, ...) {
	char text[4096];

	va_list arguments;
	va_start(arguments, error);
	std::vsnprintf(text, sizeof(text), error, arguments);
	va_end(arguments);

	std::fflush(stdout);
	std::fprintf(stderr, "Error: %s\n", text);

	Sys_UnloadGame();
	SDL_Quit();
	EndProcess(1);
}

/*
================
Sys_Quit
================
*/
void Sys_Quit(void) {
	IN_Shutdown();
	Sys_UnloadGame();
	SDL_Quit();
	EndProcess(0);
}

/*
================
Sys_Headless
================
*/
bool Sys_Headless(void) {
	static cvar_t *sys_headless = Cvar_Get("sys_headless", "0", CVAR_INIT);

	return sys_headless->integer != 0;
}

/*
================
Sys_GetClipboardData

Returns the first line of the text on the clipboard, in memory the caller frees, or NULL.
The clipboard belongs to the window system; without a window there is none.
================
*/
char *Sys_GetClipboardData(void) {
	if (!Sys_Window() || !SDL_HasClipboardText()) {
		return NULL;
	}

	char *text = SDL_GetClipboardText();
	if (!text) {
		return NULL;
	}

	// The clipboard holds UTF-8 and the engine's characters are single bytes: what does
	// not fit is left out, so the result is never longer than the original.
	char *data = static_cast<char *>(
	    Z_Malloc(static_cast<int>(std::strlen(text)) + 1, TAG_CLIPBOARD, qfalse));
	char *out = data;
	const char *cursor = text;
	while (Uint32 codepoint = SDL_StepUTF8(&cursor, NULL)) {
		if (codepoint == '\n' || codepoint == '\r' || codepoint == '\b') {
			break;
		}
		if (codepoint < 0x100) {
			*out++ = static_cast<char>(codepoint);
		}
	}
	*out = 0;

	SDL_free(text);
	return data;
}

/*
================
Sys_LowPhysicalMemory
================
*/
qboolean Sys_LowPhysicalMemory() {
	static cvar_t *sys_lowmem = Cvar_Get("sys_lowmem", "0", 0);

	if (sys_lowmem->integer) {
		return qtrue;
	}
	return SDL_GetSystemRAM() <= kLowMemoryThreshold ? qtrue : qfalse;
}

/*
================
Sys_QueEvent
================
*/
void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr) {
	sysEvent_t *ev = &eventQueue[eventHead & kQueuedEventsMask];

	if (eventHead - eventTail >= kMaxQueuedEvents) {
		Com_Printf("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if (ev->evPtr) {
			Z_Free(ev->evPtr);
		}
		eventTail++;
	}
	eventHead++;

	if (time == 0) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent(void) {
	// see what the window system and standard input have for the queue
	Sys_PumpEvents();
	Sys_QueueConsoleInput();

	if (eventHead > eventTail) {
		eventTail++;
		return eventQueue[(eventTail - 1) & kQueuedEventsMask];
	}

	// create an empty event to return
	sysEvent_t ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.evTime = Sys_Milliseconds();
	return ev;
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f(void) {
	IN_Shutdown();
	IN_Init();
}

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
void Sys_Init(void) {
	Cmd_AddCommand("in_restart", Sys_In_Restart_f);

	Cvar_Set("arch", SDL_GetPlatform());

	// The engine has no processor-specific code left.
	Cvar_Get("sys_cpustring", "generic", CVAR_ROM);
	Cvar_Get("sys_cpuid", va("%i", CPUID_GENERIC), CVAR_ROM);
	Com_Printf("%i logical processors, %i MB of memory\n", SDL_GetNumLogicalCPUCores(),
	           SDL_GetSystemRAM());

	IN_Init();
}

/*
==================
main
==================
*/
int main(int argc, char **argv) {
	// The engine takes its command line as one string and splits it at spaces again, so
	// an argument that contains one is quoted.
	std::string commandLine;
	for (int i = 1; i < argc; i++) {
		const std::string argument = argv[i];
		if (i > 1) {
			commandLine += ' ';
		}
		if (argument.empty() || argument.find(' ') != std::string::npos) {
			commandLine += '"' + argument + '"';
		} else {
			commandLine += argument;
		}
	}

	// The console is standard output; show each line when it is printed.
	std::setvbuf(stdout, NULL, _IOLBF, 0);

	// get the initial time base
	Sys_Milliseconds();

	Com_Init(commandLine.data());

	Sys_StartConsoleInput();

	// main game loop
	while (true) {
		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();
	}

	// never gets here
}
