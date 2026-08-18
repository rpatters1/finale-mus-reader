// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "reader/document_factory.h"

#include <memory>
#include <span>
#include <utility>

#include "defaults/default_document.h"
#include "import/support/embedded_graphics.h"
#include "import/header.h"
#include "import/support/legacy_mapping.h"
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
// Those references therefore resolve against the source's own table, which is the hazard
// P0.2 of research/PRODUCTION_READINESS.md describes and which this does not address.
//
// The construction context is the session's, so a seeded comparator the source table has no
// entry for is registered like any other and receives musxdom's placeholder definition. That
// is the narrow part of P0.2 this does settle: such a reference used to throw out of
// `FontInfo::getName`, and now reads as `Missing Font (n)`. A seeded comparator that the
// source table does answer is still silently answered by the wrong face.
void seedPinnedDefaults(
    const musx::dom::DocumentPtr& document,
    const defaults::ParsedDefaultDocument& pinned, ImportReport& report,
    musx::factory::ConstructionContext& construction)
{
    report.defaultsPlatform = pinned.platform;
    document->getOptions() = musx::factory::OptionsFactory::create(
        construction, pinned.options, document, pinned.optionsFilter);
    document->getOthers() = musx::factory::OthersFactory::create(
        construction, pinned.others, document, pinned.optionLikeOthersFilter);
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
    constructionOptions.embeddedGraphics = recoverEmbeddedGraphics(parsed, report);
    auto session = musx::factory::DocumentFactory::begin(std::move(constructionOptions));
    const auto& document = session.getDocument();

    // Keep both baseline representations alive through the complete import. The XML tree
    // owns the elements used to seed filtered pools; the fully formed document is the
    // read-only source for later completion passes.
    const auto pinned = defaults::parseDefault(
        parseXml, parseDocument, report.sourcePlatform);
    seedPinnedDefaults(document, pinned, report, session.getConstructionContext());
    document->getHeader() = header::recover(data, size, report);

    SourceProfile profile;
    profile.epoch = report.formatEpoch;
    profile.version = report.sourceVersion;
    profile.byteOrder = report.byteOrder;
    profile.platform = report.sourcePlatform;
    applyLegacyMappings(
        records::LegacyRecordIndex::build(parsed), profile,
        std::span<const std::uint8_t>(data, size),
        document, pinned.referenceDocument, report, session.getConstructionContext());

    // Finishing validates the pools and runs musxdom's resolvers once, after every
    // legacy overlay has been applied. That includes resolving the registered font
    // comparators, so every font id above must already have been registered.
    return std::move(session).finish();
}

} // namespace finale_mus_reader
