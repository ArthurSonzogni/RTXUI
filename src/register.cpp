#include "register.hpp"

#include "components/component.hpp"
#include "core/refcounted.hpp"
#include "core/string.hpp"

namespace rtxui {

//namespace {

//struct Tree {
  //std::map<std::string_view, Tree> children;
  //std::map<std::string_view, Ref<ComponentBase>> components;
//};

//Tree& RegisteredComponents() {
  //static Tree components;
  //return components;
//}

//}  // namespace

//Register::Register(ComponentBase* component) {
  //Tree* current = &RegisteredComponents();
  ////for (auto& namespace_ : component->Namespaces()) {
    ////current = &current->children[namespace_];
  ////}
  //current->components.emplace(component->Tag(), component);
//}

//// static
//const ComponentBase* Register::Get(std::string_view name,
                               //const std::vector<std::string>& namespaces) {
  //std::vector<std::string_view> name_split = Split(name, '.');
  //name = name_split.back();
  //name_split.pop_back();

  //ComponentBase* out = nullptr;
  //Tree* current = &RegisteredComponents();

  //auto FindComponent = [&] {
    //{
      //Tree* current_current = current;
      //for (const auto& namespace_ : name_split) {
        //auto it = current_current->children.find(namespace_);
        //if (it == current_current->children.end()) {
          //current_current = nullptr;
          //break;
        //} else {
          //current_current = &it->second;
        //}
      //}
      //if (current_current) {
        //auto it = current_current->components.find(name);
        //if (it != current_current->components.end()) {
          //out = it->second.get();
        //}
      //}
    //}
  //};

  //for (const auto& namespace_ : namespaces) {
    //FindComponent();

    //auto it = current->children.find(namespace_);
    //if (it == current->children.end()) {
      //return out;
    //}
    //current = &it->second;
  //}
  //FindComponent();

  //return out;
//}

//// static
//std::vector<std::string> Register::ComputeNamespaces(
    //std::source_location location) {
  //std::string function_name = location.function_name();

  //// Split by space, ignoring the ones enclosed by <>(){}
  //std::vector<std::string> split_by_space;
  //{
    //size_t start = 0;
    //int bracket_count = 0;
    //for (size_t i = 0; i < function_name.size(); ++i) {
      //if (function_name[i] == '<' || function_name[i] == '(' ||
          //function_name[i] == '{') {
        //bracket_count++;
      //}
      //if (function_name[i] == '>' || function_name[i] == ')' ||
          //function_name[i] == '}') {
        //bracket_count--;
      //}
      //if (function_name[i] == ' ' && bracket_count == 0) {
        //split_by_space.push_back(function_name.substr(start, i - start));
        //start = i + 1;
      //}
    //}
    //split_by_space.push_back(function_name.substr(start));
  //}

  //// Find the first element that is a function
  //for (auto& s : split_by_space) {
    //if (s.find('(') == std::string::npos) {
      //continue;
    //}

    //auto namespace_ = Split(s, "::");
    //namespace_.pop_back();

    //std::vector<std::string> out;
    //for (auto& n : namespace_) {
      //if (n == "{anonymous}") {
        //continue;
      //}
      //if (n == "(anonymous namespace)") {
        //continue;
      //}
      //if (n.starts_with("RTXUI_")) {
        //continue;
      //}
      //out.emplace_back(n);
    //}
    //return out;
  //}

  //return {};
//}

//// static
//std::string Register::Print() {
  //std::string out;
  //std::function<void(const Tree&, std::string)> PrintTree;
  //PrintTree = [&](const Tree& tree, std::string prefix) {
    //for (const auto& [name, component] : tree.components) {
      //out += prefix + std::string(name) + "\n";
    //}
    //for (const auto& [name, child] : tree.children) {
      //out += prefix + std::string(name) + " {\n";
      //PrintTree(child, prefix + "  ");
      //out += prefix + "}\n";
    //}
  //};
  //PrintTree(RegisteredComponents(), "");
  //return out;
//}

}  // namespace rtxui
