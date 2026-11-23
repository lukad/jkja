// The field tables of the saved game structures, checked against the structures: every
// member has to be in its table (docs/decisions/0015-savegame-format.md).

#include "../src/qcommon/sg_fields.h"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>

// src/game/g_savetables.cpp
std::span<const sg::Table *const> G_SaveTables();

// src/ghoul2/G2_savetables.cpp
std::span<const sg::Table *const> G2_SaveTables();

namespace {

void CheckWithNested(const sg::Table &table, std::set<const sg::Table *> &seen) {
	if (!seen.insert(&table).second) {
		return;
	}
	INFO(table.name);
	CHECK(sg::CheckLayout(table) == "");

	std::set<std::string> names;
	for (const sg::Field &field : table.fields) {
		INFO(field.name);
		CHECK(names.insert(field.name).second); // a reader finds members by name
		if (field.encoding == sg::Encoding::Struct) {
			CheckWithNested(*field.table, seen);
		}
	}
}

} // namespace

TEST_CASE("the tables of the game structures name every member", "[savetables]") {
	std::set<const sg::Table *> seen;
	for (const sg::Table *table : G_SaveTables()) {
		CheckWithNested(*table, seen);
	}
	CHECK(seen.size() >= G_SaveTables().size());
}

TEST_CASE("the tables of the Ghoul2 structures name every member", "[savetables]") {
	std::set<const sg::Table *> seen;
	for (const sg::Table *table : G2_SaveTables()) {
		CheckWithNested(*table, seen);
	}
	CHECK(seen.size() >= G2_SaveTables().size());
}
