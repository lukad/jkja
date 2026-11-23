// The field tables of the game structures that are saved
// (docs/decisions/0015-savegame-format.md).

#ifndef G_SAVETABLES_H
#define G_SAVETABLES_H

#include "../qcommon/sg_fields.h"

// The tables that the save code names. The tables of the structures inside these are
// reached through them.
template <> const sg::Table &sg::TableOf<gentity_s>();
template <> const sg::Table &sg::TableOf<gclient_s>();
template <> const sg::Table &sg::TableOf<gNPC_t>();
template <> const sg::Table &sg::TableOf<level_locals_t>();
template <> const sg::Table &sg::TableOf<Vehicle_t>();
template <> const sg::Table &sg::TableOf<parms_t>();
template <> const sg::Table &sg::TableOf<objectives_t>();

// All of them, for the schema of a save file and for checking.
std::span<const sg::Table *const> G_SaveTables();

#endif // G_SAVETABLES_H
