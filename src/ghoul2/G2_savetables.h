// The field tables of the Ghoul2 model state that is saved
// (docs/decisions/0015-savegame-format.md). ghoul2_shared.h has to be included first.

#ifndef G2_SAVETABLES_H
#define G2_SAVETABLES_H

#include "../qcommon/sg_fields.h"

template <> const sg::Table &sg::TableOf<CGhoul2Info>();
template <> const sg::Table &sg::TableOf<surfaceInfo_t>();
template <> const sg::Table &sg::TableOf<boneInfo_t>();
template <> const sg::Table &sg::TableOf<boltInfo_t>();

// All of them, for the schema of a save file and for checking.
std::span<const sg::Table *const> G2_SaveTables();

#endif // G2_SAVETABLES_H
