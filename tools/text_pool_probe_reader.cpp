// Print every texts-pool object the reader recovers from one MUS file, so the result can be
// held beside a Finale 27 companion. Runs through the public reader, so it reports what the
// importer makes of the file rather than what the file stores.

#include <cstdio>
#include <string>
#include <string_view>

#include "finale_mus_reader/reader.h"
#include "musx/musx.h"
#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#endif
#include "musx/xml/PugiXmlImpl.h"

namespace {

std::string escape(std::string_view text)
{
    std::string result;
    for (const char ch : text) {
        if (ch == '\n') result += "\\n";
        else if (ch == '\r') result += "\\r";
        else result += ch;
    }
    return result;
}

template <typename T>
void dump(const musx::dom::DocumentPtr& document, const char* label)
{
    for (const auto& item : document->getTexts()->getArray<T>()) {
        std::printf("  %s\t%u\t%s\n", label, item->getTextNumber(),
            escape(item->text).c_str());
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: text_pool_probe_reader <file>\n");
        return 2;
    }
    const auto result = finale_mus_reader::Reader::read<musx::xml::pugi::Document>(argv[1]);
    if (!result.document) {
        std::fprintf(stderr, "import failed\n");
        return 1;
    }
    std::printf("%s\n", argv[1]);
    using namespace musx::dom::texts;
    dump<FileInfoText>(result.document, "fileInfo");
    dump<BlockText>(result.document, "blockText");
    dump<LyricsVerse>(result.document, "verse");
    dump<LyricsChorus>(result.document, "chorus");
    dump<LyricsSection>(result.document, "section");
    dump<SmartShapeText>(result.document, "smartShapeText");
    dump<ExpressionText>(result.document, "expression");
    for (const auto& diagnostic : result.report.diagnostics) {
        if (diagnostic.level != musx::util::Logger::LogLevel::Verbose) {
            std::printf("  ! %s\n", diagnostic.message.c_str());
        }
    }
    return 0;
}
