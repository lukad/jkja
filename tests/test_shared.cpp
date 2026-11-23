// The shared helpers that replaced compiler-specific library functions and assembly.

#include "../src/game/q_shared.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <cstring>

TEST_CASE("case-insensitive comparison orders strings as the original C runtime did") {
	CHECK(Q_stricmp("Kyle", "kYLE") == 0);
	CHECK(Q_stricmp("abc", "abd") < 0);
	CHECK(Q_stricmp("abd", "ABC") > 0);
	CHECK(Q_stricmp("abc", "abcd") < 0);

	// Letters are folded to lower case, so '_' (95) sorts before 'A' and 'a' (97).
	CHECK(Q_stricmp("_", "A") < 0);
	CHECK(Q_stricmp("A", "_") > 0);

	// Bytes above 127 compare as unsigned values and are not folded.
	CHECK(Q_stricmp("\xe9", "z") > 0);
}

TEST_CASE("bounded case-insensitive comparison stops at the given length") {
	CHECK(Q_stricmpn("models/players/kyle", "MODELS/PLAYERS/luke", 15) == 0);
	CHECK(Q_stricmpn("models/players/kyle", "MODELS/PLAYERS/luke", 16) < 0);
	CHECK(Q_stricmpn("abc", "xyz", 0) == 0);
}

TEST_CASE("case conversion works in place and returns its argument") {
	char text[] = "Jedi_Academy 2003";

	CHECK(Q_strlwr(text) == text);
	CHECK(std::strcmp(text, "jedi_academy 2003") == 0);
	CHECK(Q_strupr(text) == text);
	CHECK(std::strcmp(text, "JEDI_ACADEMY 2003") == 0);
}

TEST_CASE("random helpers stay in range whatever RAND_MAX is") {
	std::srand(1001);

	for (int i = 0; i < 20000; i++) {
		const float unit = random();
		REQUIRE(unit >= 0.0f);
		REQUIRE(unit <= 1.0f);

		const int die = Q_irand(1, 6);
		REQUIRE(die >= 1);
		REQUIRE(die <= 6);

		const float spread = Q_flrand(-2.0f, 3.0f);
		REQUIRE(spread >= -2.0f);
		REQUIRE(spread < 3.0f);
	}
}

namespace {

cplane_t Plane(float x, float y, float z, float dist, int type) {
	cplane_t plane;
	std::memset(&plane, 0, sizeof(plane));
	plane.normal[0] = x;
	plane.normal[1] = y;
	plane.normal[2] = z;
	plane.dist = dist;
	plane.type = static_cast<byte>(type);
	plane.signbits = static_cast<byte>((x < 0 ? 1 : 0) | (y < 0 ? 2 : 0) | (z < 0 ? 4 : 0));
	return plane;
}

} // namespace

TEST_CASE("BoxOnPlaneSide reports front, back and crossing") {
	vec3_t mins = {-1, -1, -1};
	vec3_t maxs = {1, 1, 1};

	SECTION("axial plane") {
		cplane_t behindBox = Plane(0, 0, 1, -5, PLANE_Z);
		cplane_t throughBox = Plane(0, 0, 1, 0, PLANE_Z);
		cplane_t beyondBox = Plane(0, 0, 1, 5, PLANE_Z);

		CHECK(BoxOnPlaneSide(mins, maxs, &behindBox) == 1);
		CHECK(BoxOnPlaneSide(mins, maxs, &throughBox) == 3);
		CHECK(BoxOnPlaneSide(mins, maxs, &beyondBox) == 2);
	}

	SECTION("planes of every sign combination") {
		const float n = 0.57735026f; // 1 / sqrt(3)
		for (int signs = 0; signs < 8; signs++) {
			const float x = (signs & 1) ? -n : n;
			const float y = (signs & 2) ? -n : n;
			const float z = (signs & 4) ? -n : n;

			// The box reaches sqrt(3) along the normal in both directions.
			cplane_t behindBox = Plane(x, y, z, -2.0f, PLANE_NON_AXIAL);
			cplane_t throughBox = Plane(x, y, z, 0.5f, PLANE_NON_AXIAL);
			cplane_t beyondBox = Plane(x, y, z, 2.0f, PLANE_NON_AXIAL);

			CHECK(BoxOnPlaneSide(mins, maxs, &behindBox) == 1);
			CHECK(BoxOnPlaneSide(mins, maxs, &throughBox) == 3);
			CHECK(BoxOnPlaneSide(mins, maxs, &beyondBox) == 2);
		}
	}
}
