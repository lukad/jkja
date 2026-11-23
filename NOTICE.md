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
| Xing MP3 decoder, via FreeAmp | `mp3code/` | Copyright 1995-1997 Xing Technology; portions copyright 1998-1999 EMusic.com. GPL version 2 or later. |
| Independent JPEG Group's software, release 6 | `jpeg-6/` | Copyright 1991-1995 Thomas G. Lane. This software is based in part on the work of the Independent JPEG Group. |
| zlib 1.1.3: its headers, and a variant derived from it | `zlib/`, `zlib32/` | Copyright 1995-1998 Jean-loup Gailly and Mark Adler. zlib licence. |
| MD4 message digest | `qcommon/md4.cpp` | Copyright 1991-1992 RSA Data Security, Inc. Derived from the RSA Data Security, Inc. MD4 Message-Digest Algorithm. |
| OpenGL extension header | `renderer/glext.h` | Copyright 1991-2000 Silicon Graphics, Inc. SGI Free Software License B, version 1.1. |
| OpenAL headers | `client/openal/` | Copyright 1999-2000 by authors. GNU Library General Public License, version 2 or later. |

Some engine files carry a copyright line of id Software, Inc. (1999-2000) from the Quake
III Arena code the game was built on; they are part of the release described under
"Origin".

Locations are relative to `src/`. This table is updated whenever a component is
added, replaced or removed.
