// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "reader/resources.h"

namespace finale_mus_reader {
namespace detail {

ReaderResources prepareReaderResources(const ReaderOptions &options,
                                       XmlParser parseXml) {
    ReaderResources result;
    result.symbolFontNames = text::parseMacSymbolFonts(options.macSymbolFonts);
    result.percussionMappings =
        percussion::parseMappingTables(options.percussionMappingXml, parseXml);
    return result;
}

} // namespace detail
} // namespace finale_mus_reader
