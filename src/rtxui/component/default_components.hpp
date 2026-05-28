// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_DEFAULT_COMPONENTS_HPP_
#define RTXUI_DEFAULT_COMPONENTS_HPP_

#include "rtxui/component/component.hpp"
#include <string_view>

namespace rtxui {

class h1 : public Component<h1> {
 public:
  std::string_view view = R"html(
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
};

class div : public Component<div> {
 public:
  std::string_view view = R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";
};

class span : public Component<span> {
 public:
  std::string_view view = R"html(
    <slot></slot>
    <style>
      self { display: inline; }
    </style>
  )html";
};

class p : public Component<p> {
 public:
  std::string_view view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        margin-top: 1;
        margin-bottom: 1;
      }
    </style>
  )html";
};

class strong : public Component<strong> {
 public:
  std::string_view view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline; 
        font-weight: bold;
      }
    </style>
  )html";
};

class ul : public Component<ul> {
 public:
  std::string_view view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        padding-left: 2;
      }
    </style>
  )html";
};

class li : public Component<li> {
 public:
  std::string_view view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
      }
    </style>
  )html";
};

class ol : public Component<ol> {
 public:
  std::string_view view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        padding-left: 2;
      }
    </style>
  )html";
};

class button : public Component<button> {
 public:
  std::string_view view = R"html(
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
};

}  // namespace rtxui

#endif  // RTXUI_DEFAULT_COMPONENTS_HPP_
