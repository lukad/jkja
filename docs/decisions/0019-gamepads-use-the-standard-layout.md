---
status: accepted
date: 2026-11-24
decision-makers: Luka Dornhecker
---

# Gamepads are read through SDL3's standard layout, not through the release's joystick model

## Context and Problem Statement

The release's PC input code (`win32/win_input.cpp`) reads the first joystick the
Windows multimedia API reports, as a list of numbered buttons and up to six numbered
axes. Its model is the flight stick of its time: the first two axes move the player or,
by `joy_xbutton` and `joy_ybutton`, press cursor keys; the third and fourth press
keys; the fifth and sixth move the mouse like a track ball; the hat presses keys.
Nothing looks around by an analog axis, and which stick of a pad is which axis depends
on the driver.

The roadmap asks for gamepads on SDL3 ([design document](../design.md), sections 4.5
and 8). SDL3 offers two interfaces: the joystick as the driver numbers it, and the
gamepad, where every known device is mapped to one layout of two sticks, two triggers,
a d-pad and named buttons. Which model does the port follow?

## Decision Drivers

* A pad of today should work when it is plugged in: one stick moves, the other looks.
* The game's own controls menu, which is game data, has to keep working: "enable
  joystick", "X axis as buttons", "Y axis as buttons", "joystick threshold", and the
  binding of joystick buttons to actions.
* "Port, don't improve" speaks for the release's model; it would however give a pad
  no analog look at all.
* The client code that turns input into movement is legacy code and should not
  change.
* Platform code lives in `src/sys`.

## Considered Options

* SDL3's gamepad interface with the standard layout
* The release's joystick model on SDL3's joystick interface
* No gamepad support in M2

## Decision Outcome

Chosen option: "SDL3's gamepad interface with the standard layout", because it is the
only one with which a pad of today plays the game as its player expects, and it can be
had without touching the client code.

* The first gamepad SDL reports is used, and a pad may be plugged in and pulled out
  while the game runs. Devices that SDL does not know as gamepads are not used.
* `in_joystick` switches the pad on, as the menu's "enable joystick" does. Its default
  stays 0, the release's.
* The left stick moves: its axes become the engine's joystick axis events for sideways
  and forward movement, which the release's client code already turns into movement,
  walking until the stick is almost fully deflected and running from there. `joy_xbutton` and `joy_ybutton` make an axis press the
  cursor keys instead, as in the release. Their defaults are both 0, where the release
  has `joy_xbutton` at 1: the layout decided here is a stick that moves.
* The right stick looks: its deflection becomes mouse motion, so sensitivity, inversion
  and smoothing of the mouse apply to it and it moves the pointer of the menus. The new
  setting `joy_lookspeed` is the motion per second at full deflection.
* `joy_threshold` is the dead zone of both sticks and the point at which a trigger
  counts as pressed.
* Buttons, triggers and d-pad press the engine's joystick keys, which the controls
  menu binds like any key: A, B, X, Y, the shoulders, back, start and the stick
  buttons are `JOY0` to `JOY9` in that order, which is the order Windows gave the
  buttons of such a pad; the triggers are `JOY16` and `JOY17`; the d-pad presses
  `JOY24` to `JOY27`, the keys of the release's hat.
* The game data binds nothing to joystick keys, so a pad's buttons do nothing until
  they are bound in the controls menu.
* Rumble is not part of this: the release's force feedback was removed in M0.

### Consequences

* Good, because sticks, triggers and buttons are the same on every pad SDL knows,
  whatever its driver calls them.
* Good, because everything is in `src/sys`; the client code is the release's.
* Good, because the controls menu keeps working as it is.
* Bad, because this is behaviour the retail game did not have, in a port that
  otherwise keeps to it.
* Bad, because a flight stick or another device without a gamepad mapping is not
  usable, where the release would have read it.
* Bad, because the menus are still operated by mouse and keyboard: the right stick
  moves the pointer, but no button of the pad clicks, and Escape, which opens the
  menu, is not a key that can be bound. The release was the same.

### Confirmation

* SDL's virtual joystick stands in for a pad in scripted runs: with `in_joystick` 1,
  deflecting the left stick moves the player, deflecting the right stick turns the
  view, and a button bound to an action triggers it.
* The same run under the sanitizer preset is free of reports.
* A real pad has to be tried by a person; until then that is reported as not verified,
  and so is everything on macOS and Windows.

## Pros and Cons of the Options

### SDL3's gamepad interface with the standard layout

* Good, because of analog look and a layout that does not depend on the driver.
* Good, because SDL maintains the mappings.
* Bad, because it is new behaviour.

### The release's joystick model on SDL3's joystick interface

* Good, because it is the release's behaviour, line for line.
* Bad, because on a pad the right stick ends up as key presses or as a track ball
  whose speed depends on the frame rate, depending on how the driver numbers its axes.
* Bad, because the release's analog sideways axis is inverted, which its default of
  `joy_xbutton` 1 hides.

### No gamepad support in M2

* Good, because it is no work, and `src/win32` can go without its joystick code
  being ported.
* Bad, because the design document names gamepads as part of the platform layer.

## More Information

* `raven-2013:code/win32/win_input.cpp`, `IN_JoyMove`.
* `src/client/cl_input.cpp`, `CL_JoystickEvent` and `CL_JoystickMove`.
* To be revisited if operating the menus from the pad is wanted: that needs a rule
  for when a button is a click and when it is a key to bind.
