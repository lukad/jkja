// The field tables of savegames: writing, the schema, reading into a structure that
// has changed since, and the check of a table against its structure.

#include "../src/qcommon/sg_fields.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <deque>
#include <string>

namespace {

enum colour_t { RED, GREEN, BLUE };

struct inner_t {
	float position[3];
	short ammo;
	bool armed;
};

struct thing_t {
	int health;
	char name[8];
	inner_t parts[2];
	thing_t *owner;
	char *label;
	void *extra;
	void *cache;
	colour_t colour;
	unsigned char flags;
	double precise;
	long long serial;
};

// thing_t as a later build has it: a member added in front, one removed (colour), one
// widened (health), one array shortened (name), one changed in kind (serial).
struct newInner_t {
	bool armed;
	float position[3];
	int charge;
	short ammo;
};

struct newThing_t {
	int added;
	long long health;
	char name[4];
	newInner_t parts[3];
	newThing_t *owner;
	char *label;
	void *extra;
	unsigned char flags;
	float precise;
	float serial;
};

} // namespace

SG_TABLE(inner_t)
SG_FIELD(position)
SG_FIELD(ammo)
SG_FIELD(armed)
SG_TABLE_END(inner_t)

SG_TABLE(thing_t)
SG_FIELD(health)
SG_FIELD(name)
SG_FIELD(parts)
SG_INDEX(owner, "thing")
SG_STRING(label)
SG_PRESENT(extra)
SG_OMIT(cache)
SG_FIELD(colour)
SG_FIELD(flags)
SG_FIELD(precise)
SG_FIELD(serial)
SG_TABLE_END(thing_t)

namespace {

// The newer structures are read with the names of the older ones.
const sg::Table &NewInnerTable() {
	using Self = newInner_t;
	static const sg::Field fields[] = {SG_FIELD(armed) SG_FIELD(position) SG_FIELD(charge)
	                                       SG_FIELD(ammo)};
	static const sg::Table table{"inner_t", sizeof(Self), alignof(Self), fields};
	return table;
}

const sg::Table &NewThingTable() {
	using Self = newThing_t;
	static const sg::Field fields[] = {
	    SG_FIELD(added) SG_FIELD(health) SG_FIELD(name)
	        sg::detail::MakeField<decltype(Self::parts)>(
	            "parts", offsetof(Self, parts), sg::Encoding::Struct, &NewInnerTable(), nullptr),
	    SG_INDEX(owner, "thing") SG_STRING(label) SG_PRESENT(extra) SG_FIELD(flags)
	        SG_FIELD(precise) SG_FIELD(serial)};
	static const sg::Table table{"thing_t", sizeof(Self), alignof(Self), fields};
	return table;
}

// Pointers to things are indexes into an array of them; strings are kept in a deque.
struct World {
	thing_t things[3];
	newThing_t newThings[3];
	std::deque<std::string> strings;
	int reused = 0;
};

sg::Codec CodecFor(World &world, bool newer) {
	sg::Codec codec;
	codec.context = &world;
	codec.indexOf = [](void *context, const char *kind, const void *pointer, const void *) {
		REQUIRE(std::string(kind) == "thing");
		const World *w = static_cast<const World *>(context);
		return pointer
		           ? static_cast<std::int32_t>(static_cast<const thing_t *>(pointer) - w->things)
				   : -1;
	};
	if (newer) {
		codec.pointerOf = [](void *context, const char *, std::int32_t index) -> void * {
			World *w = static_cast<World *>(context);
			return index < 0 ? nullptr : &w->newThings[index];
		};
	} else {
		codec.pointerOf = [](void *context, const char *, std::int32_t index) -> void * {
			World *w = static_cast<World *>(context);
			return index < 0 ? nullptr : &w->things[index];
		};
	}
	codec.newString = [](void *context, std::string_view text, char *original) {
		World *w = static_cast<World *>(context);
		if (original && text == original) {
			w->reused++;
			return original;
		}
		w->strings.emplace_back(text);
		return w->strings.back().data();
	};
	static int marker;
	codec.present = &marker;
	return codec;
}

thing_t SampleThing(World &world) {
	thing_t thing{};
	thing.health = -75;
	std::strcpy(thing.name, "kyle");
	thing.parts[0] = {{1.5f, -2.0f, 3.25f}, -7, true};
	thing.parts[1] = {{4.0f, 5.0f, 6.0f}, 300, false};
	thing.owner = &world.things[2];
	static char label[] = "a label";
	thing.label = label;
	thing.extra = &world;
	thing.cache = &world;
	thing.colour = BLUE;
	thing.flags = 0xf0;
	thing.precise = 0.1;
	thing.serial = -5000000000LL;
	return thing;
}

sg::Schema SchemaOf(const sg::Table &table) {
	std::vector<std::uint8_t> bytes;
	const sg::Table *const tables[] = {&table};
	sg::WriteSchema(bytes, tables);

	sg::Schema schema;
	std::string error;
	REQUIRE(schema.Parse(bytes, error));
	return schema;
}

} // namespace

TEST_CASE("a structure survives being written and read", "[savefields]") {
	World world{};
	const sg::Codec codec = CodecFor(world, false);
	const thing_t saved = SampleThing(world);

	std::vector<std::uint8_t> bytes;
	sg::Write(bytes, sg::TableOf<thing_t>(), &saved, codec);

	// the encodings have the widths the format promises, whatever the structure's are
	const std::size_t inner = 3 * 4 + 2 + 1;
	CHECK(bytes.size() == 4 + 8 + 2 * inner + 4 + (4 + 7) + 1 + 4 + 1 + 8 + 8);

	const sg::Schema schema = SchemaOf(sg::TableOf<thing_t>());

	thing_t loaded{};
	int untouched;
	loaded.cache = &untouched;
	std::span<const std::uint8_t> data = bytes;
	std::string error;
	REQUIRE(schema.Read(data, sg::TableOf<thing_t>(), &loaded, nullptr, codec, error));
	CHECK(data.empty());

	CHECK(loaded.health == -75);
	CHECK(std::string(loaded.name) == "kyle");
	CHECK(loaded.parts[0].position[2] == 3.25f);
	CHECK(loaded.parts[0].ammo == -7);
	CHECK(loaded.parts[0].armed);
	CHECK(loaded.parts[1].ammo == 300);
	CHECK(!loaded.parts[1].armed);
	CHECK(loaded.owner == &world.things[2]);
	CHECK(std::string(loaded.label) == "a label");
	CHECK(loaded.label != saved.label);
	CHECK(loaded.extra == codec.present);
	CHECK(loaded.cache == &untouched); // an omitted member is left alone
	CHECK(loaded.colour == BLUE);
	CHECK(loaded.flags == 0xf0);
	CHECK(loaded.precise == 0.1);
	CHECK(loaded.serial == -5000000000LL);
}

TEST_CASE("null pointers and reusable strings", "[savefields]") {
	World world{};
	const sg::Codec codec = CodecFor(world, false);
	thing_t saved = SampleThing(world);
	saved.owner = nullptr;
	saved.extra = nullptr;

	std::vector<std::uint8_t> bytes;
	sg::Write(bytes, sg::TableOf<thing_t>(), &saved, codec);
	const sg::Schema schema = SchemaOf(sg::TableOf<thing_t>());

	// the structure that is loaded over already has the same string
	thing_t original = SampleThing(world);
	thing_t loaded{};
	std::span<const std::uint8_t> data = bytes;
	std::string error;
	REQUIRE(schema.Read(data, sg::TableOf<thing_t>(), &loaded, &original, codec, error));
	CHECK(loaded.owner == nullptr);
	CHECK(loaded.extra == nullptr);
	CHECK(loaded.label == original.label);
	CHECK(world.reused == 1);

	saved.label = nullptr;
	bytes.clear();
	sg::Write(bytes, sg::TableOf<thing_t>(), &saved, codec);
	data = bytes;
	REQUIRE(schema.Read(data, sg::TableOf<thing_t>(), &loaded, nullptr, codec, error));
	CHECK(loaded.label == nullptr);
}

TEST_CASE("a file is read into a structure that has changed", "[savefields]") {
	World world{};
	const thing_t saved = SampleThing(world);

	std::vector<std::uint8_t> bytes;
	sg::Write(bytes, sg::TableOf<thing_t>(), &saved, CodecFor(world, false));
	const sg::Schema schema = SchemaOf(sg::TableOf<thing_t>());

	newThing_t loaded{};
	loaded.added = 11;
	loaded.serial = 2.5f;
	loaded.parts[0].charge = 9;
	loaded.parts[2].ammo = 42;
	std::span<const std::uint8_t> data = bytes;
	std::string error;
	REQUIRE(schema.Read(data, NewThingTable(), &loaded, nullptr, CodecFor(world, true), error));
	CHECK(data.empty());

	CHECK(loaded.added == 11);                    // not in the file: kept
	CHECK(loaded.health == -75);                  // widened, with its sign
	CHECK(std::string(loaded.name, 4) == "kyle"); // shortened: the first elements
	CHECK(loaded.parts[0].armed);                 // members of a nested structure, reordered
	CHECK(loaded.parts[0].position[0] == 1.5f);
	CHECK(loaded.parts[0].ammo == -7);
	CHECK(loaded.parts[0].charge == 9);
	CHECK(loaded.parts[1].ammo == 300);
	CHECK(loaded.parts[2].ammo == 42); // an element the file does not have
	CHECK(loaded.owner == &world.newThings[2]);
	CHECK(std::string(loaded.label) == "a label");
	CHECK(loaded.flags == 0xf0);
	CHECK(loaded.precise == 0.1f); // narrowed
	CHECK(loaded.serial == 2.5f);  // an integer in the file, a float here: left alone
}

TEST_CASE("damaged data is refused", "[savefields]") {
	World world{};
	const sg::Codec codec = CodecFor(world, false);
	const thing_t saved = SampleThing(world);

	std::vector<std::uint8_t> bytes;
	sg::Write(bytes, sg::TableOf<thing_t>(), &saved, codec);
	const sg::Schema schema = SchemaOf(sg::TableOf<thing_t>());

	thing_t loaded{};
	std::string error;

	// cut short at every length
	for (std::size_t length = 0; length < bytes.size(); length++) {
		std::span<const std::uint8_t> data(bytes.data(), length);
		CHECK_FALSE(schema.Read(data, sg::TableOf<thing_t>(), &loaded, nullptr, codec, error));
		CHECK_FALSE(error.empty());
	}

	// a schema that is cut short, or that names a structure it does not describe
	std::vector<std::uint8_t> schemaBytes;
	const sg::Table *const tables[] = {&sg::TableOf<thing_t>()};
	sg::WriteSchema(schemaBytes, tables);
	for (std::size_t length = 0; length < schemaBytes.size(); length++) {
		sg::Schema cut;
		CHECK_FALSE(cut.Parse(std::span<const std::uint8_t>(schemaBytes.data(), length), error));
	}

	// a structure the schema does not have
	sg::Schema other = SchemaOf(sg::TableOf<inner_t>());
	std::span<const std::uint8_t> data = bytes;
	CHECK_FALSE(other.Read(data, sg::TableOf<thing_t>(), &loaded, nullptr, codec, error));
	CHECK(error.find("thing_t") != std::string::npos);
}

namespace {

struct gappy_t {
	int first;
	short second;
	char third;
	char fourth;
	double fifth;
	char last;
};

sg::Table GappyTable(std::span<const sg::Field> fields) {
	return sg::Table{"gappy_t", sizeof(gappy_t), alignof(gappy_t), fields};
}

} // namespace

TEST_CASE("a table is checked against its structure", "[savefields]") {
	using Self = gappy_t;

	CHECK(sg::CheckLayout(sg::TableOf<thing_t>()).empty());
	CHECK(sg::CheckLayout(sg::TableOf<inner_t>()).empty());
	CHECK(sg::CheckLayout(NewThingTable()).empty());

	const sg::Field complete[] = {SG_FIELD(last) SG_FIELD(first) SG_FIELD(second) SG_FIELD(third)
	                                  SG_FIELD(fourth) SG_FIELD(fifth)};
	CHECK(sg::CheckLayout(GappyTable(complete)).empty());

	const sg::Field withoutSecond[] = {SG_FIELD(first) SG_FIELD(third) SG_FIELD(fourth)
	                                       SG_FIELD(fifth) SG_FIELD(last)};
	CHECK(sg::CheckLayout(GappyTable(withoutSecond)).find("before third") != std::string::npos);

	// The limit of the check: a member that is missing where alignment would have left
	// room anyway cannot be told from padding.
	const sg::Field withoutFourth[] = {SG_FIELD(first) SG_FIELD(second) SG_FIELD(third)
	                                       SG_FIELD(fifth) SG_FIELD(last)};
	CHECK(sg::CheckLayout(GappyTable(withoutFourth)).empty());

	const sg::Field withoutLast[] = {SG_FIELD(first) SG_FIELD(second) SG_FIELD(third)
	                                     SG_FIELD(fourth) SG_FIELD(fifth)};
	CHECK(sg::CheckLayout(GappyTable(withoutLast)).find("at the end") != std::string::npos);

	const sg::Field twice[] = {SG_FIELD(first) SG_FIELD(first) SG_FIELD(second) SG_FIELD(third)
	                               SG_FIELD(fourth) SG_FIELD(fifth) SG_FIELD(last)};
	CHECK(sg::CheckLayout(GappyTable(twice)).find("overlaps") != std::string::npos);
}
