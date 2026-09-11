// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include "coverage/support/companion_path.h"

namespace finale_mus_reader_tests {
namespace {

TEST_CASE("Coverage companion names preserve dots in extensionless source names", "[coverage]")
{
    using finale_mus_reader::coverage::companionBaseNameFor;

    CHECK(companionBaseNameFor(std::filesystem::path("Score.mus")) == "Score");
    CHECK(companionBaseNameFor(std::filesystem::path("Score.MUS")) == "Score");
    CHECK(companionBaseNameFor(std::filesystem::path("Score.96:09:20")) == "Score.96:09:20");
    CHECK(companionBaseNameFor(std::filesystem::path("Score")) == "Score");
}

TEST_CASE("Coverage companion names distinguish colliding source names", "[coverage]")
{
    using finale_mus_reader::coverage::companionBaseNameFor;
    using finale_mus_reader::coverage::companionNameConflictFor;

    CHECK(companionBaseNameFor(std::filesystem::path("Score"), true) ==
          "Score.from-no-extension");
    CHECK(companionBaseNameFor(std::filesystem::path("Score.mus"), true) == "Score.from-mus");
    CHECK(companionBaseNameFor(std::filesystem::path("Score.3"), true) ==
          "Score.3.from-no-extension");
    CHECK(companionBaseNameFor(std::filesystem::path("Score.3.MUS"), true) ==
          "Score.3.from-mus");
    CHECK(companionNameConflictFor(std::filesystem::path("dir/Score")) ==
          std::filesystem::path("dir/Score.mus"));
    CHECK(companionNameConflictFor(std::filesystem::path("dir/Score.3.mus")) ==
          std::filesystem::path("dir/Score.3"));
}

} // namespace
} // namespace finale_mus_reader_tests
