#include "component.hpp"

namespace rtxui {

void Component::Bind(std::string_view name, Ref<Cell> value) {
  bindings_[std::string(name)] = value;
}

}  // namespace rtxui
