// Reading .pk3 archives: the zip reader on top of miniz
// (docs/decisions/0012-dependencies.md).

#include "../src/game/q_shared.h"
#include "../src/qcommon/unzip.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>
#include <vector>

namespace {

// Bytes that deflate well but are not a trivial run.
std::vector<unsigned char> Pattern(std::size_t size) {
	std::vector<unsigned char> data(size);
	unsigned int state = 12345;
	for (std::size_t i = 0; i < size; i++) {
		state = state * 1103515245u + 12345u;
		data[i] = static_cast<unsigned char>((i / 64) ^ ((state >> 16) & 3));
	}
	return data;
}

std::vector<unsigned char> ReadEntry(unzFile archive, const char *name, unsigned chunk,
                                     int *closeResult) {
	std::vector<unsigned char> content;

	REQUIRE(unzLocateFile(archive, name, 2) == UNZ_OK);
	REQUIRE(unzOpenCurrentFile(archive) == UNZ_OK);

	std::vector<unsigned char> buffer(chunk);
	for (;;) {
		const int got = unzReadCurrentFile(archive, buffer.data(), chunk);
		REQUIRE(got >= 0);
		if (got == 0) {
			break;
		}
		content.insert(content.end(), buffer.begin(), buffer.begin() + got);
	}

	// Closing compares the CRC-32 of what was read with the one in the archive.
	*closeResult = unzCloseCurrentFile(archive);
	return content;
}

} // namespace

TEST_CASE("CRC-32 is the standard one") {
	const char *check = "123456789";
	CHECK(mz_crc32(MZ_CRC32_INIT, reinterpret_cast<const unsigned char *>(check), 9) ==
	      0xCBF43926u);
}

TEST_CASE("the zip reader inflates deflated entries and copies stored ones") {
	const std::string path = std::string(JASP_TEST_SCRATCH_DIR) + "/test_zip.pk3";
	// larger than the reader's 64 KB input buffer, so that it has to refill it
	const std::vector<unsigned char> large = Pattern(300000);
	const std::string small = "seta r_mode \"4\"\n";

	{
		mz_zip_archive writer;
		std::memset(&writer, 0, sizeof(writer));
		REQUIRE(mz_zip_writer_init_file(&writer, path.c_str(), 0));
		REQUIRE(mz_zip_writer_add_mem(&writer, "maps/large.bin", large.data(), large.size(),
		                              MZ_BEST_COMPRESSION));
		REQUIRE(mz_zip_writer_add_mem(&writer, "stored.cfg", small.data(), small.size(),
		                              MZ_NO_COMPRESSION));
		REQUIRE(mz_zip_writer_finalize_archive(&writer));
		REQUIRE(mz_zip_writer_end(&writer));
	}

	unzFile archive = unzOpen(path.c_str());
	REQUIRE(archive != NULL);
	// as the engine does after opening an archive; nothing else selects an entry
	REQUIRE(unzGoToFirstFile(archive) == UNZ_OK);

	unz_global_info info;
	REQUIRE(unzGetGlobalInfo(archive, &info) == UNZ_OK);
	CHECK(info.number_entry == 2);

	SECTION("deflated entry, read in small pieces") {
		int closeResult = -1;
		const std::vector<unsigned char> content =
		    ReadEntry(archive, "maps/large.bin", 1000, &closeResult);
		CHECK(content == large);
		CHECK(closeResult == UNZ_OK);
	}

	SECTION("deflated entry, read in one piece") {
		int closeResult = -1;
		const std::vector<unsigned char> content =
		    ReadEntry(archive, "maps/large.bin", static_cast<unsigned>(large.size()), &closeResult);
		CHECK(content == large);
		CHECK(closeResult == UNZ_OK);
	}

	SECTION("stored entry, found without regard to case") {
		int closeResult = -1;
		const std::vector<unsigned char> content =
		    ReadEntry(archive, "STORED.CFG", 5, &closeResult);
		CHECK(std::string(content.begin(), content.end()) == small);
		CHECK(closeResult == UNZ_OK);
	}

	CHECK(unzClose(archive) == UNZ_OK);
}
