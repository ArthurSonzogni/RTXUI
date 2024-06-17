#ifndef REACTIVE_HPP_
#define REACTIVE_HPP_

#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace reactive {

/// A Reactive object represents a value that can be invalidated.
/// The values are ref counted and garbage collected.

// The possible types of a Reactive object.
enum Type {
  kNull,
  kBool,
  kInt,
  kDouble,
  kString,
  kArray,
  kMap,
  kSet,
};

class Reactive;
class Iterator;
struct Value;

struct Value {
  std::variant<bool,
               int,
               double,
               std::string,
               std::vector<Reactive>,
               std::map<std::string, Reactive>,
               std::set<Reactive>>
      value;
  std::uint16_t ref_count = 1;
};

Reactive Null();
Reactive Bool(bool value);
Reactive Int(int value);
Reactive Double(double value);
Reactive String(const char* value);
Reactive String(const std::string& value);
Reactive Array();
Reactive Array(std::initializer_list<Reactive> values);
Reactive Map();
Reactive Map(std::initializer_list<std::pair<std::string, Reactive>> values);
Reactive Set();
Reactive Set(std::initializer_list<Reactive> values);

class Reactive {
 public:
  // Constructors
  Reactive();
  Reactive(bool value);
  Reactive(int value);
  Reactive(double value);
  Reactive(const char* value);
  Reactive(const std::string& value);
  static Reactive Array();
  static Reactive Map();
  static Reactive Set();

  // Copy and move constructors:
  Reactive(const Reactive& other);
  Reactive(Reactive&& other);

  // Destructor
  ~Reactive();

  // Type checkers
  Type Type() const;
  bool IsNull() const;
  bool IsBool() const;
  bool IsInt() const;
  bool IsDouble() const;
  bool IsString() const;
  bool IsArray() const;
  bool IsMap() const;
  bool IsSet() const;

  // Value getters
  bool AsBool() const;
  int AsInt() const;
  double AsDouble() const;
  std::string AsString() const;

  // Operators:
  Reactive& operator=(const Reactive& other);
  Reactive& operator=(Reactive&& other);
  bool operator==(const Reactive& other) const;
  bool operator!=(const Reactive& other) const;
  void operator=(bool value);
  Reactive operator+(const Reactive& other) const;
  Reactive operator-(const Reactive& other) const;
  Reactive operator*(const Reactive& other) const;
  Reactive operator/(const Reactive& other) const;
  Reactive operator%(const Reactive& other) const;
  Reactive operator&&(const Reactive& other) const;
  Reactive operator||(const Reactive& other) const;
  Reactive operator!() const;
  Reactive operator[](int index) const;
  Reactive operator[](const char* key) const;
  Reactive operator[](const std::string& key) const;

  std::strong_ordering operator<=>(const Reactive&) const = default;

  // Iterators:
  Iterator begin() const;
  Iterator end() const;

  // Methods:
  void push_back(const Reactive& value);
  void insert(const std::string& key, const Reactive& value);
  void insert(const Reactive& value);
  void erase(const std::string& key);
  void erase(int index);
  void clear();
  int size() const;
  bool empty() const;

 private:
  friend class Iterator;
  Reactive(std::vector<Reactive> value);
  Reactive(std::map<std::string, Reactive> value);
  Reactive(std::set<Reactive> value);
  void AssignNull();

  Value* ptr_;
};

// Iterator over a Reactive object. It can be used in range-based for loops.
class Iterator {
 public:
  // Operators:
  Reactive& operator*();
  Iterator& operator++();
  bool operator==(const Iterator& other) const;
  bool operator!=(const Iterator& other) const;

 private:
  friend class Reactive;
  // Constructors:
  Iterator();
  Iterator(std::vector<Reactive>::iterator it);
  Iterator(std::set<Reactive>::iterator it);
  Iterator(std::map<std::string, Reactive>::iterator it);

  Reactive r_;
  std::optional<std::variant<std::vector<Reactive>::iterator,
                             std::set<Reactive>::iterator,
                             std::map<std::string, Reactive>::iterator>>
      it_;
};

}  // namespace reactive

#endif  // REACTIVE_HPP_
