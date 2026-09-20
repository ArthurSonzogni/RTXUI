#ifndef STRING_HPP_
#define STRING_HPP_

#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

// Split a string by a delimiter, return a vector of string views.
auto Split(std::string_view text, char delimiter)
    -> std::vector<std::string_view>;
auto Split(std::string_view text, std::string_view delimiter)
    -> std::vector<std::string_view>;
auto StripIndent(const std::string_view& text) -> std::string;
auto Repeat(std::string_view text, int count) -> std::string;
auto CodePointToString(uint32_t codepoint) -> std::string;
auto Base64Encode(std::string_view input) -> std::string;
auto EatCodePoint(std::string_view input,
                  size_t start,
                  size_t* end,
                  uint32_t* ucs) -> bool;

auto IsCombining(uint32_t ucs) -> bool;
auto IsFullWidth(uint32_t ucs) -> bool;
auto IsControl(uint32_t ucs) -> bool;
auto string_width(std::string_view input) -> int;

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
  // Optimization: Inlined fast-path for ASCII characters to avoid out-of-line
  // iterator calls and NextSlow overhead. Yields ~6% speedup in Layout/Paint.
  GraphemeIterator(std::string_view text, size_t pos) : text_(text), pos_(pos) {
    if (pos_ < text_.size()) {
      unsigned char c = static_cast<unsigned char>(text_[pos_]);
      if (c < 128 && c != '\r') {
        if (pos_ + 1 >= text_.size() ||
            static_cast<unsigned char>(text_[pos_ + 1]) < 128) {
          int width = (c >= 32) ? 1 : 0;
          current_ = Grapheme{text_.substr(pos_, 1), width};
          return;
        }
      }
      NextSlow();
    }
  }

  const Grapheme& operator*() const { return current_; }
  const Grapheme* operator->() const { return &current_; }

  GraphemeIterator& operator++() {
    pos_ += current_.text.size();
    if (pos_ < text_.size()) {
      unsigned char c = static_cast<unsigned char>(text_[pos_]);
      if (c < 128 && c != '\r') {
        if (pos_ + 1 >= text_.size() ||
            static_cast<unsigned char>(text_[pos_ + 1]) < 128) {
          int width = (c >= 32) ? 1 : 0;
          current_ = Grapheme{text_.substr(pos_, 1), width};
          return *this;
        }
      }
      NextSlow();
    } else {
      pos_ = text_.size();
      current_ = Grapheme{"", 0};
    }
    return *this;
  }
  GraphemeIterator operator++(int) {
    GraphemeIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  bool operator==(const GraphemeIterator& other) const {
    return pos_ == other.pos_ && text_.data() == other.text_.data();
  }

 private:
  void NextSlow();

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
