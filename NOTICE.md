# Notices

## Licence

This project is distributed under the GNU General Public License, version 2. The full
text is in [LICENSE](LICENSE).

## Origin

The code derives from the source release of *Star Wars Jedi Knight: Jedi Academy*
published by Raven Software and Activision on SourceForge in April 2013. That release is
preserved unmodified at the tag `raven-2013`.

The release did not arrive here with a licence file. Its readme, as preserved by mirrors
of the SourceForge release, states that the code is copyright Activision 2003 and that
the source is released under GNU GPLv2. No "or any later version" wording is attached to
that statement, so the code is treated as GPL version 2 only.

## Game data

The game data (`assets*.pk3` and everything in it) is not part of this project, is not
covered by the licence above, and is not distributed here. A retail copy of the game is
required to run it.

## Trademarks

"Star Wars" and "Jedi Knight" are trademarks of their respective owners. This project is
not affiliated with or endorsed by them.

## Third-party components in the tree

These components carry their own notices in their source files, which are kept intact.

| Component | Location | Notice |
|---|---|---|
| Xing MP3 decoder, via FreeAmp | `src/mp3code/` | Copyright 1995-1997 Xing Technology; portions copyright 1998-1999 EMusic.com. GPL version 2 or later. |
| MD4 message digest | `src/qcommon/md4.cpp` | Copyright 1991-1992 RSA Data Security, Inc. Derived from the RSA Data Security, Inc. MD4 Message-Digest Algorithm. |
| OpenGL extension header | `src/renderer/glext.h` | Copyright 1991-2000 Silicon Graphics, Inc. SGI Free Software License B, version 1.1. |
| stb_image 2.34 and stb_image_write 1.19, unmodified, from commit `6b5799b` of github.com/nothings/stb | `external/stb/` | By Sean Barrett and contributors. Public domain, or MIT licence at the user's choice; see `external/stb/LICENSE`. |

Some engine files carry a copyright line of id Software, Inc. (1999-2000) from the Quake
III Arena code the game was built on; they are part of the release described under
"Origin".

This table is updated whenever a component is added, replaced or removed.

## Libraries the build downloads

These are not in the tree. The build downloads their release archives and links them
statically into the binaries ([ADR-0012](docs/decisions/0012-dependencies.md)); versions
and hashes are in `cmake/Dependencies.cmake`.

| Library | Notice |
|---|---|
| miniz | Copyright 2013-2014 RAD Game Tools and Valve Software; copyright 2010-2014 Rich Geldreich and Tenacious Software LLC. MIT licence. |
