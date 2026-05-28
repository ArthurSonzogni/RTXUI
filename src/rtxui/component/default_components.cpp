#include "rtxui/component/default_components.hpp"

namespace rtxui {

std::string_view h1::Setup() {
  return R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        font-weight: bold;
        text-decoration: underlined;
        margin-bottom: 1;
      }
    </style>
  )html";
}

std::string_view div::Setup() {
  return R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";
}

std::string_view span::Setup() {
  return R"html(
    <slot></slot>
    <style>
      self { display: inline; }
    </style>
  )html";
}

std::string_view p::Setup() {
  return R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        margin-top: 1;
        margin-bottom: 1;
      }
    </style>
  )html";
}

std::string_view strong::Setup() {
  return R"html(
    <slot></slot>
    <style>
      self { 
        display: inline; 
        font-weight: bold;
      }
    </style>
  )html";
}

std::string_view ul::Setup() {
  return R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        padding-left: 2;
      }
    </style>
  )html";
}

std::string_view li::Setup() {
  return R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
      }
    </style>
  )html";
}

std::string_view ol::Setup() {
  return R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        padding-left: 2;
      }
    </style>
  )html";
}

std::string_view button::Setup() {
  return R"html(
    <slot></slot>
    <style>
      self { 
        display: inline-block; 
        border: tall;
        padding-left: 1;
        padding-right: 1;
      }
    </style>
  )html";
}

}  // namespace rtxui
