---
status: proposed
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# PC code that the release lacks is taken from the Jedi Outcast release, case by case

## Context and Problem Statement

[ADR-0016](0016-menu-mouse-handling.md) took the menus' mouse handling from Raven's
Jedi Outcast source release, because the Jedi Academy release holds only the console
version of it. That record is about the menus. The first play test found the same
situation elsewhere: the loading screen draws its progress as a vertical saber, from
two images (`gfx/menus/newFront/SaberLoad`, `GlowLoad`) that the PC game data does not
contain, so a blank upright box appears. The PC game data ships the images of a
horizontal bar (`gfx/hud/mp_levelload`, `load_tick`, `load_tick_cap`) that nothing in
the release uses. The function that draws that bar, `CG_LoadBar`, is in neither tree of
the release and not among its commented-out lines; the Jedi Outcast release has it,
with exactly those three image names.

More places of this kind are to be expected as play testing goes on. Is each of them a
decision of its own?

## Decision Drivers

* The game has to behave as the retail PC game does.
* "Port, don't improve": restored code is preferred to rewritten code.
* Every line in the repository has a known origin and a licence compatible with GPLv2.
* Decisions of the same kind should not have to be taken again for every function.

## Considered Options

* A standing rule with conditions: such code is taken from the Jedi Outcast release
* A decision record for every case
* Write the missing code anew in every case

## Decision Outcome

Chosen option: "a standing rule with conditions", because the reasoning of ADR-0016
holds for every such case, and the conditions keep the rule from becoming a licence to
import at will.

Code is taken from the Jedi Outcast release when all of these hold:

1. The Jedi Academy release has only the console version of the code, or none.
2. The PC version cannot be restored from the import commit: it is in neither tree and
   not among the commented-out lines next to the console code.
3. The Jedi Outcast code fits what the Jedi Academy PC game data and menus expect
   (names of images, menu keywords, cvars), which is the evidence that the PC game ran
   code like it.

How it is done is as in ADR-0016: only what is missing is taken; one commit brings the
code in unchanged and names the file, the mirror, its commit and the file's checksum;
adaptations follow in a commit of their own; `NOTICE.md` lists what was taken. Each use
of the rule is listed in the design document, section 2.

The first use is the loading bar: `CG_LoadBar` and the registration of its three images.

### Consequences

* Good, because findings of the play test of this kind can be fixed without a pause for
  a decision each time.
* Good, because the conditions and the commit discipline keep every taken line traceable.
* Bad, because the amount of code from a second release grows with every use; the list
  in the design document is what keeps it in view.
* Bad, because Jedi Outcast is a year older than Jedi Academy; where the two differ, the
  adaptation can only be judged against the retail game.

### Confirmation

For each use: the three conditions are stated in the commit message, and the play test
or a screenshot shows the PC behaviour. The retail executable under Wine is the
reference where behaviour is in doubt.

## Pros and Cons of the Options

### A standing rule with conditions

* Good, because one decision covers a class of findings.
* Bad, because a rule is applied by judgement; a doubtful case still needs to be asked.

### A decision record for every case

* Good, because every import is looked at on its own.
* Bad, because the answer is the same each time, and the play test stops for it.

### Write the missing code anew

* Good, because no further code from outside the Jedi Academy release.
* Bad, because of the reasons in ADR-0016: new code where the original exists, and no
  claim to independence from two published versions.

## More Information

The Jedi Outcast code was read at `https://github.com/grayj/Jedi-Outcast`, commit
`85f58467344d3ccbc6e2501a9af573ff4488a898`, files `code/cgame/cg_info.cpp` and
`code/cgame/cg_main.cpp`.
