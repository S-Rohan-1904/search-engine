#pragma once

#include <string>
#include <string_view>
#include <vector>

// The parts of an HTML page a crawler cares about.
struct HtmlPage {
    std::string title;               // contents of <title>, trimmed
    std::string text;                // visible text, whitespace collapsed
    std::vector<std::string> links;  // raw href values, in document order
};

// Extracts the title, the visible text and the links from an HTML document.
//
// This is a scanner, not a parser: it never builds a tree. That is enough for
// text extraction and it degrades gracefully on the malformed markup that most
// of the web is made of, where a real parser has to decide what a stray </div>
// means.
//
// The contents of <script> and <style> are dropped, since they are code rather
// than prose and would otherwise dominate the indexed text. Block-level tags
// become whitespace so that "<p>one</p><p>two</p>" does not read as "onetwo".
// Character references for the five XML entities, &nbsp;, and numeric
// references are decoded; everything else is left alone.
HtmlPage parse_html(std::string_view html);

// Decodes HTML character references: the five XML entities, &nbsp;, and numeric
// references, which are written out as UTF-8. Anything unrecognised is left
// exactly as written, since a visible &mdash; in the output gets noticed and
// fixed while silently deleted text does not.
std::string decode_html_entities(std::string_view text);
