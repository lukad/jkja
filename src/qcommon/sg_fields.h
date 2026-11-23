// Field tables for savegames (docs/decisions/0015-savegame-format.md).
//
// A structure that is saved has a table that names every one of its members. The table
// is what the writer and the reader walk, and it is written into the save file as that
// file's schema, so a reader whose structure has since gained or lost members can still
// read the file: members are matched by name.
//
// A table is written next to the structure's users, with the macros at the end of this
// file. The encoding of a member comes from its type; only pointers need to be told
// what they are.

#ifndef SG_FIELDS_H
#define SG_FIELDS_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace sg {

// How a member is represented in a file. All values are little-endian.
enum class Encoding : std::uint8_t {
	I8,
	U8,
	I16,
	U16,
	I32,
	U32,
	I64,
	U64,
	F32,
	F64,
	Bool,    // one byte, 0 or 1
	Struct,  // a structure with a table of its own, member after member
	Index,   // a pointer into something that can be counted, as a 32-bit index
	String,  // a pointer to a string: 32-bit length, -1 for a null pointer, then the bytes
	Present, // a pointer of which only "null or not" is kept, as one byte
	Omitted, // not in the file; a reader leaves the member as it finds it
};

struct Table;

// One member. An array member is one field with a count; arrays of arrays are counted
// through.
struct Field {
	const char *name;
	std::uint32_t offset;       // of the member in its structure
	std::uint32_t elementSize;  // in memory
	std::uint32_t elementAlign; // in memory
	std::uint32_t count;
	Encoding encoding;
	const Table *table; // Encoding::Struct: the table of the element
	const char *kind;   // Encoding::Index: what the pointer points into
};

struct Table {
	const char *name;
	std::uint32_t size;  // of the structure in memory
	std::uint32_t align; // of the structure in memory
	std::span<const Field> fields;
};

// The table of a structure. Specialised by SG_TABLE.
template <typename T> const Table &TableOf();

// How the pointers of a table's structures are turned into what is written, and back.
// The functions are the game's; the context is handed back to them untouched.
struct Codec {
	void *context = nullptr;

	// Writing: the index for a pointer of the given kind. `root` is the structure that
	// Write was called with.
	std::int32_t (*indexOf)(void *context, const char *kind, const void *pointer,
	                        const void *root) = nullptr;

	// Reading: the pointer for an index of the given kind.
	void *(*pointerOf)(void *context, const char *kind, std::int32_t index) = nullptr;

	// Reading: storage for a string that was read. `original` is the pointer that the
	// same member holds in the structure given to Read as the original, or null.
	char *(*newString)(void *context, std::string_view text, char *original) = nullptr;

	// Reading: what a pointer that is "not null" in the file is set to.
	void *present = nullptr;
};

// Appends the structure at `object` to `out`.
void Write(std::vector<std::uint8_t> &out, const Table &table, const void *object,
           const Codec &codec);

// Appends the description of the given tables, and of every table they refer to, to
// `out`.
void WriteSchema(std::vector<std::uint8_t> &out, std::span<const Table *const> tables);

// The tables a file was written with.
class Schema {
public:
	// Parses what WriteSchema wrote. Returns false and sets `error` if it is damaged.
	bool Parse(std::span<const std::uint8_t> data, std::string &error);

	// Reads one structure of the given table from the front of `data` into `object`
	// and removes what was read from `data`. Members that the file does not have are
	// left as they are; what the file has and the table does not is skipped. `original`
	// is null or a structure of the same type whose strings may be reused.
	// Returns false and sets `error` if the data does not fit the schema.
	bool Read(std::span<const std::uint8_t> &data, const Table &table, void *object,
	          const void *original, const Codec &codec, std::string &error) const;

private:
	struct FileField {
		std::string name;
		Encoding encoding = Encoding::I32;
		std::uint32_t count = 0;
		std::string reference; // the table of a Struct, the kind of an Index
	};
	struct FileTable {
		std::string name;
		std::vector<FileField> fields;
	};

	const FileTable *Find(std::string_view name) const;
	bool ReadTable(std::span<const std::uint8_t> &data, const FileTable &fileTable,
	               const Table *table, std::uint8_t *object, const std::uint8_t *original,
	               const Codec &codec, std::string &error, int depth) const;
	bool ReadField(std::span<const std::uint8_t> &data, const FileField &fileField,
	               const Field *field, std::uint8_t *object, const std::uint8_t *original,
	               const Codec &codec, std::string &error, int depth) const;

	std::vector<FileTable> tables_;
};

// Checks a table against the structure it describes: the members it names must cover
// the structure, in whatever order, without overlapping and without leaving more room
// between them than alignment explains. Returns an empty string, or what is wrong.
// A missing member is found unless it is so small that it fits where alignment would
// have left padding anyway, a bool between a short and an int for example.
std::string CheckLayout(const Table &table);

namespace detail {

template <typename T> using Element = std::remove_all_extents_t<T>;

template <typename T> constexpr std::uint32_t Count() {
	return static_cast<std::uint32_t>(sizeof(T) / sizeof(Element<T>));
}

template <typename E> constexpr Encoding EncodingOf() {
	if constexpr (std::is_same_v<E, bool>) {
		return Encoding::Bool;
	} else if constexpr (std::is_same_v<E, float>) {
		return Encoding::F32;
	} else if constexpr (std::is_same_v<E, double>) {
		return Encoding::F64;
	} else if constexpr (std::is_enum_v<E>) {
		return EncodingOf<std::underlying_type_t<E>>();
	} else if constexpr (std::is_integral_v<E>) {
		const bool isSigned = std::is_signed_v<E>;
		switch (sizeof(E)) {
		case 1:
			return isSigned ? Encoding::I8 : Encoding::U8;
		case 2:
			return isSigned ? Encoding::I16 : Encoding::U16;
		case 4:
			return isSigned ? Encoding::I32 : Encoding::U32;
		default:
			return isSigned ? Encoding::I64 : Encoding::U64;
		}
	} else {
		static_assert(std::is_class_v<E>, "this member's type cannot be saved");
		return Encoding::Struct;
	}
}

template <typename T>
Field MakeField(const char *name, std::size_t offset, Encoding encoding, const Table *table,
                const char *kind) {
	return Field{name,
	             static_cast<std::uint32_t>(offset),
	             static_cast<std::uint32_t>(sizeof(Element<T>)),
	             static_cast<std::uint32_t>(alignof(Element<T>)),
	             Count<T>(),
	             encoding,
	             table,
	             kind};
}

// A member that is not a pointer: the type says how it is written.
template <typename T> Field ValueField(const char *name, std::size_t offset) {
	using E = Element<T>;
	static_assert(!std::is_pointer_v<E>,
	              "a pointer member needs SG_INDEX, SG_STRING, SG_PRESENT or SG_OMIT");
	const Encoding encoding = EncodingOf<E>();
	const Table *table = nullptr;
	if constexpr (std::is_class_v<E>) {
		table = &TableOf<E>();
	}
	return MakeField<T>(name, offset, encoding, table, nullptr);
}

template <typename T>
Field PointerField(const char *name, std::size_t offset, Encoding encoding, const char *kind) {
	static_assert(std::is_pointer_v<Element<T>>, "this member is not a pointer");
	return MakeField<T>(name, offset, encoding, nullptr, kind);
}

} // namespace detail

} // namespace sg

// Writing a table:
//
//   SG_TABLE(thing_t)
//   SG_FIELD(health)            a value, an array of values or a structure with a table
//   SG_INDEX(owner, "entity")   a pointer that the codec turns into an index
//   SG_STRING(name)             a char pointer to a string
//   SG_PRESENT(extra)           a pointer of which only "null or not" is saved
//   SG_OMIT(cache)              a member that is not saved
//   SG_TABLE_END(thing_t)
//
// The table of a structure has to be in sight before a table that contains it.

#define SG_TABLE(Type)                                                                             \
	template <> const sg::Table &sg::TableOf<Type>() {                                             \
		using Self = Type;                                                                         \
		static const sg::Field fields[] = {

#define SG_FIELD(member)                                                                           \
	sg::detail::ValueField<decltype(Self::member)>(#member, offsetof(Self, member)),

#define SG_INDEX(member, kind)                                                                     \
	sg::detail::PointerField<decltype(Self::member)>(#member, offsetof(Self, member),              \
	                                                 sg::Encoding::Index, kind),

#define SG_STRING(member)                                                                          \
	sg::detail::PointerField<decltype(Self::member)>(#member, offsetof(Self, member),              \
	                                                 sg::Encoding::String, nullptr),

#define SG_PRESENT(member)                                                                         \
	sg::detail::PointerField<decltype(Self::member)>(#member, offsetof(Self, member),              \
	                                                 sg::Encoding::Present, nullptr),

#define SG_OMIT(member)                                                                            \
	sg::detail::MakeField<decltype(Self::member)>(#member, offsetof(Self, member),                 \
	                                              sg::Encoding::Omitted, nullptr, nullptr),

#define SG_TABLE_END(Type)                                                                         \
	}                                                                                              \
	;                                                                                              \
	static const sg::Table table{#Type, static_cast<std::uint32_t>(sizeof(Self)),                  \
	                             static_cast<std::uint32_t>(alignof(Self)), fields};               \
	return table;                                                                                  \
	}

#endif // SG_FIELDS_H
