// Writer, schema and reader for the field tables of savegames
// (docs/decisions/0015-savegame-format.md).

#include "sg_fields.h"

#include <algorithm>
#include <cstring>

namespace sg {

namespace {

// Structures nest a few levels deep. A file that claims more than this is damaged.
constexpr int maxDepth = 32;

// Schema strings are names of members and types, with their length in one byte.
constexpr std::size_t maxNameLength = 255;

using Bytes = std::vector<std::uint8_t>;
using View = std::span<const std::uint8_t>;

//
// writing little-endian values
//

void PutBytes(Bytes &out, std::uint64_t value, std::size_t width) {
	for (std::size_t i = 0; i < width; i++) {
		out.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
	}
}

void PutName(Bytes &out, std::string_view name) {
	name = name.substr(0, maxNameLength); // the length is written as one byte
	PutBytes(out, name.size(), 1);
	out.insert(out.end(), name.begin(), name.end());
}

//
// reading little-endian values
//

bool TakeBytes(View &in, std::size_t width, std::uint64_t &value) {
	if (in.size() < width) {
		return false;
	}
	value = 0;
	for (std::size_t i = 0; i < width; i++) {
		value |= static_cast<std::uint64_t>(in[i]) << (8 * i);
	}
	in = in.subspan(width);
	return true;
}

bool TakeName(View &in, std::string &name) {
	std::uint64_t length = 0;
	if (!TakeBytes(in, 1, length) || in.size() < length) {
		return false;
	}
	name.assign(reinterpret_cast<const char *>(in.data()), static_cast<std::size_t>(length));
	in = in.subspan(static_cast<std::size_t>(length));
	return true;
}

//
// encodings
//

bool IsInteger(Encoding encoding) {
	return encoding <= Encoding::U64;
}

bool IsFloat(Encoding encoding) {
	return encoding == Encoding::F32 || encoding == Encoding::F64;
}

bool IsSigned(Encoding encoding) {
	return encoding == Encoding::I8 || encoding == Encoding::I16 || encoding == Encoding::I32 ||
	       encoding == Encoding::I64;
}

// The number of bytes of a value with a fixed width, 0 for the other encodings.
std::size_t Width(Encoding encoding) {
	switch (encoding) {
	case Encoding::I8:
	case Encoding::U8:
	case Encoding::Bool:
	case Encoding::Present:
		return 1;
	case Encoding::I16:
	case Encoding::U16:
		return 2;
	case Encoding::I32:
	case Encoding::U32:
	case Encoding::F32:
	case Encoding::Index:
		return 4;
	case Encoding::I64:
	case Encoding::U64:
	case Encoding::F64:
		return 8;
	default:
		return 0;
	}
}

bool IsKnown(std::uint64_t encoding) {
	return encoding <= static_cast<std::uint64_t>(Encoding::Present);
}

// A value of memory, of 1, 2, 4 or 8 bytes, as an integer. The value keeps its bit
// pattern; a signed one is sign-extended.
std::uint64_t LoadInteger(const std::uint8_t *from, std::size_t size, bool isSigned) {
	std::uint64_t value = 0;
	std::memcpy(&value, from, size); // every target is little-endian (design, 4.8)
	if (isSigned && size < 8 && (value >> (8 * size - 1)) & 1) {
		value |= ~std::uint64_t{0} << (8 * size);
	}
	return value;
}

void StoreInteger(std::uint8_t *to, std::size_t size, std::uint64_t value) {
	std::memcpy(to, &value, size);
}

void SignExtend(std::uint64_t &value, std::size_t width) {
	if (width < 8 && (value >> (8 * width - 1)) & 1) {
		value |= ~std::uint64_t{0} << (8 * width);
	}
}

std::uint32_t AlignUp(std::uint32_t value, std::uint32_t alignment) {
	return (value + alignment - 1) / alignment * alignment;
}

//
// writing
//

void WriteTable(Bytes &out, const Table &table, const std::uint8_t *object, const void *root,
                const Codec &codec);

void WriteElement(Bytes &out, const Field &field, const std::uint8_t *element, const void *root,
                  const Codec &codec) {
	switch (field.encoding) {
	case Encoding::Struct:
		WriteTable(out, *field.table, element, root, codec);
		break;

	case Encoding::Bool:
		out.push_back(*element ? 1 : 0);
		break;

	case Encoding::Index: {
		const void *pointer = nullptr;
		std::memcpy(&pointer, element, sizeof(pointer));
		const std::int32_t index = codec.indexOf(codec.context, field.kind, pointer, root);
		PutBytes(out, static_cast<std::uint32_t>(index), 4);
		break;
	}

	case Encoding::String: {
		const char *text = nullptr;
		std::memcpy(&text, element, sizeof(text));
		if (!text) {
			PutBytes(out, static_cast<std::uint32_t>(-1), 4);
		} else {
			const std::size_t length = std::strlen(text);
			PutBytes(out, length, 4);
			out.insert(out.end(), text, text + length);
		}
		break;
	}

	case Encoding::Present: {
		const void *pointer = nullptr;
		std::memcpy(&pointer, element, sizeof(pointer));
		out.push_back(pointer ? 1 : 0);
		break;
	}

	case Encoding::Omitted:
		break;

	default: {
		// integers and floating-point numbers go out as the bytes they are
		const std::size_t width = Width(field.encoding);
		PutBytes(out, LoadInteger(element, width, false), width);
		break;
	}
	}
}

void WriteTable(Bytes &out, const Table &table, const std::uint8_t *object, const void *root,
                const Codec &codec) {
	for (const Field &field : table.fields) {
		if (field.encoding == Encoding::Omitted) {
			continue;
		}
		for (std::uint32_t i = 0; i < field.count; i++) {
			WriteElement(out, field, object + field.offset + i * field.elementSize, root, codec);
		}
	}
}

void CollectTables(const Table &table, std::vector<const Table *> &all) {
	if (std::find(all.begin(), all.end(), &table) != all.end()) {
		return;
	}
	all.push_back(&table);
	for (const Field &field : table.fields) {
		if (field.encoding == Encoding::Struct) {
			CollectTables(*field.table, all);
		}
	}
}

} // namespace

void Write(Bytes &out, const Table &table, const void *object, const Codec &codec) {
	WriteTable(out, table, static_cast<const std::uint8_t *>(object), object, codec);
}

void WriteSchema(Bytes &out, std::span<const Table *const> tables) {
	std::vector<const Table *> all;
	for (const Table *table : tables) {
		CollectTables(*table, all);
	}

	PutBytes(out, all.size(), 4);
	for (const Table *table : all) {
		std::uint32_t written = 0;
		for (const Field &field : table->fields) {
			written += field.encoding != Encoding::Omitted ? 1u : 0u;
		}

		PutName(out, table->name);
		PutBytes(out, written, 4);
		for (const Field &field : table->fields) {
			if (field.encoding == Encoding::Omitted) {
				continue;
			}
			PutName(out, field.name);
			PutBytes(out, static_cast<std::uint8_t>(field.encoding), 1);
			PutBytes(out, field.count, 4);
			if (field.encoding == Encoding::Struct) {
				PutName(out, field.table->name);
			} else if (field.encoding == Encoding::Index) {
				PutName(out, field.kind);
			}
		}
	}
}

bool Schema::Parse(View data, std::string &error) {
	tables_.clear();
	error = "the description of the saved structures is damaged";

	std::uint64_t tableCount = 0;
	if (!TakeBytes(data, 4, tableCount)) {
		return false;
	}
	for (std::uint64_t t = 0; t < tableCount; t++) {
		FileTable table;
		std::uint64_t fieldCount = 0;
		if (!TakeName(data, table.name) || !TakeBytes(data, 4, fieldCount)) {
			return false;
		}
		for (std::uint64_t f = 0; f < fieldCount; f++) {
			FileField field;
			std::uint64_t encoding = 0;
			std::uint64_t count = 0;
			if (!TakeName(data, field.name) || !TakeBytes(data, 1, encoding) ||
			    !TakeBytes(data, 4, count) || !IsKnown(encoding)) {
				return false;
			}
			field.encoding = static_cast<Encoding>(encoding);
			field.count = static_cast<std::uint32_t>(count);
			if ((field.encoding == Encoding::Struct || field.encoding == Encoding::Index) &&
			    !TakeName(data, field.reference)) {
				return false;
			}
			table.fields.push_back(std::move(field));
		}
		tables_.push_back(std::move(table));
	}

	// every structure that is referred to has to be described
	for (const FileTable &table : tables_) {
		for (const FileField &field : table.fields) {
			if (field.encoding == Encoding::Struct && !Find(field.reference)) {
				return false;
			}
		}
	}

	error.clear();
	return true;
}

const Schema::FileTable *Schema::Find(std::string_view name) const {
	for (const FileTable &table : tables_) {
		if (table.name == name) {
			return &table;
		}
	}
	return nullptr;
}

bool Schema::Read(View &data, const Table &table, void *object, const void *original,
                  const Codec &codec, std::string &error) const {
	const FileTable *fileTable = Find(table.name);
	if (!fileTable) {
		error = std::string("the save does not describe ") + table.name;
		return false;
	}
	return ReadTable(data, *fileTable, &table, static_cast<std::uint8_t *>(object),
	                 static_cast<const std::uint8_t *>(original), codec, error, 0);
}

// Reads a structure as the file describes it. `table` is the structure it goes into, or
// null when the reader has no use for it and it is only skipped.
bool Schema::ReadTable(View &data, const FileTable &fileTable, const Table *table,
                       std::uint8_t *object, const std::uint8_t *original, const Codec &codec,
                       std::string &error, int depth) const {
	if (depth > maxDepth) {
		error = "the saved structures are nested too deeply";
		return false;
	}

	for (const FileField &fileField : fileTable.fields) {
		const Field *field = nullptr;
		if (table) {
			for (const Field &candidate : table->fields) {
				if (candidate.encoding != Encoding::Omitted && fileField.name == candidate.name) {
					field = &candidate;
					break;
				}
			}
		}
		if (!ReadField(data, fileField, field, object, original, codec, error, depth)) {
			if (error.empty()) {
				error = "the save ends inside " + fileTable.name + "." + fileField.name;
			}
			return false;
		}
	}
	return true;
}

// Reads the elements of one member as the file has them. `field` is the member they go
// into, or null. Elements beyond the member's count are skipped, as is a member whose
// kind of value has changed.
bool Schema::ReadField(View &data, const FileField &fileField, const Field *field,
                       std::uint8_t *object, const std::uint8_t *original, const Codec &codec,
                       std::string &error, int depth) const {
	const Encoding from = fileField.encoding;

	const FileTable *fileStruct = nullptr;
	if (from == Encoding::Struct) {
		fileStruct = Find(fileField.reference);
	}

	// Whether the value in the file can go into the member at all.
	bool usable = field != nullptr;
	if (usable) {
		const Encoding to = field->encoding;
		if (IsInteger(from)) {
			usable = IsInteger(to);
		} else if (IsFloat(from)) {
			usable = IsFloat(to);
		} else if (from == Encoding::Struct) {
			usable = to == Encoding::Struct && fileField.reference == field->table->name;
		} else if (from == Encoding::Index) {
			usable = to == Encoding::Index && fileField.reference == field->kind;
		} else {
			usable = from == to;
		}
	}

	for (std::uint32_t i = 0; i < fileField.count; i++) {
		const bool store = usable && i < field->count;
		std::uint8_t *element = store ? object + field->offset + i * field->elementSize : nullptr;

		switch (from) {
		case Encoding::Struct: {
			const std::uint8_t *originalElement =
			    store && original ? original + field->offset + i * field->elementSize : nullptr;
			if (!ReadTable(data, *fileStruct, store ? field->table : nullptr, element,
			               originalElement, codec, error, depth + 1)) {
				return false;
			}
			break;
		}

		case Encoding::String: {
			std::uint64_t length = 0;
			if (!TakeBytes(data, 4, length)) {
				return false;
			}
			char *text = nullptr;
			if (length != 0xffffffffu) {
				if (data.size() < length) {
					return false;
				}
				if (store) {
					char *reusable = nullptr;
					if (original) {
						std::memcpy(&reusable, original + field->offset + i * field->elementSize,
						            sizeof(reusable));
					}
					text = codec.newString(
					    codec.context,
					    std::string_view(reinterpret_cast<const char *>(data.data()),
						                 static_cast<std::size_t>(length)),
					    reusable);
				}
				data = data.subspan(static_cast<std::size_t>(length));
			}
			if (store) {
				std::memcpy(element, &text, sizeof(text));
			}
			break;
		}

		default: {
			const std::size_t width = Width(from);
			std::uint64_t value = 0;
			if (!TakeBytes(data, width, value)) {
				return false;
			}
			if (!store) {
				break;
			}

			if (from == Encoding::Index) {
				std::uint64_t index = value;
				SignExtend(index, 4);
				void *pointer =
				    codec.pointerOf(codec.context, field->kind, static_cast<std::int32_t>(index));
				std::memcpy(element, &pointer, sizeof(pointer));
			} else if (from == Encoding::Present) {
				void *pointer = value ? codec.present : nullptr;
				std::memcpy(element, &pointer, sizeof(pointer));
			} else if (from == Encoding::Bool) {
				*element = value ? 1 : 0;
			} else if (IsInteger(from)) {
				if (IsSigned(from)) {
					SignExtend(value, width);
				}
				StoreInteger(element, field->elementSize, value);
			} else if (from == field->encoding) {
				StoreInteger(element, field->elementSize, value); // the bytes of a float
			} else if (from == Encoding::F32) {
				float narrow;
				const std::uint32_t bits = static_cast<std::uint32_t>(value);
				std::memcpy(&narrow, &bits, sizeof(narrow));
				const double wide = narrow;
				std::memcpy(element, &wide, sizeof(wide));
			} else {
				double wide;
				std::memcpy(&wide, &value, sizeof(wide));
				const float narrow = static_cast<float>(wide);
				std::memcpy(element, &narrow, sizeof(narrow));
			}
			break;
		}
		}
	}
	return true;
}

std::string CheckLayout(const Table &table) {
	std::vector<const Field *> fields;
	for (const Field &field : table.fields) {
		fields.push_back(&field);
	}
	std::sort(fields.begin(), fields.end(),
	          [](const Field *a, const Field *b) { return a->offset < b->offset; });

	const std::string prefix = std::string(table.name) + ": ";

	std::uint32_t end = 0; // of the members seen so far
	const Field *previous = nullptr;
	for (const Field *field : fields) {
		// In a packed structure members are aligned to the structure's alignment at most.
		const std::uint32_t alignment = std::min(field->elementAlign, table.align);
		const std::uint32_t expected = AlignUp(end, alignment);

		if (field->offset < end) {
			return prefix + field->name + " overlaps " + (previous ? previous->name : "?");
		}
		if (field->offset != expected) {
			return prefix + std::to_string(field->offset - end) + " bytes before " + field->name +
			       " at offset " + std::to_string(end) + " belong to no member of the table";
		}
		end = field->offset + field->elementSize * field->count;
		previous = field;
	}

	if (AlignUp(end, table.align) != table.size) {
		return prefix + std::to_string(table.size - end) + " bytes at the end, from offset " +
		       std::to_string(end) + ", belong to no member of the table";
	}
	return {};
}

} // namespace sg
