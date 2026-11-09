// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_INTERNAL_HANDLER_HPP_
#define RTXUI_INTERNAL_HANDLER_HPP_

// Not part of the API: how Bind() turns a method into a handler a template
// calls, reading each of its arguments from the template's text. In a public
// header only because Bind() instantiates it with the application's methods.

#include <charconv>
#include <cstddef>
#include <functional>
#include <optional>
#include <rtxui/rtxui_export.hpp>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#if defined(__cpp_impl_reflection) && __has_include(<meta>)
#include <array>
#include <meta>
#endif

#include "rtxui/event.hpp"

namespace rtxui::internal {

/// One call of a handler: what the template wrote between its parentheses,
/// and, for a key handler, the key.
struct HandlerCall {
  std::string_view name;
  std::string_view args;
  const Event* event = nullptr;
};

enum class HandlerResult {
  /// It ran, or its arguments were wrong, which is reported.
  kHandled,
  /// It ran and returned false: the event goes on to the next handler.
  kDeclined,
  /// It does not take what the call passes: no arguments when it needs some,
  /// or no key. The caller looks for another handler.
  kNotApplicable,
};

using Handler = std::function<HandlerResult(const HandlerCall&)>;

RTXUI_EXPORT bool TryParseFloat(std::string_view str, float& value);
RTXUI_EXPORT bool TryParseFloat(std::string_view str, double& value);
RTXUI_EXPORT bool TryParseFloat(std::string_view str, long double& value);
/// `str` without surrounding spaces.
RTXUI_EXPORT std::string_view TrimArgument(std::string_view str);
/// The comma-separated arguments of `str`, each trimmed.
RTXUI_EXPORT std::vector<std::string_view> SplitArguments(std::string_view str);
/// Reports a call whose arguments the handler cannot take.
RTXUI_EXPORT void ReportHandlerArguments(const HandlerCall& call,
                                         std::string_view problem);

// Whether `T` reads itself from text with a Parse(std::string_view, T&)
// function, found by argument-dependent lookup.
template <typename T>
concept HasParse = requires(std::string_view text, T& value) {
  { Parse(text, value) } -> std::convertible_to<bool>;
};

#if defined(__cpp_impl_reflection) && __has_include(<meta>)
template <typename E>
consteval auto Enumerators() {
  constexpr size_t kSize = std::meta::enumerators_of(^^E).size();
  std::array<std::meta::info, kSize> out{};
  auto all = std::meta::enumerators_of(^^E);
  for (size_t i = 0; i < kSize; ++i) {
    out[i] = all[i];
  }
  return out;
}

template <auto kEnumerators, typename E, size_t... Is>
bool ParseEnumerator(std::string_view text,
                     E& value,
                     std::index_sequence<Is...> /*unused*/) {
  return ((std::meta::identifier_of(kEnumerators[Is]) == text
               ? (value = [:kEnumerators[Is]:], true)
               : false) ||
          ...);
}
#endif

template <typename T>
inline constexpr bool kDependentFalse = false;

/// Reads `value` from one argument's text. Whether it could.
template <typename T>
bool ParseArgument(std::string_view text, T& value) {
  if constexpr (HasParse<T>) {
    return Parse(text, value);
  } else if constexpr (std::is_same_v<T, std::string> ||
                       std::is_same_v<T, std::string_view>) {
    value = T(text);
    return true;
  } else if constexpr (std::is_same_v<T, bool>) {
    text = TrimArgument(text);
    if (text == "true" || text == "1") {
      value = true;
      return true;
    }
    if (text == "false" || text == "0") {
      value = false;
      return true;
    }
    return false;
  } else if constexpr (std::is_integral_v<T>) {
    text = TrimArgument(text);
    const char* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(text.data(), end, value);
    return !text.empty() && ec == std::errc() && ptr == end;
  } else if constexpr (std::is_floating_point_v<T>) {
    return TryParseFloat(TrimArgument(text), value);
  } else if constexpr (std::is_enum_v<T>) {
#if defined(__cpp_impl_reflection) && __has_include(<meta>)
    constexpr auto kEnumerators = Enumerators<T>();
    return ParseEnumerator<kEnumerators>(
        TrimArgument(text), value,
        std::make_index_sequence<kEnumerators.size()>{});
#else
    static_assert(kDependentFalse<T>,
                  "A handler taking an enum needs C++26 reflection, or a "
                  "`bool Parse(std::string_view, E&)` function next to the "
                  "enum.");
    return false;
#endif
  } else {
    static_assert(kDependentFalse<T>,
                  "A handler argument must be a number, a bool, a string, an "
                  "enum, Event::Keyboard, or a type with a `bool "
                  "Parse(std::string_view, T&)` function next to it.");
    return false;
  }
}

// The parts of a method's type a handler needs.
template <typename F>
struct MethodSignature;
template <typename R, typename... A>
struct MethodSignature<R(A...)> {
  using Return = R;
  using Args = std::tuple<std::decay_t<A>...>;
};
template <typename R, typename... A>
struct MethodSignature<R(A...) const> : MethodSignature<R(A...)> {};
template <typename R, typename... A>
struct MethodSignature<R(A...) noexcept> : MethodSignature<R(A...)> {};
template <typename R, typename... A>
struct MethodSignature<R(A...) const noexcept> : MethodSignature<R(A...)> {};

template <typename T>
inline constexpr bool kIsKey = std::is_same_v<T, Event::Keyboard>;
template <typename T>
inline constexpr bool kIsString =
    std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>;

// What reading a call's arguments gave.
enum class ReadResult {
  kOk,
  // The call passes something else than the handler takes: no arguments when
  // it needs some, arguments when it takes none, or no key.
  kNotApplicable,
  // The arguments are wrong, which is reported.
  kInvalid,
};

// Fills `values` from `call`: each Event::Keyboard with the key, the others
// from the arguments, in order.
template <typename... A, size_t... Is>
ReadResult ReadArguments(const HandlerCall& call,
                         std::tuple<std::optional<A>...>& values,
                         std::index_sequence<Is...> /*unused*/) {
  constexpr size_t kTextArgs = ((kIsKey<A> ? 0 : 1) + ... + 0);
  constexpr bool kTakesKey = (kIsKey<A> || ...);
  if constexpr (kTakesKey) {
    if (!call.event || !call.event->is<Event::Keyboard>()) {
      return ReadResult::kNotApplicable;
    }
  }

  std::vector<std::string_view> texts;
  if constexpr (kTextArgs == 0) {
    if (!TrimArgument(call.args).empty()) {
      return ReadResult::kNotApplicable;
    }
  } else if constexpr (kTextArgs == 1) {
    // One argument takes the whole text, commas included. Only a string can
    // be empty: an `onclick="Pick"` does not call Pick(int).
    if (!(kIsString<A> || ...) && TrimArgument(call.args).empty()) {
      return ReadResult::kNotApplicable;
    }
    texts.push_back(call.args);
  } else {
    if (TrimArgument(call.args).empty()) {
      return ReadResult::kNotApplicable;
    }
    texts = SplitArguments(call.args);
    if (texts.size() != kTextArgs) {
      ReportHandlerArguments(call, "takes " + std::to_string(kTextArgs) +
                                       " arguments, not " +
                                       std::to_string(texts.size()));
      return ReadResult::kInvalid;
    }
  }

  size_t next = 0;
  bool ok = true;
  [[maybe_unused]] const auto read = [&]<typename T>(std::optional<T>& value) {
    if (!ok) {
      return;
    }
    if constexpr (kIsKey<T>) {
      value = call.event->get<Event::Keyboard>();
    } else {
      const size_t index = next++;
      T parsed{};
      if (!ParseArgument(texts[index], parsed)) {
        ReportHandlerArguments(
            call, "cannot take '" + std::string(texts[index]) +
                      "' as argument " + std::to_string(index + 1));
        ok = false;
        return;
      }
      value = std::move(parsed);
    }
  };
  (read(std::get<Is>(values)), ...);
  return ok ? ReadResult::kOk : ReadResult::kInvalid;
}

template <typename Self, typename Return, typename... A, size_t... Is>
HandlerResult CallHandler(Self* self,
                          auto member,
                          const HandlerCall& call,
                          std::index_sequence<Is...> indices) {
  std::tuple<std::optional<A>...> values;
  switch (ReadArguments(call, values, indices)) {
    case ReadResult::kOk:
      break;
    case ReadResult::kNotApplicable:
      return HandlerResult::kNotApplicable;
    case ReadResult::kInvalid:
      return HandlerResult::kHandled;
  }
  if constexpr (std::is_same_v<Return, bool>) {
    return (self->*member)(std::move(*std::get<Is>(values))...)
               ? HandlerResult::kHandled
               : HandlerResult::kDeclined;
  } else {
    (self->*member)(std::move(*std::get<Is>(values))...);
    return HandlerResult::kHandled;
  }
}

template <typename Self, typename Return, typename Args>
struct HandlerMaker;
template <typename Self, typename Return, typename... A>
struct HandlerMaker<Self, Return, std::tuple<A...>> {
  static Handler Make(Self* self, auto member) {
    return [self, member](const HandlerCall& call) {
      return CallHandler<Self, Return, A...>(self, member, call,
                                             std::index_sequence_for<A...>{});
    };
  }
};

/// A handler calling `member` on `self`.
template <typename Self, typename F, typename C>
Handler MakeHandler(Self* self, F C::* member) {
  using Signature = MethodSignature<F>;
  return HandlerMaker<Self, typename Signature::Return,
                      typename Signature::Args>::Make(self, member);
}

}  // namespace rtxui::internal

#endif  // RTXUI_INTERNAL_HANDLER_HPP_
