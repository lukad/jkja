// JPEG and PNG decoding and JPEG encoding through the stb libraries
// (docs/decisions/0012-dependencies.md). Every image is made by the test.

#include "engine_stubs.h"

#include "../src/game/q_shared.h"
#include "../src/qcommon/qcommon.h"
#include "../src/renderer/tr_stb.h"

#include <catch2/catch_test_macros.hpp>
#include <miniz.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace {

void AppendBigEndian(std::vector<unsigned char> &out, unsigned int value) {
	out.push_back(static_cast<unsigned char>(value >> 24));
	out.push_back(static_cast<unsigned char>(value >> 16));
	out.push_back(static_cast<unsigned char>(value >> 8));
	out.push_back(static_cast<unsigned char>(value));
}

void AppendChunk(std::vector<unsigned char> &png, const char *type,
                 const std::vector<unsigned char> &data) {
	AppendBigEndian(png, static_cast<unsigned int>(data.size()));

	std::vector<unsigned char> typeAndData(type, type + 4);
	typeAndData.insert(typeAndData.end(), data.begin(), data.end());
	png.insert(png.end(), typeAndData.begin(), typeAndData.end());

	AppendBigEndian(png, static_cast<unsigned int>(
	                         mz_crc32(MZ_CRC32_INIT, typeAndData.data(), typeAndData.size())));
}

// A PNG file with 8 bits per channel: colour type 0 is grey, 6 is RGBA.
std::vector<unsigned char> MakePng(int width, int height, int colourType,
                                   const std::vector<unsigned char> &pixels) {
	const int channels = colourType == 6 ? 4 : 1;

	std::vector<unsigned char> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};

	std::vector<unsigned char> header;
	AppendBigEndian(header, static_cast<unsigned int>(width));
	AppendBigEndian(header, static_cast<unsigned int>(height));
	header.push_back(8);                                      // bit depth
	header.push_back(static_cast<unsigned char>(colourType)); // colour type
	header.push_back(0);                                      // compression: deflate
	header.push_back(0);                                      // filter method
	header.push_back(0);                                      // not interlaced
	AppendChunk(png, "IHDR", header);

	// each row is preceded by its filter type; 0 is "none"
	std::vector<unsigned char> rows;
	for (int y = 0; y < height; y++) {
		rows.push_back(0);
		const auto rowStart = pixels.begin() + y * width * channels;
		rows.insert(rows.end(), rowStart, rowStart + width * channels);
	}

	mz_ulong compressedSize = mz_compressBound(static_cast<mz_ulong>(rows.size()));
	std::vector<unsigned char> compressed(compressedSize);
	REQUIRE(mz_compress(compressed.data(), &compressedSize, rows.data(),
	                    static_cast<mz_ulong>(rows.size())) == MZ_OK);
	compressed.resize(compressedSize);
	AppendChunk(png, "IDAT", compressed);

	AppendChunk(png, "IEND", {});
	return png;
}

} // namespace

TEST_CASE("an 8-bit grey PNG is read back exactly") {
	stubs::Reset();

	const int width = 16;
	const int height = 9;
	std::vector<unsigned char> grey(static_cast<std::size_t>(width * height));
	for (std::size_t i = 0; i < grey.size(); i++) {
		grey[i] = static_cast<unsigned char>(i * 7);
	}
	stubs::Files()["heightmap.png"] = MakePng(width, height, 0, grey);

	unsigned char *pic = NULL;
	int w = 0;
	int h = 0;
	LoadPNG8("heightmap.png", &pic, &w, &h);

	REQUIRE(pic != NULL);
	CHECK(w == width);
	CHECK(h == height);
	CHECK(std::vector<unsigned char>(pic, pic + width * height) == grey);
	CHECK(stubs::Printed().empty());
	Z_Free(pic);
}

TEST_CASE("an RGBA PNG is read back exactly") {
	stubs::Reset();

	const int width = 5;
	const int height = 4;
	std::vector<unsigned char> rgba(static_cast<std::size_t>(width * height * 4));
	for (std::size_t i = 0; i < rgba.size(); i++) {
		rgba[i] = static_cast<unsigned char>(i * 13 + 1);
	}
	stubs::Files()["textures/test.png"] = MakePng(width, height, 6, rgba);

	unsigned char *pic = NULL;
	int w = 0;
	int h = 0;
	LoadPNG32("textures/test.png", &pic, &w, &h);

	REQUIRE(pic != NULL);
	CHECK(w == width);
	CHECK(h == height);
	CHECK(std::vector<unsigned char>(pic, pic + width * height * 4) == rgba);
	Z_Free(pic);
}

TEST_CASE("a JPEG survives encoding and decoding within the loss of the format") {
	stubs::Reset();

	const int width = 64;
	const int height = 48;
	std::vector<unsigned char> rgba(static_cast<std::size_t>(width * height * 4));
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			unsigned char *pixel = &rgba[static_cast<std::size_t>((y * width + x) * 4)];
			pixel[0] = static_cast<unsigned char>(x * 4);
			pixel[1] = static_cast<unsigned char>(y * 5);
			pixel[2] = static_cast<unsigned char>(128 + x - y);
			pixel[3] = 17; // alpha is not stored in a JPEG
		}
	}

	int size = 0;
	unsigned char *jpeg = Compress_JPG(&size, 95, width, height, rgba.data(), qfalse);
	REQUIRE(jpeg != NULL);
	REQUIRE(size > 100);
	CHECK(jpeg[0] == 0xff);
	CHECK(jpeg[1] == 0xd8);

	SECTION("decoded from memory") {
		unsigned char *pic = NULL;
		int w = 0;
		int h = 0;
		Decompress_JPG("test", jpeg, size, &pic, &w, &h);

		REQUIRE(pic != NULL);
		REQUIRE(w == width);
		REQUIRE(h == height);
		int worst = 0;
		for (int i = 0; i < width * height; i++) {
			for (int c = 0; c < 3; c++) {
				const int difference =
				    std::abs(pic[i * 4 + c] - rgba[static_cast<std::size_t>(i * 4 + c)]);
				worst = difference > worst ? difference : worst;
			}
			REQUIRE(pic[i * 4 + 3] == 255);
		}
		CHECK(worst <= 12);
		Z_Free(pic);
	}

	SECTION("decoded from a file") {
		stubs::Files()["textures/test.jpg"].assign(jpeg, jpeg + size);

		unsigned char *pic = NULL;
		int w = 0;
		int h = 0;
		LoadJPG("textures/test.jpg", &pic, &w, &h);

		REQUIRE(pic != NULL);
		CHECK(w == width);
		CHECK(h == height);
		Z_Free(pic);
	}

	Z_Free(jpeg);
}

TEST_CASE("bottom-up pixels are flipped when a screenshot is written") {
	stubs::Reset();

	// top half black, bottom half white, stored bottom row first as OpenGL returns it
	const int width = 16;
	const int height = 16;
	std::vector<unsigned char> bottomUp(static_cast<std::size_t>(width * height * 4), 255);
	for (int y = height / 2; y < height; y++) {
		for (int x = 0; x < width * 4; x++) {
			bottomUp[static_cast<std::size_t>(y * width * 4 + x)] = (x % 4 == 3) ? 255 : 0;
		}
	}

	SaveJPG("screenshots/shot0000.jpg", 95, width, height, bottomUp.data());
	REQUIRE(stubs::Files().count("screenshots/shot0000.jpg") == 1);

	unsigned char *pic = NULL;
	int w = 0;
	int h = 0;
	LoadJPG("screenshots/shot0000.jpg", &pic, &w, &h);
	REQUIRE(pic != NULL);
	CHECK(pic[0] < 40);                         // first row of the file: black
	CHECK(pic[(height - 1) * width * 4] > 215); // last row: white
	Z_Free(pic);
}

TEST_CASE("a missing image is not an error; a broken one is reported") {
	stubs::Reset();

	unsigned char *pic = reinterpret_cast<unsigned char *>(1);
	int w = -1;
	int h = -1;
	LoadJPG("textures/nothere.jpg", &pic, &w, &h);
	CHECK(pic == NULL);
	CHECK(stubs::Printed().empty());

	stubs::Files()["textures/broken.jpg"] = {0xff, 0xd8, 0xff, 0xe0, 1, 2, 3, 4, 5};
	LoadJPG("textures/broken.jpg", &pic, &w, &h);
	CHECK(pic == NULL);
	CHECK(w == 0);
	CHECK(h == 0);
	CHECK(stubs::Printed().find("textures/broken.jpg") != std::string::npos);
}
