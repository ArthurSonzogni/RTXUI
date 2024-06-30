#ifndef APP_HPP_
#define APP_HPP_

#include <optional>

template <typename T, typename E>
class Expected {
 public:
  Expected(T value) : value_(std::move(value)) {}  // NOLINT
  Expected(E error) : error_(std::move(error)) {}  // NOLINT

  bool has_value() const { return value_.has_value(); }
  explicit operator bool() const { return value_.has_value(); }

  T& value() { return value_.value(); }
  const T& value() const { return value_.value(); }

  E& error() { return error_.value(); }
  const E& error() const { return error_.value(); }

 private:
  std::optional<T> value_;
  std::optional<E> error_;
};

#endif  // APP_HPP_
