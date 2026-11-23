---
status: proposed
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Brightness is applied by a pass over the finished frame, in the legacy renderer's back end

## Context and Problem Statement

The release sets the brightness of the picture through the gamma ramp of the display:
`R_SetColorMappings` builds a table from `r_gamma` and `r_overBrightBits` and hands it
to `GLimp_SetGamma`. SDL3 has no gamma ramp, so the platform layer reports a device
without gamma control ([design document](../design.md), section 4.5), and the renderer
takes the path the release has for such devices. What that path does and does not do:

* With the defaults (`r_gamma` 1, `r_overBrightBits` 0, `r_intensity` 1) the table is
  the identity and the picture is the release's. Nothing is lost there.
* The table is applied to textures when they are loaded, and to nothing else: not to
  light maps, vertex colours, effects, videos or the menus. Images that are loaded
  already keep the old table until the renderer restarts.
* The "Brightness" slider of the video menu changes `r_gamma` between 0.5 and 3. In
  the release it acts at once and on everything. Here it does nothing that can be seen
  until the renderer restarts, and then it changes the textures only.
* `r_overBrightBits 1`, not the default and not in the menus, has no effect.

The game is dark, and its own menu sends the player to this slider. How is brightness
applied without a gamma ramp?

## Decision Drivers

* The menu's brightness slider has to do what it does in the retail game: act at
  once, on the whole picture, with the same curve.
* The legacy renderer is the reference the new renderer is compared with from M4; its
  picture should be the release's for the same settings.
* Graphics API calls live in the renderer's back end only; platform code lives in
  `src/sys` only.
* The legacy renderer is to be removed at the end of M5; work on it should stay small.
* Nothing may change the picture of other programs or outlive the game.

## Considered Options

* Leave the release's path for devices without gamma control as it is
* Apply the table to the finished frame in the back end
* Scale the finished frame with a blend
* Set the display's gamma ramp through each window system

## Decision Outcome

Chosen option: "apply the table to the finished frame in the back end", because it is
the only one that gives the slider the release's behaviour, and it uses means the
renderer already has.

* At the end of a frame, before the buffers are swapped, the back end copies the frame
  to a texture and draws it back through a fragment program that looks each colour
  channel up in the table of `R_SetColorMappings`, held in a texture of 256 texels.
  The dynamic glow of the release copies the scene and uses fragment programs in the
  same way.
* With such a pass available the renderer is told that the device has gamma control.
  `GLimp_SetGamma` stays the interface: the platform layer no longer implements it,
  the back end does, and stores the table instead of sending it to a display.
* Where the context lacks fragment programs, the path of today remains.
* While the table is the identity the pass is skipped, so the default settings cost
  nothing and draw exactly what they draw today.
* A screenshot reads the frame after the pass. The release corrects screenshots with
  the table because its ramp acts behind the frame buffer; that correction is not
  applied on top of the pass.
* The new renderer does not inherit this. Its brightness belongs to the tone mapping
  of M6.

### Consequences

* Good, because the brightness slider works as in the retail game, in a window as
  well as full screen, on every platform alike.
* Good, because the desktop and other programs are never touched and nothing has to
  be restored when the game ends or crashes.
* Good, because `r_overBrightBits` works again for those who set it.
* Bad, because the legacy renderer gains about 150 lines that are thrown away with it.
* Bad, because the frame is quantised to 8 bits before the curve is applied, where a
  display's ramp works on the signal at the precision of the hardware; a strong
  setting can show banding in dark gradients.
* Neutral, because the release never applies overbright bits in a window (the ramp
  would have brightened the desktop). That rule is kept; lifting it would be a change
  of behaviour and is not part of this decision.

### Confirmation

* A screenshot at the default settings is identical with and without the pass.
* Scripted input moves the menu's slider; a screenshot taken straight afterwards,
  without a restart of the renderer, is brighter over the whole picture, menus and
  light-mapped walls included, and the values of a grey ramp follow the table.
* The render smoke test passes in the sanitizer preset with `r_gamma` away from 1.
* Run-time behaviour on macOS and Windows cannot be checked from the development
  machine and is reported as not verified.

## Pros and Cons of the Options

### Leave the release's path as it is

* Good, because it is no work and is the release's own code.
* Good, because the default picture is already right.
* Bad, because a control of the game's own menu does nothing visible, on a game that
  is known to be dark.
* Bad, because after a restart the textures are brightened and the light maps are
  not, which is a picture the retail game never showed.

### Apply the table to the finished frame in the back end

* Good, because the curve is the release's, from the release's own table.
* Good, because it stays inside the back end and needs no platform code.
* Good, because the renderer has the building blocks: the copy of the frame into a
  rectangle texture and ARB fragment programs, both used by the dynamic glow.
* Bad, because of the lines added to a renderer that is to be removed.
* Bad, because of 8-bit quantisation before the curve.

### Scale the finished frame with a blend

One more full-screen quad, blended so that it multiplies what is there; fixed function
only.

* Good, because it needs no fragment program and is a dozen lines.
* Bad, because a multiplication is not the release's curve: it lifts nothing out of
  black and clips the highlights, so the slider would look different from the retail
  game's at every setting but the default.

### Set the display's gamma ramp through each window system

* Good, because it is what the release did, with the precision of the hardware.
* Bad, because Wayland gives a program no way to do it, and Wayland is where the
  port is played.
* Bad, because it changes every other program's picture and stays behind after a
  crash.
* Bad, because it is three implementations in `src/sys`, one per platform, where SDL
  deliberately offers none.

## More Information

* `src/renderer/tr_image.cpp`: `R_SetColorMappings`, `R_LightScaleTexture`;
  `src/renderer/tr_init.cpp`: defaults of `r_gamma`, `r_overBrightBits`, `r_intensity`
  and the correction of screenshots; `src/renderer/tr_backend.cpp`: the dynamic glow.
* SDL3 removed `SDL_SetWindowGammaRamp`; its migration guide points programs to a
  shader.
* To be revisited if the legacy renderer is kept beyond M5, or if banding shows in
  play tests.
