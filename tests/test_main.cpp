#include <catch2/catch_test_macros.hpp>

// Placeholder test to verify the test framework is working
TEST_CASE("Basic test framework verification", "[framework]") {
    REQUIRE(true);
    REQUIRE(1 + 1 == 2);
}

TEST_CASE("String operations", "[basic]") {
    std::string test = "Kitchen CAD Designer";
    REQUIRE(test.length() > 0);
    REQUIRE(test.find("Kitchen") != std::string::npos);
    REQUIRE(test.find("CAD") != std::string::npos);
    REQUIRE(test.find("Designer") != std::string::npos);
}