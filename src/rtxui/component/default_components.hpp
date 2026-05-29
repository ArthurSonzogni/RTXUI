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

class hr : public Component<hr> {
 public:
  std::string line_chars;

  void InitReflection() override;
  std::string_view Setup() override;
  bool Digest() override;
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

class TextInputBase {
 public:
  std::string value;
  int cursor_pos = 0;

  // Render bindings
  std::string left_text;
  std::string cursor_char;
  std::string right_text;
  std::string cursor_class = "cursor";

 protected:
  bool is_focused_ = false;
  int ideal_column_ = 0;

  void KeepCursorVisible(Element* root, bool is_multiline);
  bool OnEventShared(ComponentBase* self, Event event, bool is_multiline);
  bool DigestShared(ComponentBase* self);
};

class input : public Component<input>, public TextInputBase {
 public:
  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;
};

class textarea : public Component<textarea>, public TextInputBase {
 public:
  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;
};

class checkbox : public Component<checkbox> {
 public:
  bool checked = false;
  std::string checked_char = " ";
  std::string focus_class = "";

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;
};

class slider : public Component<slider> {
 public:
  int value = 0;
  int min = 0;
  int max = 100;
  int step = 1;
  int width = 20;

  // Render bindings
  std::string track_left;
  std::string thumb_char = "●";
  std::string track_right;
  std::string focus_class = "";

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;
};

class progress : public Component<progress> {
 public:
  double value = 0;
  double max = 100;
  int width = 20;

  // Render bindings
  std::string filled_track;
  std::string empty_track;

  void InitReflection() override;
  std::string_view Setup() override;
  bool Digest() override;
};

struct OptionInfo {
  std::string value;
  std::string label;
  Element* element = nullptr;
};

class select : public Component<select> {
 public:
  std::string value;
  bool is_open = false;
  int hovered_index = -1;

  // Render bindings
  std::string selected_label;
  std::string arrow_char = "▾";
  std::string dropdown_class = "closed";
  std::string focus_class = "";

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;

  void SelectOption(std::string_view value);
  std::vector<OptionInfo> GetOptions();
};

class option : public Component<option> {
 public:
  std::string value;

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
};

}  // namespace rtxui

#endif  // RTXUI_DEFAULT_COMPONENTS_HPP_
