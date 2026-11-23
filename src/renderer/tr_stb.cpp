// JPEG and PNG images, decoded and encoded with the stb libraries
// (docs/decisions/0012-dependencies.md).

#include "tr_stb.h"

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"

#include <cstring>
#include <vector>

// The one place where the two stb libraries are compiled. They get no file access of
// their own; files are read and written by the engine.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "../../external/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STBI_WRITE_NO_STDIO
#include "../../external/stb/stb_image_write.h"

namespace {

void Decode(const char *name, const unsigned char *data, int length, int components, memtag_t tag,
            unsigned char **pic, int *width, int *height) {
	*pic = NULL;
	if (width) {
		*width = 0;
	}
	if (height) {
		*height = 0;
	}

	int w = 0;
	int h = 0;
	int componentsInFile = 0;
	stbi_uc *decoded = stbi_load_from_memory(data, length, &w, &h, &componentsInFile, components);
	if (!decoded) {
		Com_Printf(S_COLOR_YELLOW "WARNING: %s could not be read: %s\n", name,
		           stbi_failure_reason());
		return;
	}

	// The engine frees images with Z_Free, so the pixels move to memory it owns.
	const int size = w * h * components;
	*pic = static_cast<unsigned char *>(Z_Malloc(size, tag, qfalse));
	std::memcpy(*pic, decoded, static_cast<std::size_t>(size));
	stbi_image_free(decoded);

	if (width) {
		*width = w;
	}
	if (height) {
		*height = h;
	}
}

void Load(const char *filename, int components, memtag_t tag, unsigned char **pic, int *width,
          int *height) {
	*pic = NULL;

	void *file = NULL;
	const int length = FS_ReadFile(filename, &file);
	if (!file) {
		return;
	}
	Decode(filename, static_cast<const unsigned char *>(file), length, components, tag, pic, width,
	       height);
	FS_FreeFile(file);
}

void AppendToVector(void *context, void *data, int size) {
	auto *out = static_cast<std::vector<unsigned char> *>(context);
	const auto *bytes = static_cast<const unsigned char *>(data);
	out->insert(out->end(), bytes, bytes + size);
}

} // namespace

void LoadJPG(const char *filename, unsigned char **pic, int *width, int *height) {
	Load(filename, 4, TAG_TEMP_JPG, pic, width, height);
}

void LoadPNG32(const char *filename, unsigned char **pic, int *width, int *height) {
	Load(filename, 4, TAG_TEMP_PNG, pic, width, height);
}

void LoadPNG8(const char *filename, unsigned char **pic, int *width, int *height) {
	Load(filename, 1, TAG_TEMP_PNG, pic, width, height);
}

void Decompress_JPG(const char *name, const unsigned char *data, int length, unsigned char **pic,
                    int *width, int *height) {
	Decode(name, data, length, 4, TAG_TEMP_JPG, pic, width, height);
}

unsigned char *Compress_JPG(int *outputSize, int quality, int image_width, int image_height,
                            unsigned char *image_buffer, int invert) {
	std::vector<unsigned char> encoded;

	stbi_flip_vertically_on_write(invert ? 1 : 0);
	const int written = stbi_write_jpg_to_func(AppendToVector, &encoded, image_width, image_height,
	                                           4, image_buffer, quality);
	stbi_flip_vertically_on_write(0);

	*outputSize = 0;
	if (!written || encoded.empty()) {
		return NULL;
	}

	const int size = static_cast<int>(encoded.size());
	auto *out = static_cast<unsigned char *>(Z_Malloc(size, TAG_TEMP_JPG, qfalse));
	std::memcpy(out, encoded.data(), encoded.size());
	*outputSize = size;
	return out;
}

void SaveJPG(const char *filename, int quality, int image_width, int image_height,
             unsigned char *image_buffer) {
	int size = 0;
	unsigned char *encoded =
	    Compress_JPG(&size, quality, image_width, image_height, image_buffer, qtrue);
	if (!encoded) {
		Com_Printf(S_COLOR_YELLOW "WARNING: %s could not be written\n", filename);
		return;
	}
	FS_WriteFile(filename, encoded, size);
	Z_Free(encoded);
}
