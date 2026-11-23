// Keyboard and mouse: what the window system reports becomes the engine's key, character
// and mouse events.

#include "sys_local.h"

#include "../client/client.h"
#include "../server/server.h"

#include <SDL3/SDL.h>

namespace {

// False while the window does not have the keyboard.
bool windowHasFocus = true;

// True while the mouse belongs to the game: the pointer is hidden and held, and the
// mouse reports motion instead of a position.
bool mouseActive;

// The key that opens the console types a character as well, which must not reach the
// console's input line. Text is dropped while that key is down.
bool consoleKeyIsDown;

// Mouse motion since the last mouse event that was queued.
float motionX;
float motionY;

cvar_t *in_mouse;

/*
===============
KeyFromEvent

The engine's number for a key. Keys that type a character have the number of the
character on the unshifted key, with letters in upper case; that is what the Windows
version produced.
===============
*/
int KeyFromEvent(const SDL_KeyboardEvent &event) {
	switch (event.scancode) {
	// The console key is the key in that position, whatever the layout prints on it.
	case SDL_SCANCODE_GRAVE:
		return A_CONSOLE;

	case SDL_SCANCODE_LSHIFT:
	case SDL_SCANCODE_RSHIFT:
		return A_SHIFT;
	case SDL_SCANCODE_LCTRL:
	case SDL_SCANCODE_RCTRL:
		return A_CTRL;
	case SDL_SCANCODE_LALT:
	case SDL_SCANCODE_RALT:
		return A_ALT;
	case SDL_SCANCODE_CAPSLOCK:
		return A_CAPSLOCK;
	case SDL_SCANCODE_NUMLOCKCLEAR:
		return A_NUMLOCK;
	case SDL_SCANCODE_SCROLLLOCK:
		return A_SCROLLLOCK;
	case SDL_SCANCODE_PAUSE:
		return A_PAUSE;
	case SDL_SCANCODE_PRINTSCREEN:
		return A_PRINTSCREEN;

	case SDL_SCANCODE_BACKSPACE:
		return A_BACKSPACE;
	case SDL_SCANCODE_TAB:
		return A_TAB;
	case SDL_SCANCODE_RETURN:
		return A_ENTER;
	case SDL_SCANCODE_ESCAPE:
		return A_ESCAPE;
	case SDL_SCANCODE_SPACE:
		return A_SPACE;

	case SDL_SCANCODE_INSERT:
		return A_INSERT;
	case SDL_SCANCODE_DELETE:
		return A_DELETE;
	case SDL_SCANCODE_HOME:
		return A_HOME;
	case SDL_SCANCODE_END:
		return A_END;
	case SDL_SCANCODE_PAGEUP:
		return A_PAGE_UP;
	case SDL_SCANCODE_PAGEDOWN:
		return A_PAGE_DOWN;
	case SDL_SCANCODE_UP:
		return A_CURSOR_UP;
	case SDL_SCANCODE_DOWN:
		return A_CURSOR_DOWN;
	case SDL_SCANCODE_LEFT:
		return A_CURSOR_LEFT;
	case SDL_SCANCODE_RIGHT:
		return A_CURSOR_RIGHT;

	case SDL_SCANCODE_F1:
		return A_F1;
	case SDL_SCANCODE_F2:
		return A_F2;
	case SDL_SCANCODE_F3:
		return A_F3;
	case SDL_SCANCODE_F4:
		return A_F4;
	case SDL_SCANCODE_F5:
		return A_F5;
	case SDL_SCANCODE_F6:
		return A_F6;
	case SDL_SCANCODE_F7:
		return A_F7;
	case SDL_SCANCODE_F8:
		return A_F8;
	case SDL_SCANCODE_F9:
		return A_F9;
	case SDL_SCANCODE_F10:
		return A_F10;
	case SDL_SCANCODE_F11:
		return A_F11;
	case SDL_SCANCODE_F12:
		return A_F12;

	case SDL_SCANCODE_KP_0:
		return A_KP_0;
	case SDL_SCANCODE_KP_1:
		return A_KP_1;
	case SDL_SCANCODE_KP_2:
		return A_KP_2;
	case SDL_SCANCODE_KP_3:
		return A_KP_3;
	case SDL_SCANCODE_KP_4:
		return A_KP_4;
	case SDL_SCANCODE_KP_5:
		return A_KP_5;
	case SDL_SCANCODE_KP_6:
		return A_KP_6;
	case SDL_SCANCODE_KP_7:
		return A_KP_7;
	case SDL_SCANCODE_KP_8:
		return A_KP_8;
	case SDL_SCANCODE_KP_9:
		return A_KP_9;
	case SDL_SCANCODE_KP_PERIOD:
		return A_KP_PERIOD;
	case SDL_SCANCODE_KP_ENTER:
		return A_KP_ENTER;
	case SDL_SCANCODE_KP_PLUS:
		return A_KP_PLUS;
	case SDL_SCANCODE_KP_MINUS:
		return A_KP_MINUS;
	case SDL_SCANCODE_KP_MULTIPLY:
		return A_MULTIPLY;
	case SDL_SCANCODE_KP_DIVIDE:
		return A_DIVIDE;

	default:
		break;
	}

	// A key that types a character of Latin-1, which is the range of the key numbers.
	const SDL_Keycode character = event.key;
	if (character >= 'a' && character <= 'z') {
		return static_cast<int>(character) - 'a' + 'A';
	}
	if (character >= 0xe0 && character <= 0xfe && character != 0xf7) {
		// the accented letters; 0xf7 is the division sign
		return static_cast<int>(character) - 0x20;
	}
	if (character > ' ' && character < 0x100) {
		return static_cast<int>(character);
	}
	return 0;
}

/*
===============
ToggleFullscreen

Alt and Enter, as in the Windows version: not while the game is in a cinematic or
behind a menu.
===============
*/
void ToggleFullscreen() {
	const bool allowed = !ge || (ge->GameAllowedToSaveHere() && !(cls.keyCatchers & KEYCATCH_UI));
	if (allowed) {
		Cvar_SetValue("r_fullscreen", Cvar_VariableIntegerValue("r_fullscreen") ? 0.0f : 1.0f);
		Cbuf_AddText("vid_restart\n");
	}
}

void KeyEvent(const SDL_KeyboardEvent &event) {
	if (event.scancode == SDL_SCANCODE_GRAVE) {
		consoleKeyIsDown = event.down;
	}

	if (event.down && !event.repeat && event.scancode == SDL_SCANCODE_RETURN &&
	    (event.mod & SDL_KMOD_ALT)) {
		ToggleFullscreen();
		return;
	}

	const int key = KeyFromEvent(event);
	if (key) {
		Sys_QueEvent(0, SE_KEY, key, event.down ? qtrue : qfalse, 0, NULL);
	}

	if (!event.down) {
		return;
	}

	// The keys that type a control character, which the window system does not report
	// as text: the editing keys, and a letter typed with Ctrl (Ctrl and V is paste).
	int character = 0;
	if (event.scancode == SDL_SCANCODE_BACKSPACE) {
		character = '\b';
	} else if (event.scancode == SDL_SCANCODE_TAB) {
		character = '\t';
	} else if (event.scancode == SDL_SCANCODE_RETURN || event.scancode == SDL_SCANCODE_KP_ENTER) {
		character = '\r';
	} else if (event.scancode == SDL_SCANCODE_ESCAPE) {
		character = 27;
	} else if ((event.mod & SDL_KMOD_CTRL) && event.key >= 'a' && event.key <= 'z') {
		character = static_cast<int>(event.key) - 'a' + 1;
	}
	if (character) {
		Sys_QueEvent(0, SE_CHAR, character, 0, 0, NULL);
	}
}

/*
===============
TextEvent

Text arrives as UTF-8. The engine's characters are single bytes, Latin-1 with the
additions of Windows; what does not fit is dropped.
===============
*/
void TextEvent(const char *text) {
	if (consoleKeyIsDown) {
		return;
	}

	const char *cursor = text;
	while (Uint32 codepoint = SDL_StepUTF8(&cursor, NULL)) {
		if (codepoint >= ' ' && codepoint < 0x100 && codepoint != 0x7f) {
			Sys_QueEvent(0, SE_CHAR, static_cast<int>(codepoint), 0, 0, NULL);
		}
	}
}

void MouseButtonEvent(const SDL_MouseButtonEvent &event) {
	int key = 0;

	switch (event.button) {
	case SDL_BUTTON_LEFT:
		key = A_MOUSE1;
		break;
	case SDL_BUTTON_RIGHT:
		key = A_MOUSE2;
		break;
	case SDL_BUTTON_MIDDLE:
		key = A_MOUSE3;
		break;
	case SDL_BUTTON_X1:
		key = A_MOUSE4;
		break;
	case SDL_BUTTON_X2:
		key = A_MOUSE5;
		break;
	default:
		break;
	}
	if (key) {
		Sys_QueEvent(0, SE_KEY, key, event.down ? qtrue : qfalse, 0, NULL);
	}
}

void MouseWheelEvent(const SDL_MouseWheelEvent &event) {
	if (event.y == 0) {
		return;
	}
	const int key = event.y > 0 ? A_MWHEELUP : A_MWHEELDOWN;

	Sys_QueEvent(0, SE_KEY, key, qtrue, 0, NULL);
	Sys_QueEvent(0, SE_KEY, key, qfalse, 0, NULL);
}

void SetMouseActive(bool active) {
	if (active == mouseActive || !Sys_Window()) {
		return;
	}
	if (!SDL_SetWindowRelativeMouseMode(Sys_Window(), active)) {
		Com_Printf("could not %s the mouse: %s\n", active ? "take" : "release", SDL_GetError());
	}
	mouseActive = active;
	motionX = 0;
	motionY = 0;
}

/*
===============
Scripted input

Three commands queue the event a device would have produced, so that a test or a
command line can press keys and move the mouse:

  sys_key <key name> <1 for down, 0 for up>     names as in "bind": MOUSE1, ENTER, a
  sys_char <character code>
  sys_mousemove <x> <y>                          relative motion
===============
*/
void ScriptedKey_f() {
	if (Cmd_Argc() != 3) {
		Com_Printf("sys_key <key name> <1 for down, 0 for up>\n");
		return;
	}
	const int key = Key_StringToKeynum(Cmd_Argv(1));
	if (key <= 0) {
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}
	Sys_QueEvent(0, SE_KEY, key, atoi(Cmd_Argv(2)) ? qtrue : qfalse, 0, NULL);
}

void ScriptedChar_f() {
	if (Cmd_Argc() != 2) {
		Com_Printf("sys_char <character code>\n");
		return;
	}
	Sys_QueEvent(0, SE_CHAR, atoi(Cmd_Argv(1)), 0, 0, NULL);
}

void ScriptedMouseMove_f() {
	if (Cmd_Argc() != 3) {
		Com_Printf("sys_mousemove <x> <y>\n");
		return;
	}
	Sys_QueEvent(0, SE_MOUSE, atoi(Cmd_Argv(1)), atoi(Cmd_Argv(2)), 0, NULL);
}

} // namespace

/*
===============
Sys_PumpEvents
===============
*/
void Sys_PumpEvents(void) {
	if (!Sys_Window()) {
		return;
	}

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_EVENT_QUIT:
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
			break;

		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			windowHasFocus = true;
			break;

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			windowHasFocus = false;
			consoleKeyIsDown = false;
			// keys that were down will not be seen coming up
			Key_ClearStates();
			break;

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			KeyEvent(event.key);
			break;

		case SDL_EVENT_TEXT_INPUT:
			TextEvent(event.text.text);
			break;

		case SDL_EVENT_MOUSE_MOTION:
			if (mouseActive) {
				motionX += event.motion.xrel;
				motionY += event.motion.yrel;
			}
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			MouseButtonEvent(event.button);
			break;

		case SDL_EVENT_MOUSE_WHEEL:
			MouseWheelEvent(event.wheel);
			break;

		default:
			break;
		}
	}

	// Motion is reported in whole units; what is left over is kept for the next time.
	const int wholeX = static_cast<int>(motionX);
	const int wholeY = static_cast<int>(motionY);
	if (wholeX || wholeY) {
		motionX -= static_cast<float>(wholeX);
		motionY -= static_cast<float>(wholeY);
		Sys_QueEvent(0, SE_MOUSE, wholeX, wholeY, 0, NULL);
	}
}

/*
===========
IN_Init
===========
*/
void IN_Init(void) {
	in_mouse = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE | CVAR_LATCH);

	Cmd_AddCommand("sys_key", ScriptedKey_f);
	Cmd_AddCommand("sys_char", ScriptedChar_f);
	Cmd_AddCommand("sys_mousemove", ScriptedMouseMove_f);
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown(void) {
	SetMouseActive(false);

	Cmd_RemoveCommand("sys_key");
	Cmd_RemoveCommand("sys_char");
	Cmd_RemoveCommand("sys_mousemove");
}

/*
==================
IN_Frame

Called every frame, even if not generating commands. Decides who has the mouse: the
game, or the desktop while the console is open over a window and while the window does
not have the keyboard.
==================
*/
void IN_Frame(void) {
	if (!Sys_Window() || !in_mouse->integer) {
		SetMouseActive(false);
		return;
	}

	if ((cls.keyCatchers & KEYCATCH_CONSOLE) && !Cvar_VariableIntegerValue("r_fullscreen")) {
		SetMouseActive(false);
		return;
	}

	SetMouseActive(windowHasFocus);
}
