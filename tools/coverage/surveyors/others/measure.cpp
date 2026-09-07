// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "coverage/classification_rules.h"
#include "coverage/registry.h"
#include "coverage/support/source_gate.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

using MeasureSurveyTarget = musx::dom::others::Measure;

/// @brief One member whose value a musxdom class the reader does not yet recover would settle.
struct DeferredRecovery
{
    /// @brief The comparison-path suffix that names the member.
    std::string_view leaf;
    /// @brief The provenance the source side must report, so a recovered value never matches.
    std::string_view origin;
    /// @brief The class whose recovery settles it, named for whoever deletes this row.
    std::string_view blockedOn;
};

/// @brief Every deferred measure member, and nothing else.
/// @details **This table is the whole of the deferral.** Deleting a row makes its differences
/// unexpected again with no other change, which is the point: the rows are a list of work owed,
/// not a description of how the format behaves.
///
/// Every row is one of the measure's "something is attached here" flags. Finale 27 recomputes all of
/// them from the objects it finds, so each waits on the class that holds those objects and on
/// nothing else. `has_chord` differs in two ways: the six-word layout has no word to hold its bit
/// and reports the era's behavior instead, so it appears under both provenances; and what it waits
/// on is entry recovery rather than a pooled class, because the chord record is an entry detail
/// keyed by entry number. Recovering that class re-derives the member for every era, including the
/// ones whose bit is read directly.
constexpr DeferredRecovery deferredMeasureRecoveries[] = {
    {".has_smart_shape", "legacy-mus", "others::SmartShape"},
    {".has_expression", "legacy-mus", "others::MeasureExprAssign"},
    {".has_text_block", "legacy-mus", "details::MeasureTextAssign"},
    {".has_ossia", "legacy-mus", "details::MeasureOssiaAssign"},
    {".has_chord", "legacy-mus", "entry recovery: the chord record is an entry detail"},
    // The six-word layout has no word to hold the bit, so it reports the era's behavior instead.
    {".has_chord", "legacy-behavior", "entry recovery: the chord record is an entry detail"},
};


/// @brief Whether a later Finale beta authored this file and then wrote it out in an older format.
/// @details A back-save carries two versions: the creator names the release that made the
/// document, the last saver the format on disk. When the creator postdates the format it wrote,
/// it had members the older layout has nowhere to put -- and a beta build of it may write them
/// anywhere at all. Finale 27 reads such a file with the knowledge of what made it, so it can
/// state a value the format cannot carry and the source cannot supply.
[[nodiscard]] bool sourceWrittenByLaterBeta(const DifferenceContext& context)
{
    const auto& creator = context.sourceReport.creatorVersion;
    return creator && sourceIsBeta(&*creator)
        && finale_mus_reader::VersionBound{creator->major, creator->minor}
            > finale_mus_reader::versions::finale2012;
}

// musxdom's barline values that Finale checks "Barline ends word extensions" for.
constexpr std::int64_t doubleBarline = 3;
constexpr std::int64_t finalBarline = 4;
constexpr std::int64_t solidBarline = 5;

/// @brief Whether a measure carries a barline Finale ties the word-extension break to.
/// @details Double, final and solid, plus a backwards repeat, which Finale treats the same way.
/// Selecting any of them in the modern interface checks the box; selecting anything else clears
/// it, and only a deliberate edit afterwards separates the two.
[[nodiscard]] bool measureEndsWordExtensions(const DifferenceContext& context,
    std::string_view prefix)
{
    const auto barline = comparisonIntegerLeaf(context.source, std::string(prefix) + "barline_type");
    if (barline && (*barline == doubleBarline || *barline == finalBarline
            || *barline == solidBarline)) {
        return true;
    }
    const auto repeat = context.source.find(std::string(prefix) + "backwards_repeat_bar");
    return repeat != context.source.end() && repeat->second.first.isBool()
        && repeat->second.first.asBool();
}

/// @brief One time-signature word that is a comparator rather than a value, and the flag saying so.
struct CompositeTimeSigWord
{
    std::string_view leaf;
    std::string_view flag;
};

/// @brief The two words a composite time signature turns into list comparators.
/// @details `beats` and `divBeat` hold a count and an Edu value until their composite flag is set,
/// at which point each is a comparator into a `timeUpper` or `timeLower` list. Comparing those by
/// number across an upgrade is not sound: the Coda-banner interface lets a numerator and a
/// denominator be shared between measures and combined freely, while later releases bind a
/// composite pair to one time signature.
///
/// Both wait on `others::TimeCompositeUpper` and `others::TimeCompositeLower`. Once those are
/// recovered the comparison can resolve each side's comparator through its own list and compare
/// the time signatures, which is what settles these for good.
constexpr CompositeTimeSigWord compositeTimeSigWords[] = {
    {".beats", "composite_numerator"},
    {".div_beat", "composite_denominator"},
};

/// @brief Whether a measure's leaf is a composite comparator rather than a stored value.
[[nodiscard]] bool measureWordIsCompositeComparator(const DifferenceContext& context,
    std::string_view prefix, std::string_view flag)
{
    const auto found = context.source.find(std::string(prefix) + std::string(flag));
    return found != context.source.end() && found->second.first.isBool()
        && found->second.first.asBool();
}

std::optional<DifferenceClassification>
classifyMeasureDifference(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs) return std::nullopt;
    if (!context.path.starts_with("measures[")) return std::nullopt;

    // Finale's musx conversion writes "Barline ends word extensions" from the barline itself,
    // where the source leaves the stored bit clear. That is the upgrade stating what the era did
    // rather than what the file says, and the file is what is reported. Only the direction the
    // conversion adds is classified: a set bit the companion drops would be a real loss.
    if (context.path.ends_with(".break_word_ext") && context.origin == "legacy-mus"
        && context.sourceValue.isBool() && context.companionValue.isBool()
        && !context.sourceValue.asBool() && context.companionValue.asBool()) {
        const auto prefix = context.path.substr(0, context.path.size()
            - std::string_view(".break_word_ext").size() + 1);
        if (measureEndsWordExtensions(context, prefix)) {
            return DifferenceClassification::FinaleUpgradeLoss;
        }
    }

    // The two key-signature switches are the members Finale 2014 built out of words that are
    // filler through Finale 2012, so no supported layout can carry either. A document a later
    // beta authored and back-saved is the one case where the companion can state one anyway, and
    // it is not a recovery failure: the value is not in the bytes.
    if ((context.path.ends_with(".global_key_sig.hide_key_sig_show_accis")
            || context.path.ends_with(".global_key_sig.keyless"))
        && sourceWrittenByLaterBeta(context)) {
        return DifferenceClassification::BetaDiscrepancy;
    }

    if (!deferredRecoveryClassified()) return std::nullopt;

    // A composite time-signature word is a list comparator, and the two sides number their lists
    // independently. Only a word whose own flag says it is a comparator is deferred; a plain beat
    // count or Edu value that disagrees is a decoding failure and stays unexpected.
    if (context.sourceValue.isInteger() && context.companionValue.isInteger()) {
        for (const auto& composite : compositeTimeSigWords) {
            if (!context.path.ends_with(composite.leaf)) continue;
            const auto prefix = context.path.substr(
                0, context.path.size() - composite.leaf.size() + 1);
            if (measureWordIsCompositeComparator(context, prefix, composite.flag)) {
                return DifferenceClassification::AwaitsDependentRecovery;
            }
        }
    }
    // Both directions. A clear source bit against a set companion one is a value the reader owes;
    // a set one against a clear companion one is a cache the reader cannot yet know is stale.
    // Neither is answerable until the class that holds the objects is recovered, so both wait on
    // it and both leave when it lands.
    if (!context.sourceValue.isBool() || !context.companionValue.isBool()) return std::nullopt;
    for (const auto& deferred : deferredMeasureRecoveries) {
        if (context.path.ends_with(deferred.leaf) && context.origin == deferred.origin) {
            return DifferenceClassification::AwaitsDependentRecovery;
        }
    }
    return std::nullopt;
}

// Every origin leaf names its C++ member exactly, so the comparison model can pair the two halves
// by spelling. Building them through one helper keeps that pairing from drifting a member at a
// time.
auto measureOrigin(const char* member)
{
    return [member](const MeasureSurveyTarget& value, const SurveyContext& context) {
        // Keyed by the instance's own identity rather than by comparator alone, because a measure
        // exists once per source part: an unlinked part carries its own object under the same
        // comparator as the score's.
        return fieldOrigin<MeasureSurveyTarget>(context, member, value);
    };
}

// The contained key signature, named rather than flattened. Its three leaves are reported by the
// importer under the owning member's path, which is the pairing the comparison model needs.
Value observeGlobalKeySig(const MeasureSurveyTarget& measure, const SurveyContext& context)
{
    const auto& key = *measure.globalKeySig;
    const auto origin = [&](std::string_view leaf) {
        return fieldOrigin<MeasureSurveyTarget>(
            context, "globalKeySig." + std::string(leaf), measure);
    };
    return Value::Object{{"key", key.key}, {"keyless", key.keyless},
        {"hide_key_sig_show_accis", key.hideKeySigShowAccis},
        {"origin_key", origin("key")}, {"origin_keyless", origin("keyless")},
        {"origin_hideKeySigShowAccis", origin("hideKeySigShowAccis")}};
}

// The comparison identity of one observed measure: its source part and its comparator.
[[nodiscard]] std::optional<std::pair<std::int64_t, std::int64_t>> measureIdentity(
    const Value& measure)
{
    if (!measure.isObject()) return std::nullopt;
    const auto& fields = measure.asObject();
    const auto part = fields.find("part_id");
    const auto cmper = fields.find("cmper");
    if (part == fields.end() || cmper == fields.end()) return std::nullopt;
    if (!part->second.isInteger() || !cmper->second.isInteger()) return std::nullopt;
    return std::pair{part->second.asInteger(), cmper->second.asInteger()};
}

// Whether two observed measures state the same thing, ignoring what distinguishes the instances
// rather than their content.
[[nodiscard]] bool measureContentMatches(const Value& left, const Value& right)
{
    static const std::set<std::string, std::less<>> identityKeys{
        "part_id", "share_mode", "cmper", "origin"};
    if (!left.isObject() || !right.isObject()) return false;
    const auto& a = left.asObject();
    const auto& b = right.asObject();
    for (const auto& [key, value] : a) {
        if (identityKeys.count(key) || key.starts_with("origin_")) continue;
        const auto found = b.find(key);
        if (found == b.end() || !(found->second == value)) return false;
    }
    return true;
}

/// @brief Drops companion part measures that state exactly what their own score measure states.
/// @details Finale 27 creates a part instance for every measure of every linked part on upgrade,
/// whether or not the legacy file has a record for it. Only what the source stores is built, and
/// musxdom resolves a part request with no part object to the score object, so where the
/// companion's part measure repeats its score measure the two documents say the same thing.
///
/// **Only that case is dropped.** A part measure Finale re-laid out -- carrying its own width or
/// positioning mode -- differs from its score measure and stays in the comparison, because the
/// reader genuinely does not have those values and never can: no record in the source states them.
[[nodiscard]] std::size_t dropMaterializedPartMeasures(const SurveySnapshot& source,
    SurveySnapshot& companion)
{
    const auto sourceFound = source.find("measures");
    const auto companionFound = companion.find("measures");
    if (sourceFound == source.end() || companionFound == companion.end()
        || !sourceFound->second.isArray() || !companionFound->second.isArray()) {
        return 0;
    }
    std::set<std::pair<std::int64_t, std::int64_t>> sourceInstances;
    for (const auto& measure : sourceFound->second.asArray()) {
        if (const auto identity = measureIdentity(measure)) sourceInstances.insert(*identity);
    }
    std::map<std::int64_t, const Value*> companionScore;
    for (const auto& measure : companionFound->second.asArray()) {
        const auto identity = measureIdentity(measure);
        if (identity && identity->first == musx::dom::SCORE_PARTID) {
            companionScore.emplace(identity->second, &measure);
        }
    }

    auto& measures = companionFound->second.asArray();
    const auto before = measures.size();
    std::erase_if(measures, [&](const Value& measure) {
        const auto identity = measureIdentity(measure);
        if (!identity || identity->first == musx::dom::SCORE_PARTID) return false;
        // Only a part instance the reader does not have at all, and only where the companion's
        // own score measure already says the same thing.
        if (sourceInstances.count(*identity)) return false;
        const auto score = companionScore.find(identity->second);
        return score != companionScore.end() && measureContentMatches(*score->second, measure);
    });
    return before - measures.size();
}

void prepareMeasureComparison(ComparisonPreparationContext& context)
{
    const auto dropped = dropMaterializedPartMeasures(context.source, context.companion);
    if (dropped) {
        context.transformations[ComparisonTransformation::FinaleMaterializedPartMeasure] += dropped;
    }
}

Value observeMeasures(const SurveyContext& ctx)
{
    using Target = MeasureSurveyTarget;
    Value::Array result;
    for (const auto& measure : sourceInstances<Target>(ctx)) {
        result.push_back(observe(*measure, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("width", &Target::width),
            field("global_key_sig", &observeGlobalKeySig),
            field("beats", &Target::beats),
            field("div_beat", &Target::divBeat),
            field("disp_beats", &Target::dispBeats),
            field("disp_divbeat", &Target::dispDivbeat),
            field("custom_bar_shape", &Target::customBarShape),
            field("custom_left_bar_shape", &Target::customLeftBarShape),
            field("front_space_extra", &Target::frontSpaceExtra),
            field("back_space_extra", &Target::backSpaceExtra),
            field("break_word_ext", &Target::breakWordExt),
            field("hide_caution", &Target::hideCaution),
            field("has_smart_shape", &Target::hasSmartShape),
            field("group_barline_override", &Target::groupBarlineOverride),
            field("show_full_names", &Target::showFullNames),
            field("has_meas_numb_indiv_pos", &Target::hasMeasNumbIndivPos),
            field("allow_split_points", &Target::allowSplitPoints),
            field("composite_numerator", &Target::compositeNumerator),
            field("composite_denominator", &Target::compositeDenominator),
            field("show_key", &Target::showKey),
            field("show_time", &Target::showTime),
            field("evenly_across_measure", &Target::evenlyAcrossMeasure),
            field("positioning_mode", &Target::positioningMode),
            field("begin_new_system", &Target::beginNewSystem),
            field("has_expression", &Target::hasExpression),
            field("break_mm_rest", &Target::breakMmRest),
            field("no_meas_num", &Target::noMeasNum),
            field("has_ossia", &Target::hasOssia),
            field("has_text_block", &Target::hasTextBlock),
            field("barline_type", &Target::barlineType),
            field("forward_repeat_bar", &Target::forwardRepeatBar),
            field("backwards_repeat_bar", &Target::backwardsRepeatBar),
            field("has_ending", &Target::hasEnding),
            field("has_text_repeat", &Target::hasTextRepeat),
            field("abbrv_time", &Target::abbrvTime),
            field("use_display_timesig", &Target::useDisplayTimesig),
            field("has_chord", &Target::hasChord),
            field("left_barline_type", &Target::leftBarlineType),
            field("composite_disp_numerator", &Target::compositeDispNumerator),
            field("composite_disp_denominator", &Target::compositeDispDenominator),
            field("page_break", &Target::pageBreak),
            field("origin_width", measureOrigin("width")),
            field("origin_beats", measureOrigin("beats")),
            field("origin_divBeat", measureOrigin("divBeat")),
            field("origin_dispBeats", measureOrigin("dispBeats")),
            field("origin_dispDivbeat", measureOrigin("dispDivbeat")),
            field("origin_customBarShape", measureOrigin("customBarShape")),
            field("origin_customLeftBarShape", measureOrigin("customLeftBarShape")),
            field("origin_frontSpaceExtra", measureOrigin("frontSpaceExtra")),
            field("origin_backSpaceExtra", measureOrigin("backSpaceExtra")),
            field("origin_breakWordExt", measureOrigin("breakWordExt")),
            field("origin_hideCaution", measureOrigin("hideCaution")),
            field("origin_hasSmartShape", measureOrigin("hasSmartShape")),
            field("origin_groupBarlineOverride", measureOrigin("groupBarlineOverride")),
            field("origin_showFullNames", measureOrigin("showFullNames")),
            field("origin_hasMeasNumbIndivPos", measureOrigin("hasMeasNumbIndivPos")),
            field("origin_allowSplitPoints", measureOrigin("allowSplitPoints")),
            field("origin_compositeNumerator", measureOrigin("compositeNumerator")),
            field("origin_compositeDenominator", measureOrigin("compositeDenominator")),
            field("origin_showKey", measureOrigin("showKey")),
            field("origin_showTime", measureOrigin("showTime")),
            field("origin_evenlyAcrossMeasure", measureOrigin("evenlyAcrossMeasure")),
            field("origin_positioningMode", measureOrigin("positioningMode")),
            field("origin_beginNewSystem", measureOrigin("beginNewSystem")),
            field("origin_hasExpression", measureOrigin("hasExpression")),
            field("origin_breakMmRest", measureOrigin("breakMmRest")),
            field("origin_noMeasNum", measureOrigin("noMeasNum")),
            field("origin_hasOssia", measureOrigin("hasOssia")),
            field("origin_hasTextBlock", measureOrigin("hasTextBlock")),
            field("origin_barlineType", measureOrigin("barlineType")),
            field("origin_forwardRepeatBar", measureOrigin("forwardRepeatBar")),
            field("origin_backwardsRepeatBar", measureOrigin("backwardsRepeatBar")),
            field("origin_hasEnding", measureOrigin("hasEnding")),
            field("origin_hasTextRepeat", measureOrigin("hasTextRepeat")),
            field("origin_abbrvTime", measureOrigin("abbrvTime")),
            field("origin_useDisplayTimesig", measureOrigin("useDisplayTimesig")),
            field("origin_hasChord", measureOrigin("hasChord")),
            field("origin_leftBarlineType", measureOrigin("leftBarlineType")),
            field("origin_compositeDispNumerator", measureOrigin("compositeDispNumerator")),
            field("origin_compositeDispDenominator", measureOrigin("compositeDispDenominator")),
            field("origin_pageBreak", measureOrigin("pageBreak"))));
    }
    return Value(std::move(result));
}

COVERAGE_CLASS_WITH_PREPARATION("others", "measures", observeMeasures,
    classifyMeasureDifference, prepareMeasureComparison);

} // namespace
