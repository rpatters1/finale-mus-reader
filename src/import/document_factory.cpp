// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/document_factory.h"

#include <memory>
#include <utility>

#include "defaults/default_document.h"
#include "import/header/header.h"
#include "import/legacy_mapping.h"
#include "records/legacy_record_index.h"
#include "musx/factory/DocumentFactory.h"
#include "musx/factory/PoolFactory.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace {

// Seeds the Finale 27 options pool, excluding FontOptions, plus the four option-like layer
// attributes. FontOptions is reconstructed only from tuples physically present in the MUS
// file; leaving it absent is safer than retaining baseline ids from another font table.
//
// The remaining seeded options reference font and shape records by cmper, and those records
// are deliberately not seeded: a document has one id space per record type, so a
// pinned definition would later collide with the source record sharing its cmper.
// Those references therefore do not resolve, and font lookups throw until the
// definitions are decoded from the MUS file. See research/PRODUCTION_READINESS.md.
void seedPinnedDefaults(
    const musx::dom::DocumentPtr& document,
    const defaults::ParsedDefaultDocument& pinned, ImportReport& report)
{
    report.defaultsPlatform = pinned.platform;
    document->getOptions() = musx::factory::OptionsFactory::create(
        pinned.options, document, pinned.optionsFilter);
    document->getOthers() = musx::factory::OthersFactory::create(
        pinned.others, document, pinned.optionLikeOthersFilter);
}

} // namespace

bool hasBanner(const std::uint8_t* data, std::size_t size)
{
    return header::hasBanner(data, size);
}

void describeSourceIdentity(const std::uint8_t* data, std::size_t size, ImportReport& report)
{
    header::describeSourceIdentity(data, size, report);
}

musx::dom::DocumentPtr createDocument(
    const container::ParsedContainer& parsed,
    const std::uint8_t* data,
    std::size_t size,
    const std::optional<std::filesystem::path>& sourcePath,
    XmlParser parseXml, DocumentParser parseDocument,
    ImportReport& report)
{
    musx::factory::DocumentFactory::ConstructionOptions constructionOptions;
    constructionOptions.sourcePath = sourcePath;
    auto session = musx::factory::DocumentFactory::begin(std::move(constructionOptions));
    const auto& document = session.getDocument();

    // Keep both baseline representations alive through the complete import. The XML tree
    // owns the elements used to seed filtered pools; the fully formed document is the
    // read-only source for later completion passes.
    const auto pinned = defaults::parseDefault(
        parseXml, parseDocument, report.sourcePlatform);
    seedPinnedDefaults(document, pinned, report);
    document->getHeader() = header::recover(data, size, report);

    SourceProfile profile;
    profile.epoch = report.formatEpoch;
    profile.version = report.sourceVersion;
    profile.byteOrder = report.byteOrder;
    profile.platform = report.sourcePlatform;
    applyLegacyMappings(
        records::LegacyRecordIndex::build(parsed), profile,
        document, pinned.referenceDocument, report);

    // Finishing validates the pools and runs musxdom's resolvers once, after every
    // legacy overlay has been applied.
    return std::move(session).finish();
}

} // namespace finale_mus_reader
