// Writing and reading structures of a save file by their field tables
// (docs/decisions/0015-savegame-format.md). The functions are in g_savegame.cpp.

#ifndef G_SAVESTRUCT_H
#define G_SAVESTRUCT_H

#include "../qcommon/sg_fields.h"

// Writes count structures of one table to the save file as one chunk.
void G_SaveStruct(unsigned int chunkId, const sg::Table &table, const void *data, int count);

// Reads such a chunk. Members the file does not have keep the value they have in `data`.
// `original` is null or structures whose strings may be reused; it may be `data`.
void G_LoadStruct(unsigned int chunkId, const sg::Table &table, void *data, const void *original,
                  int count);

// The looped effects of the effects scheduler in cgame, which has no sight of the game's
// structures and therefore not of g_savetables.h.
struct SLoopedEffect;
template <> const sg::Table &sg::TableOf<SLoopedEffect>();

#endif // G_SAVESTRUCT_H
