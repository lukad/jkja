// Stand-ins for the engine services that the code under test calls: memory comes from
// the C library, files live in a map, messages are collected, and an error is an
// exception.

#include "engine_stubs.h"

#include "../src/game/q_shared.h"
#include "../src/qcommon/qcommon.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace stubs {

std::map<std::string, std::vector<unsigned char>> &Files() {
	static std::map<std::string, std::vector<unsigned char>> files;
	return files;
}

std::string &Printed() {
	static std::string printed;
	return printed;
}

void Reset() {
	Files().clear();
	Printed().clear();
}

} // namespace stubs

void QDECL Com_Printf(const char *msg, ...) {
	char text[4096];

	va_list arguments;
	va_start(arguments, msg);
	std::vsnprintf(text, sizeof(text), msg, arguments);
	va_end(arguments);

	stubs::Printed() += text;
}

void QDECL Com_Error(int, const char *error, ...) {
	char text[4096];

	va_list arguments;
	va_start(arguments, error);
	std::vsnprintf(text, sizeof(text), error, arguments);
	va_end(arguments);

	throw std::runtime_error(text);
}

void *Z_Malloc(int iSize, memtag_t, qboolean bZeroit, int) {
	void *memory = bZeroit ? std::calloc(1, static_cast<std::size_t>(iSize))
	                       : std::malloc(static_cast<std::size_t>(iSize));
	if (!memory) {
		throw std::bad_alloc();
	}
	return memory;
}

int Z_Free(void *ptr) {
	std::free(ptr);
	return 0;
}

int FS_ReadFile(const char *qpath, void **buffer) {
	const auto found = stubs::Files().find(qpath);
	if (found == stubs::Files().end()) {
		if (buffer) {
			*buffer = NULL;
		}
		return -1;
	}

	const std::vector<unsigned char> &content = found->second;
	if (buffer) {
		// like the engine, with a terminating zero after the content
		auto *copy = static_cast<unsigned char *>(std::malloc(content.size() + 1));
		std::memcpy(copy, content.data(), content.size());
		copy[content.size()] = 0;
		*buffer = copy;
	}
	return static_cast<int>(content.size());
}

void FS_FreeFile(void *buffer) {
	std::free(buffer);
}

void FS_WriteFile(const char *qpath, const void *buffer, int size) {
	const auto *bytes = static_cast<const unsigned char *>(buffer);
	stubs::Files()[qpath].assign(bytes, bytes + size);
}
