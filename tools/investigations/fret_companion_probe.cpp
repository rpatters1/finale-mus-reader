// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "musx_companion.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

constexpr std::array<std::string_view, 4> fretElementNames = {
    "fretGroup",
    "fretInst",
    "fretStyle",
    "fretboard",
};

void printElements(std::string_view xml, std::string_view elementName)
{
    const auto opening = "<" + std::string(elementName) + " ";
    const auto closing = "</" + std::string(elementName) + ">";
    std::size_t pos = 0;
    while ((pos = xml.find(opening, pos)) != std::string_view::npos) {
        const auto openingEnd = xml.find('>', pos);
        const auto pairedClosing = xml.find(closing, openingEnd);
        const auto end = openingEnd > pos && xml[openingEnd - 1] == '/'
            ? openingEnd + 1
            : pairedClosing + closing.size();
        if (end <= xml.size()) {
            std::cout << xml.substr(pos, end - pos) << '\n';
        }
        pos = end;
    }
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: fret_companion_probe FILE.musx [ELEMENT]\n";
        return 2;
    }

    const auto archive = finale_mus_reader::coverage::readCompanionArchive(
        std::filesystem::path(argv[1]));
    const std::string_view xml(reinterpret_cast<const char*>(archive.enigmaXml.data()),
        archive.enigmaXml.size());
    for (const auto elementName : fretElementNames) {
        if (argc == 3 && elementName != argv[2]) {
            continue;
        }
        printElements(xml, elementName);
    }
}
