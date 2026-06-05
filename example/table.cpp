// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>
#include "rtxui/component/default_components_internal.hpp"

using namespace rtxui;

class TableDemo : public Component<TableDemo> {
 public:
  std::string_view view = R"html(
      <div class="content">
        <h1>RTXUI Table Layout Demo</h1>
        <p>This demo showcases standard table elements (&lt;table&gt;, &lt;tr&gt;, &lt;th&gt;, &lt;td&gt;) with dynamic layout and styling.</p>

        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>Name</th>
              <th>Category</th>
              <th>Price</th>
              <th>Status</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td>#101</td>
              <td>Widget A</td>
              <td>Electronics</td>
              <td>$19.99</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#102</td>
              <td>Gadget B</td>
              <td>Appliances</td>
              <td>$49.50</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#103</td>
              <td>Tool C</td>
              <td>Hardware</td>
              <td>$5.99</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr class="alt-row">
              <td>#104</td>
              <td>Widget D</td>
              <td>Electronics</td>
              <td>$129.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#105</td>
              <td>Smartwatch E</td>
              <td>Wearables</td>
              <td>$199.99</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#106</td>
              <td>Laptop F</td>
              <td>Computers</td>
              <td>$899.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#107</td>
              <td>Keyboard G</td>
              <td>Accessories</td>
              <td>$45.00</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr class="alt-row">
              <td>#108</td>
              <td>Mouse H</td>
              <td>Accessories</td>
              <td>$25.50</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#109</td>
              <td>Monitor I</td>
              <td>Computers</td>
              <td>$249.99</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#110</td>
              <td>Desk Lamp J</td>
              <td>Furniture</td>
              <td>$35.00</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr>
              <td>#111</td>
              <td>Chair K</td>
              <td>Furniture</td>
              <td>$150.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#112</td>
              <td>Headphones L</td>
              <td>Audio</td>
              <td>$79.99</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#113</td>
              <td>Speaker M</td>
              <td>Audio</td>
              <td>$120.00</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr class="alt-row">
              <td>#114</td>
              <td>Cable N</td>
              <td>Accessories</td>
              <td>$9.99</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#115</td>
              <td>Adapter O</td>
              <td>Accessories</td>
              <td>$15.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#116</td>
              <td>Camera P</td>
              <td>Photography</td>
              <td>$450.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#117</td>
              <td>Lens Q</td>
              <td>Photography</td>
              <td>$300.00</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr class="alt-row">
              <td>#118</td>
              <td>Tripod R</td>
              <td>Photography</td>
              <td>$60.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#119</td>
              <td>Bag S</td>
              <td>Accessories</td>
              <td>$40.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#120</td>
              <td>Phone T</td>
              <td>Electronics</td>
              <td>$699.99</td>
              <td class="status active">Active</td>
            </tr>
          </tbody>
        </table>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          width: 100%;
          height: 100%;
          overflow-y: scroll;
        }
        .content {
          display: block;
          max-width: 80;
          margin: 0 auto;
        }
        h1 {
          font-weight: bold;
          margin-bottom: 1;
          color: rgb(59, 130, 246);
        }
        p {
          margin-bottom: 2;
          color: rgb(156, 163, 175);
        }
        table {
          display: block;
          border: solid;
          border-color: rgb(51, 65, 85);
          width: 100%;
        }
        tr {
          display: block;
        }
        .alt-row {
          background-color: rgb(30, 41, 59);
        }
        th {
          font-weight: bold;
          color: rgb(96, 165, 250);
          border-bottom: solid;
          border-color: rgb(51, 65, 85);
          padding: 1;
        }
        td {
          padding: 1;
        }
        .status {
          font-weight: bold;
        }
        .status.active {
          color: rgb(74, 222, 128);
        }
        .status.disabled {
          color: rgb(248, 113, 113);
        }
      </style>
    )html";

  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::h1>();
    Import<rtxui::p>();
    Component<TableDemo>::InitReflection();
  }
};

int main() {
  auto app = Ref<TableDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
