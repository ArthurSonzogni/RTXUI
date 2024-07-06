// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "reactive.hpp"

#include <cassert>

namespace reactive {

Reactive Null() {
  return Reactive();
}

Reactive Bool(bool value) {
  return Reactive(value);
}

Reactive Int(int value) {
  return Reactive(value);
}

Reactive Double(double value) {
  return Reactive(value);
}

Reactive String(const char* value) {
  return Reactive(std::string(value));
}

Reactive String(const std::string& value) {
  return Reactive(value);
}

Reactive Array() {
  return Reactive::Array();
}

Reactive Array(std::initializer_list<Reactive> values) {
  Reactive r = Reactive::Array();
  for (const auto& value : values) {
    r.push_back(value);
  }
  return r;
}

Reactive Map() {
  return Reactive::Map();
}

Reactive Map(std::initializer_list<std::pair<std::string, Reactive>> values) {
  Reactive r = Reactive::Map();
  for (const auto& [key, value] : values) {
    r.insert(key, value);
  }
  return r;
}

Reactive Set() {
  return Reactive::Set();
}

Reactive Set(std::initializer_list<Reactive> values) {
  Reactive r = Reactive::Set();
  for (const auto& value : values) {
    r.insert(value);
  }
  return r;
}

Reactive::Reactive() : ptr_(nullptr) {}
Reactive::Reactive(bool value) : ptr_(new Value{value}) {}
Reactive::Reactive(int value) : ptr_(new Value{value}) {}
Reactive::Reactive(double value) : ptr_(new Value{value}) {}
Reactive::Reactive(const char* value) : ptr_(new Value{std::string(value)}) {}
Reactive::Reactive(const std::string& value) : ptr_(new Value{value}) {}
Reactive::Reactive(const Reactive& other) : ptr_(other.ptr_) {
  if (ptr_) {
    ptr_->ref_count++;
  }
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
  if (ptr_) {
    ptr_->ref_count++;
  }
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
  if (Type() != other.Type()) {
    return reactive::Null();
  }

  switch (Type()) {
    case Type::kNull: {
      return reactive::Null();
    }

    case Type::kBool: {
      return AsBool() && other.AsBool();
    }

    case Type::kInt: {
      return AsInt() + other.AsInt();
    }

    case Type::kDouble: {
      return AsDouble() + other.AsDouble();
    }

    case Type::kString: {
      return AsString() + other.AsString();
    }

    case Type::kArray: {
      auto out = Reactive::Array();
      for (int i = 0; i < size(); i++) {
        out.push_back((*this)[i] + other[i]);
      }
      for (int i = 0; i < other.size(); i++) {
        out.push_back((*this)[i] + other[i]);
      }
      return out;
    }

    case Type::kMap: {
      auto out = Reactive::Map();
      for (const auto& [key, value] :
           std::get<std::map<std::string, Reactive>>(ptr_->value)) {
        out.insert(key, value + other[key]);
      }
      for (const auto& [key, value] :
           std::get<std::map<std::string, Reactive>>(other.ptr_->value)) {
        out.insert(key, value + other[key]);
      }
      return out;
    }

    case Type::kSet: {
      auto out = Reactive::Set();
      for (const auto& value : std::get<std::set<Reactive>>(ptr_->value)) {
        out.insert(value + other);
      }
      for (const auto& value :
           std::get<std::set<Reactive>>(other.ptr_->value)) {
        out.insert(value + other);
      }
      return out;
    }
  }
}

Reactive Reactive::operator-(const Reactive& other) const {
  if (IsBool() && other.IsBool()) {
    return AsBool() && !other.AsBool();
  }

  if (IsInt() && other.IsInt()) {
    return AsInt() - other.AsInt();
  }

  if (IsDouble() && other.IsDouble()) {
    return AsDouble() - other.AsDouble();
  }

  return reactive::Null();
}

Reactive Reactive::operator*(const Reactive& other) const {
  if (IsBool() && other.IsBool()) {
    return AsBool() && other.AsBool();
  }

  if (IsInt() && other.IsInt()) {
    return AsInt() * other.AsInt();
  }

  if (IsDouble() && other.IsDouble()) {
    return AsDouble() * other.AsDouble();
  }

  return reactive::Null();
}

Reactive Reactive::operator/(const Reactive& other) const {
  if (IsInt() && other.IsInt()) {
    return AsInt() / other.AsInt();
  }

  if (IsDouble() && other.IsDouble()) {
    return AsDouble() / other.AsDouble();
  }

  return reactive::Null();
}

Reactive Reactive::operator%(const Reactive& other) const {
  if (IsInt() && other.IsInt()) {
    return AsInt() % other.AsInt();
  }
  return reactive::Null();
}

Reactive Reactive::operator&&(const Reactive& other) const {
  if (IsBool() && other.IsBool()) {
    return AsBool() && other.AsBool();
  }
  return reactive::Null();
}

Reactive Reactive::operator||(const Reactive& other) const {
  if (IsBool() && other.IsBool()) {
    return AsBool() || other.AsBool();
  }
  return reactive::Null();
}

Reactive Reactive::operator!() const {
  if (IsBool()) {
    return !AsBool();
  }
  if (IsInt()) {
    return !AsInt();
  }
  if (IsDouble()) {
    return !AsDouble();
  }
  return reactive::Null();
}

Reactive Reactive::operator[](int index) const {
  if (IsArray()) {
    auto& array = std::get<std::vector<Reactive>>(ptr_->value);
    if (index >= 0 && index < array.size()) {
      return array[index];
    }
  }
  return reactive::Null();
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
  return reactive::Null();
}

Iterator Reactive::begin() const {
  if (IsArray()) {
    return Iterator(std::get<std::vector<Reactive>>(ptr_->value).begin());
  }
  if (IsSet()) {
    return Iterator(std::get<std::set<Reactive>>(ptr_->value).begin());
  }
  if (IsMap()) {
    return Iterator(
        std::get<std::map<std::string, Reactive>>(ptr_->value).begin());
  }
  return Iterator();
}

Iterator Reactive::end() const {
  if (IsArray()) {
    return Iterator(std::get<std::vector<Reactive>>(ptr_->value).end());
  }
  if (IsSet()) {
    return Iterator(std::get<std::set<Reactive>>(ptr_->value).end());
  }
  if (IsMap()) {
    return Iterator(
        std::get<std::map<std::string, Reactive>>(ptr_->value).end());
  }
  return Iterator();
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

// --- Iterator ---
Iterator::Iterator() {}
Iterator::Iterator(std::vector<Reactive>::iterator it) : it_(std::move(it)) {}
Iterator::Iterator(std::set<Reactive>::iterator it) : it_(std::move(it)) {}
Iterator::Iterator(std::map<std::string, Reactive>::iterator it)
    : it_(std::move(it)) {}

Reactive& Iterator::operator*() {
  assert(it_);  // Cannot dereference the end iterator.
  return std::visit(
      [&](auto&& value) -> Reactive& {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::vector<Reactive>::iterator>) {
          return *value;
        } else if constexpr (std::is_same_v<T, std::set<Reactive>::iterator>) {
          return const_cast<Reactive&>(*value);
        } else if constexpr (std::is_same_v<T, std::map<std::string,
                                                        Reactive>::iterator>) {
          return value->second;
        }
      },
      *it_);
}

Iterator& Iterator::operator++() {
  assert(it_);  // Cannot increment the end iterator.
  std::visit([&](auto&& value) { value++; }, *it_);
  return *this;
}

bool Iterator::operator==(const Iterator& other) const {
  if (!it_ && !other.it_) {
    return true;
  }
  if (!it_ || !other.it_) {
    return false;
  }
  if (it_->index() != other.it_->index()) {
    return false;
  }
  switch (it_->index()) {
    case 0:
      return std::get<0>(*it_) == std::get<0>(*other.it_);
    case 1:
      return std::get<1>(*it_) == std::get<1>(*other.it_);
    case 2:
      return std::get<2>(*it_) == std::get<2>(*other.it_);
    default:
      assert(false);
  }
}

bool Iterator::operator!=(const Iterator& other) const {
  return !(*this == other);
}

}  // namespace reactive
