// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace
{

using namespace finale_mus_reader::coverage;

using ChordSuffixComparisonIdentity = std::pair<std::int64_t, std::int64_t>;

std::optional<std::int64_t> chordSuffixIntegerMember(const Value& object, std::string_view key)
{
    const auto* value = object.find(key);
    if (!value || !value->isInteger()) return std::nullopt;
    return value->asInteger();
}

std::optional<ChordSuffixComparisonIdentity> chordSuffixIdentity(const Value& object)
{
    const auto partId = chordSuffixIntegerMember(object, "part_id");
    const auto cmper = chordSuffixIntegerMember(object, "cmper");
    if (!partId || !cmper) return std::nullopt;
    return ChordSuffixComparisonIdentity{*partId, *cmper};
}

bool chordSuffixBoolMemberIs(const Value& object, std::string_view key, bool expected)
{
    const auto* value = object.find(key);
    return value && value->isBool() && value->asBool() == expected;
}

bool isChordSuffixFiller(const Value& element)
{
    if (!element.isObject()) return false;
    const auto* font = element.find("font");
    if (!font || !font->isObject()) return false;
    return chordSuffixIntegerMember(element, "symbol") == 0 &&
           chordSuffixIntegerMember(element, "xdisp") == 0 &&
           chordSuffixIntegerMember(element, "ydisp") == 0 &&
           chordSuffixIntegerMember(element, "prefix") == 0 &&
           chordSuffixBoolMemberIs(element, "is_number", false) &&
           chordSuffixIntegerMember(*font, "font_id") == 0 &&
           chordSuffixIntegerMember(*font, "font_size") == 24 &&
           chordSuffixBoolMemberIs(*font, "bold", false) &&
           chordSuffixBoolMemberIs(*font, "italic", false) &&
           chordSuffixBoolMemberIs(*font, "underline", false) &&
           chordSuffixBoolMemberIs(*font, "strikeout", false) &&
           chordSuffixBoolMemberIs(*font, "absolute", false) &&
           chordSuffixBoolMemberIs(*font, "hidden", false);
}

std::size_t removeFinaleAddedChordSuffixFillers(SurveySnapshot& source, SurveySnapshot& companion)
{
    const auto sourceFound = source.find("chord_suffix_elements");
    const auto companionFound = companion.find("chord_suffix_elements");
    if (sourceFound == source.end() || companionFound == companion.end() ||
        !sourceFound->second.isArray() || !companionFound->second.isArray())
    {
        return 0;
    }

    std::map<ChordSuffixComparisonIdentity, std::int64_t> sourceLastIncidence;
    for (const auto& element : sourceFound->second.asArray())
    {
        const auto identity = chordSuffixIdentity(element);
        const auto inci = chordSuffixIntegerMember(element, "inci");
        if (identity && inci)
        {
            sourceLastIncidence[*identity] = (std::max)(sourceLastIncidence[*identity], *inci);
        }
    }
    std::map<ChordSuffixComparisonIdentity, std::int64_t> companionLastIncidence;
    for (const auto& element : companionFound->second.asArray())
    {
        const auto identity = chordSuffixIdentity(element);
        const auto inci = chordSuffixIntegerMember(element, "inci");
        if (identity && inci)
        {
            companionLastIncidence[*identity] =
                (std::max)(companionLastIncidence[*identity], *inci);
        }
    }

    auto& elements = companionFound->second.asArray();
    const auto oldSize = elements.size();
    std::erase_if(elements,
                  [&](const Value& element)
                  {
                      const auto identity = chordSuffixIdentity(element);
                      const auto inci = chordSuffixIntegerMember(element, "inci");
                      if (!identity || !inci || !isChordSuffixFiller(element)) return false;
                      const auto sourceLast = sourceLastIncidence.find(*identity);
                      const auto companionLast = companionLastIncidence.find(*identity);
                      return sourceLast != sourceLastIncidence.end() &&
                             companionLast != companionLastIncidence.end() &&
                             *inci == sourceLast->second + 1 && *inci == companionLast->second;
                  });
    return oldSize - elements.size();
}

void prepareChordSuffixElementComparison(ComparisonPreparationContext& context)
{
    if (context.sourceEpoch != finale_mus_reader::FormatEpoch::CodaBanner) return;
    const auto removed = removeFinaleAddedChordSuffixFillers(context.source, context.companion);
    if (removed)
    {
        context.transformations[ComparisonTransformation::FinaleAddedChordSuffixFiller] += removed;
    }
}

bool numericOnlySuffixSemanticsWereLost(const ComparisonLeaves& source,
                                        const ComparisonLeaves& companion,
                                        const std::string& elementPath)
{
    const auto sourceNumber = source.find(elementPath + ".is_number");
    const auto companionNumber = companion.find(elementPath + ".is_number");
    const auto sourcePrefix = source.find(elementPath + ".prefix");
    const auto companionPrefix = companion.find(elementPath + ".prefix");
    if (sourceNumber == source.end() || !sourceNumber->second.first.isBool() ||
        !sourceNumber->second.first.asBool() || companionNumber == companion.end() ||
        !companionNumber->second.first.isBool() || companionNumber->second.first.asBool() ||
        sourcePrefix == source.end() || !sourcePrefix->second.first.isInteger() ||
        companionPrefix == companion.end() || !companionPrefix->second.first.isInteger() ||
        companionPrefix->second.first.asInteger() != 0)
    {
        return false;
    }

    const auto inciAt = elementPath.rfind(",inci=");
    if (inciAt == std::string::npos) return false;
    const auto definitionPrefix = elementPath.substr(0, inciAt + 1);
    std::set<std::string> sourceElements;
    for (const auto& [path, unused] : source)
    {
        if (path.rfind(definitionPrefix, 0) != 0) continue;
        const auto close = path.find(']', definitionPrefix.size());
        if (close != std::string::npos) sourceElements.insert(path.substr(0, close + 1));
    }
    if (sourceElements != std::set<std::string>{elementPath}) return false;

    const auto sameExceptNumberAndPrefix =
        [&](const ComparisonLeaves& left, const ComparisonLeaves& right)
    {
        for (const auto& [path, value] : left)
        {
            if (path.rfind(elementPath + ".", 0) != 0 || path == elementPath + ".is_number" ||
                path == elementPath + ".prefix")
            {
                continue;
            }
            const auto found = right.find(path);
            if (found == right.end() || found->second.first != value.first) return false;
        }
        return true;
    };
    return sameExceptNumberAndPrefix(source, companion) &&
           sameExceptNumberAndPrefix(companion, source);
}

std::optional<DifferenceClassification>
classifyChordSuffixElementDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.epoch != finale_mus_reader::FormatEpoch::CodaBanner) return std::nullopt;

    static const std::regex changedField(
        R"(^(chord_suffix_elements\[(?:part_id=\d+,)?cmper=\d+,inci=\d+\])\.(is_number|prefix)$)");
    std::match_results<std::string_view::const_iterator> match;
    if (context.category == Differs && context.origin == "legacy-mus" &&
        std::regex_match(context.path.begin(), context.path.end(), match, changedField))
    {
        const std::string elementPath(match[1].first, match[1].second);
        if (numericOnlySuffixSemanticsWereLost(context.source, context.companion, elementPath))
        {
            if (std::string_view(match[2].first, match[2].second) == "prefix")
            {
                const auto sourcePrefix = context.source.find(elementPath + ".prefix");
                if (sourcePrefix->second.first.asInteger() == 0) return std::nullopt;
            }
            return DifferenceClassification::FinaleUpgradeLoss;
        }
    }
    return std::nullopt;
}

Value observeChordSuffixElements(const SurveyContext& ctx)
{
    using Target = musx::dom::others::ChordSuffixElement;
    Value::Array result;
    for (const auto& element : sourceInstances<Target>(ctx))
    {
        const auto origin = [&ctx, &element](std::string_view member)
        { return fieldOrigin<Target>(ctx, member, *element); };
        Value::Object font{
            {"font_id", element->font->fontId},
            {"font_size", element->font->fontSize},
            {"bold", element->font->bold},
            {"italic", element->font->italic},
            {"underline", element->font->underline},
            {"strikeout", element->font->strikeout},
            {"absolute", element->font->absolute},
            {"hidden", element->font->hidden},
            {"origin_fontId", origin("font.fontId")},
            {"origin_fontSize", origin("font.fontSize")},
            {"origin_bold", origin("font.bold")},
            {"origin_italic", origin("font.italic")},
            {"origin_underline", origin("font.underline")},
            {"origin_strikeout", origin("font.strikeout")},
            {"origin_absolute", origin("font.absolute")},
            {"origin_hidden", origin("font.hidden")},
        };
        result.emplace_back(observe(
            *element, ctx, field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("inci", [](const Target& value) { return value.getInci().value_or(0); }),
            field("font", [font = std::move(font)](const Target&) { return Value(font); }),
            field("symbol",
                  [](const Target& value) { return static_cast<std::uint32_t>(value.symbol); }),
            field("xdisp", &Target::xdisp), field("ydisp", &Target::ydisp),
            field("is_number", &Target::isNumber), field("prefix", &Target::prefix),
            field("origin_symbol", [&origin](const Target&) { return origin("symbol"); }),
            field("origin_xdisp", [&origin](const Target&) { return origin("xdisp"); }),
            field("origin_ydisp", [&origin](const Target&) { return origin("ydisp"); }),
            field("origin_isNumber", [&origin](const Target&) { return origin("isNumber"); }),
            field("origin_prefix", [&origin](const Target&) { return origin("prefix"); })));
    }
    return result;
}

COVERAGE_CLASS_WITH_PREPARATION("others", "chord_suffix_elements", observeChordSuffixElements,
                                classifyChordSuffixElementDifference,
                                prepareChordSuffixElementComparison);

} // namespace
