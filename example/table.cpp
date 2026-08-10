// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Table elements: <table>, <thead>, <tr>, <th>, <td>, with colspan, rowspan and a
// sticky header row.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TableDemo : public Component<TableDemo> {
 public:
  std::string_view view = R"html(
      <div class="content">
        <h1>RTXUI Table Layout Demo</h1>
        <p>This demo showcases standard table elements (&lt;table&gt;, &lt;tr&gt;, &lt;th&gt;, &lt;td&gt;) with dynamic layout and styling. Scroll down to see the sticky header!</p>

        <table>
          <thead>
            <tr class="header-row">
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
              <td>Gadget A</td>
              <td>Appliances</td>
              <td>$10.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#117</td>
              <td>Widget Z</td>
              <td>Consumer Electronics</td>
              <td>$15.00</td>
              <td class="status active">Active</td>
            </tr>

            <tr>
              <td>#102</td>
              <td>Gadget B</td>
              <td>Appliances</td>
              <td>$49.50</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#103</td>
              <td>Tool C</td>
              <td>Hardware</td>
              <td>$5.99</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr>
              <td>#104</td>
              <td>Widget D</td>
              <td style="white-space: nowrap; text-overflow: ellipsis; overflow: hidden; max-width: 15;">Consumer Electronics</td>
              <td>$129.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#105</td>
              <td>Smartwatch E</td>
              <td>Wearables</td>
              <td>$199.99</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td colspan="2" class="border">colspan=2</td>
              <td>Computers</td>
              <td>$899.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#107</td>
              <td>Keyboard G</td>
              <td>Accessories</td>
              <td>$45.00</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr>
              <td rowspan="4" class="border">rowspan=4</td>
              <td>Mouse H</td>
              <td>Accessories</td>
              <td>$25.50</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>Monitor I</td>
              <td>Computers</td>
              <td>$249.99</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>Desk Lamp J</td>
              <td>Furniture</td>
              <td>$35.00</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr class="alt-row">
              <td>Chair K</td>
              <td>Furniture</td>
              <td>$150.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#112</td>
              <td>Headphones L</td>
              <td>Audio</td>
              <td>$79.99</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#113</td>
              <td>Speaker M</td>
              <td>Audio</td>
              <td>$120.00</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr>
              <td>#114</td>
              <td>Cable N</td>
              <td>Accessories</td>
              <td>$9.99</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#115</td>
              <td>Adapter O</td>
              <td>Accessories</td>
              <td>$15.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#116</td>
              <td>Camera P</td>
              <td>Photography</td>
              <td>$450.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#117</td>
              <td>Lens Q</td>
              <td>Photography</td>
              <td>$300.00</td>
              <td class="status disabled">Inactive</td>
            </tr>
            <tr>
              <td>#118</td>
              <td>Tripod R</td>
              <td>Photography</td>
              <td>$60.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr class="alt-row">
              <td>#119</td>
              <td>Bag S</td>
              <td>Accessories</td>
              <td>$40.00</td>
              <td class="status active">Active</td>
            </tr>
            <tr>
              <td>#120</td>
              <td>Phone T</td>
              <td style="white-space: nowrap; text-overflow: ellipsis; overflow: hidden; max-width: 15;">Consumer Electronics</td>
              <td>$699.99</td>
              <td class="status active">Active</td>
            </tr>
          </tbody>
        </table>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);
          --accent-bright: rgb(121, 192, 255);
          --danger: rgb(248, 81, 73);

          display: block;
          padding: 1;
          background-color: var(--bg);
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
          color: var(--accent);
        }
        p {
          margin-bottom: 2;
          color: var(--muted);
        }
        table {
          display: block;
          border: solid;
          border-color: var(--border);
          width: 100%;
        }
        tr {
          display: block;
          background-color: rgb(15, 23, 42); 
        }
        th {
          font-weight: bold;
          color: var(--accent-bright);
          border-bottom: solid;
          border-color: var(--border);
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
          color: var(--danger);
        }
        .border {
          border: tall;
          border-color: rgb(74, 222, 128);
          background-color: rgb(15, 23, 42);
          padding: 0;
        }
        tr.alt-row {
          background-color: var(--surface); 
        }
        .header-row {
          position: sticky;
          top: 0;
          z-index: 10;
          background-color: rgb(15, 23, 42);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<TableDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
