#include "reactive.hpp"
#include <cassert>

namespace reactive {

Reactive Array() {
  return Reactive::Array();
}
Reactive Map() {
  return Reactive::Map();
}
Reactive Set() {
  return Reactive::Set();
}

Reactive::Reactive() : ptr_(nullptr) {}
Reactive::Reactive(bool value) : ptr_(new Value{value}) {}
Reactive::Reactive(int value) : ptr_(new Value{value}) {}
Reactive::Reactive(double value) : ptr_(new Value{value}) {}
Reactive::Reactive(const char* value) : ptr_(new Value{std::string(value)}) {}
Reactive::Reactive(const std::string& value) : ptr_(new Value{value}) {}
Reactive::Reactive(const Reactive& other) : ptr_(other.ptr_) {
  ptr_->ref_count++;
}
Reactive::Reactive(Reactive&& other) : ptr_(other.ptr_) {
  other.ptr_ = nullptr;
}

// static
Reactive Reactive::Array() {
  return Reactive(std::vector<Reactive>());
}

// static
Reactive Reactive::Map() {
  return Reactive(std::map<std::string, Reactive>());
}

// static
Reactive Reactive::Set() {
  return Reactive(std::set<Reactive>());
}

// Destructor
Reactive::~Reactive() {
  AssignNull();
}

// Type checkers
Type Reactive::Type() const {
  if (IsNull()) {
    return Type::kNull;
  }
  switch (ptr_->value.index()) {
    case 0:
      return Type::kBool;
    case 1:
      return Type::kInt;
    case 2:
      return Type::kDouble;
    case 3:
      return Type::kString;
    case 4:
      return Type::kArray;
    case 5:
      return Type::kMap;
    case 6:
      return Type::kSet;
    default:
      assert(false);
  }
}

bool Reactive::IsNull() const {
  return ptr_ == nullptr;
}
bool Reactive::IsBool() const {
  return !IsNull() && std::holds_alternative<bool>(ptr_->value);
}
bool Reactive::IsInt() const {
  return !IsNull() && std::holds_alternative<int>(ptr_->value);
}
bool Reactive::IsDouble() const {
  return !IsNull() && std::holds_alternative<double>(ptr_->value);
}
bool Reactive::IsString() const {
  return !IsNull() && std::holds_alternative<std::string>(ptr_->value);
}
bool Reactive::IsArray() const {
  return !IsNull() &&
         std::holds_alternative<std::vector<Reactive>>(ptr_->value);
}
bool Reactive::IsMap() const {
  return !IsNull() &&
         std::holds_alternative<std::map<std::string, Reactive>>(ptr_->value);
}
bool Reactive::IsSet() const {
  return !IsNull() && std::holds_alternative<std::set<Reactive>>(ptr_->value);
}

// Value getters
bool Reactive::AsBool() const {
  return std::get<bool>(ptr_->value);
}
int Reactive::AsInt() const {
  return std::get<int>(ptr_->value);
}
double Reactive::AsDouble() const {
  return std::get<double>(ptr_->value);
}
std::string Reactive::AsString() const {
  return std::get<std::string>(ptr_->value);
}

Reactive& Reactive::operator=(const Reactive& other) {
  AssignNull();
  ptr_ = other.ptr_;
  ptr_->ref_count++;
  return *this;
}

Reactive& Reactive::operator=(Reactive&& other) {
  AssignNull();
  ptr_ = other.ptr_;
  other.ptr_ = nullptr;
  return *this;
}

bool Reactive::operator==(const Reactive& other) const {
  return ptr_ == other.ptr_;
}
bool Reactive::operator!=(const Reactive& other) const {
  return ptr_ != other.ptr_;
}

void Reactive::operator=(bool value) {
  AssignNull();
  ptr_ = new Value{value};
}

Reactive Reactive::operator+(const Reactive& other) const {
  if (IsInt() && other.IsInt()) {
    return AsInt() + other.AsInt();
  }
  if (IsDouble() && other.IsDouble()) {
    return AsDouble() + other.AsDouble();
  }
  if (IsString() && other.IsString()) {
    return AsString() + other.AsString();
  }
  return Reactive();
}

Reactive Reactive::operator-(const Reactive& other) const {
  if (IsInt() && other.IsInt()) {
    return AsInt() - other.AsInt();
  }
  if (IsDouble() && other.IsDouble()) {
    return AsDouble() - other.AsDouble();
  }
  return Reactive();
}

Reactive Reactive::operator*(const Reactive& other) const {
  if (IsInt() && other.IsInt()) {
    return AsInt() * other.AsInt();
  }
  if (IsDouble() && other.IsDouble()) {
    return AsDouble() * other.AsDouble();
  }
  return Reactive();
}

Reactive Reactive::operator/(const Reactive& other) const {
  if (IsInt() && other.IsInt()) {
    return AsInt() / other.AsInt();
  }
  if (IsDouble() && other.IsDouble()) {
    return AsDouble() / other.AsDouble();
  }
  return Reactive();
}

Reactive Reactive::operator%(const Reactive& other) const {
  if (IsInt() && other.IsInt()) {
    return AsInt() % other.AsInt();
  }
  return Reactive();
}

Reactive Reactive::operator&&(const Reactive& other) const {
  if (IsBool() && other.IsBool()) {
    return AsBool() && other.AsBool();
  }
  return Reactive();
}

Reactive Reactive::operator||(const Reactive& other) const {
  if (IsBool() && other.IsBool()) {
    return AsBool() || other.AsBool();
  }
  return Reactive();
}

Reactive Reactive::operator!() const {
  if (IsBool()) {
    return !AsBool();
  }
  return Reactive();
}

Reactive Reactive::operator[](int index) const {
  if (IsArray()) {
    auto& array = std::get<std::vector<Reactive>>(ptr_->value);
    if (index >= 0 && index < array.size()) {
      return array[index];
    }
  }
  return Reactive();
}

Reactive Reactive::operator[](const char* key) const {
  return operator[](std::string(key));
}
Reactive Reactive::operator[](const std::string& key) const {
  if (IsMap()) {
    auto& map = std::get<std::map<std::string, Reactive>>(ptr_->value);
    auto it = map.find(key);
    if (it != map.end()) {
      return it->second;
    }
  }
  return Reactive();
}

void Reactive::push_back(const Reactive& value) {
  if (IsArray()) {
    std::get<std::vector<Reactive>>(ptr_->value).push_back(value);
  }
}
void Reactive::insert(const std::string& key, const Reactive& value) {
  if (IsMap()) {
    std::get<std::map<std::string, Reactive>>(ptr_->value).insert({key, value});
  }
}
void Reactive::insert(const Reactive& value) {
  if (IsSet()) {
    std::get<std::set<Reactive>>(ptr_->value).insert(value);
  }
}
void Reactive::erase(const std::string& key) {
  if (IsMap()) {
    std::get<std::map<std::string, Reactive>>(ptr_->value).erase(key);
  }
}
void Reactive::erase(int index) {
  if (IsArray()) {
    auto& array = std::get<std::vector<Reactive>>(ptr_->value);
    if (index >= 0 && index < array.size()) {
      array.erase(array.begin() + index);
    }
  }
}
void Reactive::clear() {
  if (IsArray()) {
    std::get<std::vector<Reactive>>(ptr_->value).clear();
  }
  if (IsMap()) {
    std::get<std::map<std::string, Reactive>>(ptr_->value).clear();
  }
  if (IsSet()) {
    std::get<std::set<Reactive>>(ptr_->value).clear();
  }
}
int Reactive::size() const {
  if (IsArray()) {
    return std::get<std::vector<Reactive>>(ptr_->value).size();
  }
  if (IsMap()) {
    return std::get<std::map<std::string, Reactive>>(ptr_->value).size();
  }
  if (IsSet()) {
    return std::get<std::set<Reactive>>(ptr_->value).size();
  }
  return 0;
}
bool Reactive::empty() const {
  return size() == 0;
}

Reactive::Reactive(std::vector<Reactive> value)
    : ptr_(new Value{std::move(value)}) {}
Reactive::Reactive(std::map<std::string, Reactive> value)
    : ptr_(new Value{std::move(value)}) {}
Reactive::Reactive(std::set<Reactive> value)
    : ptr_(new Value{std::move(value)}) {}

void Reactive::AssignNull() {
  if (ptr_ && --ptr_->ref_count == 0) {
    delete ptr_;
  }
  ptr_ = nullptr;
}

}  // namespace reactive
