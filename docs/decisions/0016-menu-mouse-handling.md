---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# The menus' missing mouse handling is taken from Raven's Jedi Outcast release

## Context and Problem Statement

The menus of the release cannot be used with a mouse. The single-player tree's menu
code is in places the console version with the PC code removed rather than set apart
([design document](../design.md), section 2). An inventory in M2 found:

* Most of the PC mouse code is still there, 800 to 900 lines: hit tests, mouse enter
  and leave with their scripts, capture, dragging of scroll bar thumbs, clicks in list
  boxes, on yes/no, multiple-choice and key-binding items.
* The code that connects it to the mouse is missing, about 170 to 190 lines:
  `Menu_HandleMouseMove` (the body is empty; the release has only a console body for
  text scrolling), `Menus_HandleOOBClick` (absent; the retail menus use
  `outOfBoundsClick` in six places), the mouse-button branch of `Menu_HandleKey`, the
  activation of an edit field by a click or by Enter (`g_editingField` is never set),
  and the click on the bar of a slider.
* Without that code nothing calls the mouse enter and leave functions, and several
  keyboard paths do not work either, because they test whether the pointer is over
  the item.
* The missing functions are nowhere in the release. The multiplayer tree's menu code
  is the console version as well, with more removed. They cannot be restored from the
  import commit, which is how every other overwritten PC part was restored in M1.

Where does the missing code come from?

## Decision Drivers

* The menus have to behave as the retail PC game's do; they are the first thing a
  play test meets.
* "Port, don't improve": restored code is preferred to rewritten code.
* Every line in the repository has a known origin and a licence compatible with GPLv2
  ([ADR-0001](0001-start-from-the-raven-source-release.md), design document,
  section 11).
* OpenJK's code is not imported (ADR-0001).

## Considered Options

* Take the missing functions from Raven's Jedi Outcast source release
* Write the missing functions anew
* Take the missing functions from id Software's Quake III: Team Arena source release

## Decision Outcome

Chosen option: "take the missing functions from Raven's Jedi Outcast source release",
because it is the nearest relative of the lost code: the same menu system by the same
authors, one game earlier, released on the same day under the same licence, and its
single-player `code/ui/ui_shared.cpp` has the PC bodies and no console code at all.

* Only what is missing is taken: the body of `Menu_HandleMouseMove`,
  `Menus_HandleOOBClick`, the mouse-button branch and the edit-field activation of
  `Menu_HandleKey`, and the mouse part of `Item_Slider_HandleKey`. Functions that the
  Jedi Academy tree already has stay as they are in Jedi Academy.
* The taken code is adapted to this tree where the two differ (item types and window
  flags that Jedi Academy added, key names), and the adaptations are a commit of their
  own after the commit that brings the code in unchanged.
* The commit that brings the code in names the file, the release and where the copy
  was obtained. `NOTICE.md` gets an entry for the Jedi Outcast release.
* The cursor's shader, whose registration is commented out in both trees of the
  release, is registered again; that line is in the tree already.
* The keyboard handling of sliders that M1 removed as console code is looked at again
  against the Jedi Outcast file, and restored from the import commit if the PC game
  had it.

### Consequences

* Good, because the menus get the code they most likely had, not an imitation of it.
* Good, because the amount taken is small and each function has one known source.
* Bad, because a second source release becomes an origin of code in this repository.
  It is Raven's own, published together with the Jedi Academy release under GPLv2;
  Raven's original download is no longer online, so the copy comes from a mirror of
  that release, and the mirror's history has to be checked against that.
* Bad, because Jedi Outcast is a year older: where Jedi Academy changed the menu
  system afterwards, the taken code needs adapting, and whether the adaptation matches
  retail Jedi Academy can only be judged by comparing behaviour with the retail game.

### Confirmation

The menu part of the play-test checklist, with the mouse only and with the keyboard
only: every item type in the main menu, the controls menu and the in-game menu can be
operated, including edit fields, sliders, list boxes and clicking outside a pop-up.
Where behaviour is in doubt, the retail executable under Wine is the reference.

## Pros and Cons of the Options

### Jedi Outcast source release

* Good, because same authors, same licence, same code lineage, PC code intact.
* Good, because it was verified during the inventory that its `ui_shared.cpp` has
  real bodies for `Menu_HandleMouseMove`, `Menus_HandleOOBClick`, the mouse branch of
  `Menu_HandleKey` and `Item_Slider_HandleKey`, and no `_XBOX` code.
* Bad, because the code predates Jedi Academy's additions to the menus.
* Bad, because the copy that is available is a third party's mirror.

### Write the missing functions anew

The functions are written against the helpers that exist, with the retail game's
behaviour as the specification.

* Good, because no code from outside the Jedi Academy release enters the repository.
* Good, because 170 to 190 lines is little.
* Bad, because it is new code in a place where the original existed: each difference
  from the lost code is a possible difference in behaviour, found only by play testing
  every menu.
* Bad, because the author of the new code cannot unsee the two published versions of
  it; calling the result independent would overstate it.

### Quake III: Team Arena source release

* Good, because it is the common ancestor, published by id Software under GPLv2 or
  later, and its origin is beyond doubt.
* Bad, because it is two games removed and written in C: Raven's changes for Jedi
  Outcast and Jedi Academy (key names, item types, scripts, sounds) are all missing,
  so most of the taken code would need rewriting.

## More Information

The inventory was made at commit `8ef4641`. The Jedi Outcast file was read at
`https://github.com/grayj/Jedi-Outcast`, path `code/ui/ui_shared.cpp`, whose readme
states "This code is copyright Activision 2002. This source is released under GNU
GPLv2."
