#pragma once

#include <filesystem>
#include <memory>
#include <optional>

namespace musx {
namespace dom {
class Document;
} // namespace dom
} // namespace musx

namespace finale_mus_reader {
namespace defaults {

[[nodiscard]] std::shared_ptr<musx::dom::Document> createMacOSOptionsDocument(
    const std::optional<std::filesystem::path>& sourcePath = std::nullopt);

} // namespace defaults
} // namespace finale_mus_reader
