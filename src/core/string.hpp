#ifndef STRING_HPP_
#define STRING_HPP_

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <iterator>

// Split a string by a delimiter, return a vector of string views.
auto Split(std::string_view text,
           char delimiter) -> std::vector<std::string_view>;
auto Split(std::string_view text,
           std::string_view delimiter) -> std::vector<std::string_view>;
auto Join(const std::vector<std::string_view>& parts,
          std::string_view delimiter) -> std::string;
auto Join(const std::vector<std::string>& parts,
          std::string_view delimiter) -> std::string;
auto StripIndent(const std::string_view& text) -> std::string;
auto Repeat(std::string_view text, int count) -> std::string;
auto CodePointToString(uint32_t codepoint) -> std::string;
auto EatCodePoint(std::string_view input,
                  size_t start,
                  size_t* end,
                  uint32_t* ucs) -> bool;
auto EatCodePoint(std::wstring_view input,
                  size_t start,
                  size_t* end,
                  uint32_t* ucs) -> bool;

auto IsCombining(uint32_t ucs) -> bool;
auto IsFullWidth(uint32_t ucs) -> bool;
auto IsControl(uint32_t ucs) -> bool;
auto string_width(std::string_view input) -> int;
auto Utf8ToGlyphs(std::string_view input) -> std::vector<std::string>;
auto GlyphPrevious(std::string_view input, size_t start) -> size_t;
auto GlyphNext(std::string_view input, size_t start) -> size_t;
auto GlyphIterate(std::string_view input, int glyph_offset, size_t start = 0) -> size_t;
auto CellToGlyphIndex(std::string_view input) -> std::vector<int>;
auto GlyphCount(std::string_view input) -> int;
auto to_wstring(std::string_view s) -> std::wstring;
auto to_string(std::wstring_view s) -> std::string;

struct Grapheme {
  std::string_view text;
  int width = 0;

  bool operator==(const Grapheme& other) const {
    return text == other.text && width == other.width;
  }
};

class GraphemeIterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = Grapheme;
  using difference_type = std::ptrdiff_t;
  using pointer = const Grapheme*;
  using reference = const Grapheme&;

  GraphemeIterator() = default;
  GraphemeIterator(std::string_view text, size_t pos);

  const Grapheme& operator*() const { return current_; }
  const Grapheme* operator->() const { return &current_; }

  GraphemeIterator& operator++();
  GraphemeIterator operator++(int) {
    GraphemeIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  bool operator==(const GraphemeIterator& other) const {
    return pos_ == other.pos_ && text_.data() == other.text_.data();
  }

 private:
  void Next();

  std::string_view text_;
  size_t pos_ = 0;
  Grapheme current_;
};

class Graphemes {
 public:
  explicit Graphemes(std::string_view text) : text_(text) {}

  GraphemeIterator begin() const { return GraphemeIterator(text_, 0); }
  GraphemeIterator end() const { return GraphemeIterator(text_, text_.size()); }

 private:
  std::string_view text_;
};

#endif  // STRING_HPP_
