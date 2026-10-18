// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Table elements: <table>, <thead>, <tr>, <th>, <td>, with colspan, rowspan and
// a sticky header row.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

struct Product {
  std::string name;
  std::string category;
  std::string price;
  bool active = true;
  bool operator==(const Product& other) const = default;
};

void Fields(const Product& product, FieldSink& out) {
  out("name", product.name);
  out("category", product.category);
  out("price", product.price);
  out("status", product.active ? "active" : "inactive");
}

class TableDemo : public Component<TableDemo> {
 public:
  std::vector<Product> products = {
      {"Gadget", "Appliances", "$10.00"},
      {"Widget", "Consumer Electronics", "$15.00"},
      {"Tool", "Hardware", "$5.99", false},
      {"Smartwatch", "Wearables", "$199.99"},
      {"Keyboard", "Accessories", "$45.00", false},
      {"Monitor", "Computers", "$249.99"},
      {"Desk lamp", "Furniture", "$35.00", false},
      {"Headphones", "Audio", "$79.99"},
      {"Camera", "Photography", "$450.00"},
      {"Tripod", "Photography", "$60.00"},
      {"Phone", "Consumer Electronics", "$699.99"},
  };

  std::string_view view = R"html(
      <div class="card">
        <h1>Tables</h1>
        <p>Columns size to their content. Scroll the list: the header row is sticky.</p>

        <div class="scroller">
          <table>
            <thead>
              <tr class="header">
                <th>Name</th>
                <th>Category</th>
                <th>Price</th>
                <th>Status</th>
              </tr>
            </thead>
            <tbody>
              <for each="{products}" as="p">
                <tr>
                  <td>{p.name}</td>
                  <td class="muted">{p.category}</td>
                  <td>{p.price}</td>
                  <td class="{p.status}">{p.status}</td>
                </tr>
              </for>
            </tbody>
          </table>
        </div>

        <p>Cells span columns with colspan and rows with rowspan.</p>
        <table class="spans">
          <tr>
            <td rowspan="2" class="span">rowspan="2"</td>
            <td>A</td>
            <td>B</td>
          </tr>
          <tr>
            <td colspan="2" class="span">colspan="2"</td>
          </tr>
        </table>
      </div>

      <style>
        self {
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 64;
        }
        h1 {
          color: var(--accent);
        }
        p, .muted {
          color: var(--muted);
        }
        .scroller {
          height: 9;
          overflow-y: scroll;
          border: round;
          border-color: var(--border);
        }
        .header {
          position: sticky;
          top: 0;
          background-color: rgb(22, 45, 77);
        }
        th {
          color: var(--accent);
          text-align: left;
        }
        th, td {
          padding: 0 1;
        }
        .active {
          color: rgb(63, 185, 80);
        }
        .inactive {
          color: rgb(248, 81, 73);
        }
        .spans td {
          border: round;
          border-color: var(--border);
        }
        .spans .span {
          border-color: var(--accent);
          color: var(--accent);
        }
      </style>
    )html";

  TableDemo() { Bind(products); }
};

int main() {
  auto app = Ref<TableDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
