// Gamepads, in the layout SDL gives every pad it knows
// (docs/decisions/0019-gamepads-use-the-standard-layout.md). The left stick becomes the
// engine's joystick axis events, the right stick mouse motion, and buttons, triggers and
// d-pad the engine's joystick keys.

#include "sys_local.h"

#include "../client/client.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <iterator>

namespace {

cvar_t *in_joystick;
cvar_t *joy_threshold;
cvar_t *joy_xbutton;
cvar_t *joy_ybutton;
cvar_t *joy_lookspeed;

// True while SDL's gamepad subsystem is up, which it is while in_joystick is set.
bool subsystemIsUp;

// The pad in use, or null.
SDL_Gamepad *pad;

// What the engine has been told, so that only changes are reported and everything can
// be let go of when the pad goes away.
int sentAxis[MAX_JOYSTICK_AXIS];
bool keyIsDown[MAX_KEYS];

// Motion of the right stick that did not yet make a whole unit, and when it was last
// looked at.
float lookX;
float lookY;
Uint64 lastLookTime;

// The pad of the scripted commands.
SDL_JoystickID scriptedID;
SDL_Joystick *scriptedJoystick;

struct ButtonKey {
	SDL_GamepadButton button;
	int key;
};

// The first ten are in the order in which Windows numbered the buttons of such a pad.
// The d-pad has the keys of the release's hat.
constexpr ButtonKey buttonKeys[] = {
    {SDL_GAMEPAD_BUTTON_SOUTH, A_JOY0},          {SDL_GAMEPAD_BUTTON_EAST, A_JOY1},
    {SDL_GAMEPAD_BUTTON_WEST, A_JOY2},           {SDL_GAMEPAD_BUTTON_NORTH, A_JOY3},
    {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, A_JOY4},  {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, A_JOY5},
    {SDL_GAMEPAD_BUTTON_BACK, A_JOY6},           {SDL_GAMEPAD_BUTTON_START, A_JOY7},
    {SDL_GAMEPAD_BUTTON_LEFT_STICK, A_JOY8},     {SDL_GAMEPAD_BUTTON_RIGHT_STICK, A_JOY9},
    {SDL_GAMEPAD_BUTTON_GUIDE, A_JOY10},         {SDL_GAMEPAD_BUTTON_MISC1, A_JOY11},
    {SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, A_JOY12}, {SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, A_JOY13},
    {SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, A_JOY14}, {SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, A_JOY15},
    {SDL_GAMEPAD_BUTTON_TOUCHPAD, A_JOY18},      {SDL_GAMEPAD_BUTTON_DPAD_UP, A_JOY24},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, A_JOY25},     {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, A_JOY26},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, A_JOY27},
};

constexpr int leftTriggerKey = A_JOY16;
constexpr int rightTriggerKey = A_JOY17;

void SetKey(int key, bool down) {
	if (keyIsDown[key] == down) {
		return;
	}
	keyIsDown[key] = down;
	Sys_QueEvent(0, SE_KEY, key, down ? qtrue : qfalse, 0, NULL);
}

void SetAxis(int axis, int value) {
	if (sentAxis[axis] == value) {
		return;
	}
	sentAxis[axis] = value;
	Sys_QueEvent(0, SE_JOYSTICK_AXIS, axis, value, 0, NULL);
}

// Lets go of everything the pad holds down.
void ReleaseAll() {
	for (int key = 0; key < MAX_KEYS; key++) {
		if (keyIsDown[key]) {
			SetKey(key, false);
		}
	}
	for (int axis = 0; axis < MAX_JOYSTICK_AXIS; axis++) {
		SetAxis(axis, 0);
	}
	lookX = 0;
	lookY = 0;
}

void ClosePad() {
	if (!pad) {
		return;
	}
	ReleaseAll();
	SDL_CloseGamepad(pad);
	pad = nullptr;
}

void OpenPad(SDL_JoystickID id) {
	if (pad) {
		return;
	}
	pad = SDL_OpenGamepad(id);
	if (!pad) {
		Com_Printf("could not open the gamepad: %s\n", SDL_GetError());
		return;
	}
	const char *name = SDL_GetGamepadName(pad);
	Com_Printf("Gamepad: %s\n", name ? name : "without a name");
	lastLookTime = SDL_GetTicksNS();
}

// The first pad that is plugged in, if none is in use.
void OpenAnyPad() {
	if (pad) {
		return;
	}
	int count = 0;
	SDL_JoystickID *ids = SDL_GetGamepads(&count);
	if (ids && count > 0) {
		OpenPad(ids[0]);
	}
	SDL_free(ids);
}

void DetachScriptedPad() {
	if (scriptedJoystick) {
		SDL_CloseJoystick(scriptedJoystick);
		scriptedJoystick = nullptr;
	}
	if (scriptedID) {
		SDL_DetachVirtualJoystick(scriptedID);
		scriptedID = 0;
	}
}

void StartSubsystem() {
	if (subsystemIsUp) {
		return;
	}
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		Com_Printf("no gamepads: %s\n", SDL_GetError());
		Cvar_Set("in_joystick", "0");
		return;
	}
	subsystemIsUp = true;
	OpenAnyPad();
	if (!pad) {
		Com_Printf("No gamepad is plugged in.\n");
	}
}

void StopSubsystem() {
	if (!subsystemIsUp) {
		return;
	}
	ClosePad();
	DetachScriptedPad();
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
	subsystemIsUp = false;
}

// An axis as a number from -1 to 1, or from 0 to 1 for a trigger.
float AxisValue(SDL_GamepadAxis axis) {
	return std::clamp(static_cast<float>(SDL_GetGamepadAxis(pad, axis)) / 32767.0f, -1.0f, 1.0f);
}

// Nothing inside the dead zone, and from its edge to full deflection the whole range.
float BeyondDeadZone(float value, float deadZone) {
	const float size = std::fabs(value);
	if (size <= deadZone) {
		return 0;
	}
	return std::copysign((size - deadZone) / (1.0f - deadZone), value);
}

/*
===============
Scripted pad

Two commands deflect the axes and press the buttons of a pad that exists in SDL only,
so that a test or a command line can play with a pad. It is plugged in by the first of
them and goes the way a real pad does from there. in_joystick has to be set.

  sys_padaxis <leftx|lefty|rightx|righty|lefttrigger|righttrigger> <-1 to 1>
  sys_padbutton <a|b|x|y|back|guide|start|leftstick|rightstick|leftshoulder|
                 rightshoulder|dpup|dpdown|dpleft|dpright|...> <1 for down, 0 for up>
===============
*/
bool AttachScriptedPad() {
	if (scriptedJoystick) {
		return true;
	}
	if (!subsystemIsUp) {
		Com_Printf("in_joystick is not set\n");
		return false;
	}

	SDL_VirtualJoystickDesc description;
	SDL_INIT_INTERFACE(&description);
	description.type = SDL_JOYSTICK_TYPE_GAMEPAD;
	description.naxes = SDL_GAMEPAD_AXIS_COUNT;
	description.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
	description.name = "Scripted gamepad";

	scriptedID = SDL_AttachVirtualJoystick(&description);
	if (scriptedID) {
		scriptedJoystick = SDL_OpenJoystick(scriptedID);
	}
	if (!scriptedJoystick) {
		Com_Printf("could not plug in the scripted gamepad: %s\n", SDL_GetError());
		DetachScriptedPad();
		return false;
	}

	// A trigger at rest is at the low end of its axis, not in the middle.
	SDL_SetJoystickVirtualAxis(scriptedJoystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
	                           SDL_JOYSTICK_AXIS_MIN);
	SDL_SetJoystickVirtualAxis(scriptedJoystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
	                           SDL_JOYSTICK_AXIS_MIN);
	return true;
}

void ScriptedPadAxis_f() {
	if (Cmd_Argc() != 3) {
		Com_Printf("sys_padaxis <axis name> <-1 to 1>\n");
		return;
	}
	const SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(Cmd_Argv(1));
	if (axis == SDL_GAMEPAD_AXIS_INVALID) {
		Com_Printf("\"%s\" isn't an axis of a gamepad\n", Cmd_Argv(1));
		return;
	}
	if (!AttachScriptedPad()) {
		return;
	}

	const float value = std::clamp(static_cast<float>(atof(Cmd_Argv(2))), -1.0f, 1.0f);
	const bool isTrigger =
	    axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
	// The joystick behind a trigger goes from the low end of the axis to the high end.
	const float position = isTrigger ? std::max(value, 0.0f) * 2.0f - 1.0f : value;
	SDL_SetJoystickVirtualAxis(scriptedJoystick, axis,
	                           static_cast<Sint16>(std::lround(position * 32767.0f)));
}

void ScriptedPadButton_f() {
	if (Cmd_Argc() != 3) {
		Com_Printf("sys_padbutton <button name> <1 for down, 0 for up>\n");
		return;
	}
	const SDL_GamepadButton button = SDL_GetGamepadButtonFromString(Cmd_Argv(1));
	if (button == SDL_GAMEPAD_BUTTON_INVALID) {
		Com_Printf("\"%s\" isn't a button of a gamepad\n", Cmd_Argv(1));
		return;
	}
	if (!AttachScriptedPad()) {
		return;
	}
	SDL_SetJoystickVirtualButton(scriptedJoystick, button, atoi(Cmd_Argv(2)) != 0);
}

} // namespace

/*
===============
Gamepad_Init
===============
*/
void Gamepad_Init(void) {
	in_joystick = Cvar_Get("in_joystick", "0", CVAR_ARCHIVE);
	joy_threshold = Cvar_Get("joy_threshold", "0.15", CVAR_ARCHIVE);
	// make the left stick press the cursor keys instead of moving the player
	joy_xbutton = Cvar_Get("joy_xbutton", "0", CVAR_ARCHIVE);
	joy_ybutton = Cvar_Get("joy_ybutton", "0", CVAR_ARCHIVE);
	// mouse motion a second with the right stick fully deflected
	joy_lookspeed = Cvar_Get("joy_lookspeed", "1000", CVAR_ARCHIVE);

	Cmd_AddCommand("sys_padaxis", ScriptedPadAxis_f);
	Cmd_AddCommand("sys_padbutton", ScriptedPadButton_f);
}

/*
===============
Gamepad_Shutdown
===============
*/
void Gamepad_Shutdown(void) {
	StopSubsystem();

	Cmd_RemoveCommand("sys_padaxis");
	Cmd_RemoveCommand("sys_padbutton");
}

/*
===============
Gamepad_Event

Takes the events of pads being plugged in and pulled out and of their buttons. Returns
whether the event was one of them.
===============
*/
bool Gamepad_Event(const SDL_Event &event) {
	switch (event.type) {
	case SDL_EVENT_GAMEPAD_ADDED:
		if (subsystemIsUp) {
			OpenPad(event.gdevice.which);
		}
		return true;

	case SDL_EVENT_GAMEPAD_REMOVED:
		if (pad && SDL_GetGamepadID(pad) == event.gdevice.which) {
			Com_Printf("The gamepad was pulled out.\n");
			ClosePad();
			OpenAnyPad();
		}
		return true;

	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		if (pad && SDL_GetGamepadID(pad) == event.gbutton.which) {
			const auto found = std::find_if(
			    std::begin(buttonKeys), std::end(buttonKeys),
			    [&](const ButtonKey &entry) { return entry.button == event.gbutton.button; });
			if (found != std::end(buttonKeys)) {
				SetKey(found->key, event.gbutton.down);
			}
		}
		return true;

	default:
		return false;
	}
}

/*
===============
Gamepad_Frame

Follows in_joystick, and reports where sticks and triggers are. Called once a frame,
after the events have been taken.
===============
*/
void Gamepad_Frame(void) {
	if (in_joystick->integer && Sys_Window()) {
		StartSubsystem();
	} else {
		StopSubsystem();
	}
	if (!pad) {
		return;
	}

	const float deadZone = std::clamp(joy_threshold->value, 0.0f, 0.95f);

	// The left stick moves, unless an axis of it is to press the cursor keys. SDL
	// counts down and right as positive; the engine forward and right.
	const float sideways = AxisValue(SDL_GAMEPAD_AXIS_LEFTX);
	const float backward = AxisValue(SDL_GAMEPAD_AXIS_LEFTY);

	const bool sidewaysIsKeys = joy_xbutton->integer != 0;
	SetKey(A_CURSOR_LEFT, sidewaysIsKeys && sideways < -deadZone);
	SetKey(A_CURSOR_RIGHT, sidewaysIsKeys && sideways > deadZone);
	SetAxis(AXIS_SIDE,
	        sidewaysIsKeys
	            ? 0
				: static_cast<int>(std::lround(BeyondDeadZone(sideways, deadZone) * 127.0f)));

	const bool forwardIsKeys = joy_ybutton->integer != 0;
	SetKey(A_CURSOR_UP, forwardIsKeys && backward < -deadZone);
	SetKey(A_CURSOR_DOWN, forwardIsKeys && backward > deadZone);
	SetAxis(AXIS_FORWARD,
	        forwardIsKeys
	            ? 0
				: static_cast<int>(std::lround(BeyondDeadZone(-backward, deadZone) * 127.0f)));

	SetKey(leftTriggerKey, AxisValue(SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > deadZone);
	SetKey(rightTriggerKey, AxisValue(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > deadZone);

	// The right stick is a mouse that moves for as long as it is deflected. A frame that
	// took long, a level being loaded, does not turn the view by its whole length.
	const Uint64 now = SDL_GetTicksNS();
	const float seconds = std::min(
	    static_cast<float>(now - lastLookTime) / static_cast<float>(SDL_NS_PER_SECOND), 0.1f);
	lastLookTime = now;

	const float speed = joy_lookspeed->value * seconds;
	lookX += BeyondDeadZone(AxisValue(SDL_GAMEPAD_AXIS_RIGHTX), deadZone) * speed;
	lookY += BeyondDeadZone(AxisValue(SDL_GAMEPAD_AXIS_RIGHTY), deadZone) * speed;

	const int wholeX = static_cast<int>(lookX);
	const int wholeY = static_cast<int>(lookY);
	if (wholeX || wholeY) {
		lookX -= static_cast<float>(wholeX);
		lookY -= static_cast<float>(wholeY);
		Sys_QueEvent(0, SE_MOUSE, wholeX, wholeY, 0, NULL);
	}
}
