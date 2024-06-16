#ifndef REACTIVE_HPP_
#define REACTIVE_HPP_

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace reactive {

/// A Reactive object represents a value that can be invalidated.
/// The values are garbage collected.

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

Reactive Array();
Reactive Map();
Reactive Set();

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
  Reactive(std::vector<Reactive> value);
  Reactive(std::map<std::string, Reactive> value);
  Reactive(std::set<Reactive> value);
  void AssignNull();

  Value* ptr_;
};

}  // namespace reactive

#endif  // REACTIVE_HPP_
