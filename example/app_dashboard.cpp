// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// A complete application, rather than a demo of one property.
//
// Everything here has appeared on its own elsewhere in example/ -- reactive
// state, computed values, <for> over a collection of structs, conditional
// rendering, flexbox, grid, transitions and custom properties. This file is
// about how they compose into something you would actually ship.
//
// Try it: click a service row to select it, use the filter buttons to narrow
// the list, and press Restart to watch a row transition back to healthy.
#include <rtxui/rtxui.hpp>

#include <string>
#include <vector>

using namespace rtxui;

namespace {

struct Service {
  std::string name;
  std::string region;
  std::string state;  // "healthy" | "degraded" | "down"
  int latency_ms = 0;
  int load_pct = 0;

  bool operator==(const Service& other) const = default;
};

}  // namespace

class Dashboard : public Component<Dashboard> {
 public:
  std::vector<Service> services = {
      {"api-gateway", "us-east-1", "healthy", 42, 61},
      {"auth-service", "us-east-1", "healthy", 18, 34},
      {"search-index", "eu-west-1", "degraded", 310, 88},
      {"media-encoder", "eu-west-1", "down", 0, 0},
      {"billing-worker", "us-west-2", "healthy", 27, 45},
      {"notification-bus", "ap-south-1", "degraded", 154, 72},
  };

  std::string filter = "all";
  std::string selected = "api-gateway";

  // Computed values: recomputed whenever the state above changes.
  std::string healthy_count() const { return std::to_string(Count("healthy")); }
  std::string degraded_count() const {
    return std::to_string(Count("degraded"));
  }
  std::string down_count() const { return std::to_string(Count("down")); }
  std::string total_count() const {
    return std::to_string(services.size());
  }

  std::string selected_name() const { return selected; }
  std::string selected_region() const {
    const Service* service = Find(selected);
    return service ? service->region : "-";
  }
  std::string selected_state() const {
    const Service* service = Find(selected);
    return service ? service->state : "-";
  }
  std::string selected_latency() const {
    const Service* service = Find(selected);
    return service ? std::to_string(service->latency_ms) + " ms" : "-";
  }
  std::string selected_load() const {
    const Service* service = Find(selected);
    return service ? std::to_string(service->load_pct) : "0";
  }
  bool selected_is_down() const {
    const Service* service = Find(selected);
    return service && service->state != "healthy";
  }

  std::string filter_all_class() const {
    return filter == "all" ? "chip active" : "chip";
  }
  std::string filter_degraded_class() const {
    return filter == "degraded" ? "chip active" : "chip";
  }
  std::string filter_down_class() const {
    return filter == "down" ? "chip active" : "chip";
  }

  void ShowAll() { filter = "all"; }
  void ShowDegraded() { filter = "degraded"; }
  void ShowDown() { filter = "down"; }

  void Restart() {
    Service* service = Find(selected);
    if (service) {
      service->state = "healthy";
      service->latency_ms = 30;
      service->load_pct = 40;
    }
  }

  std::string_view view = R"html(
      <div class="app">
        <div class="header">
          <span class="brand">RTXUI</span>
          <span class="title">Service Health</span>
          <span class="spacer"></span>
          <span class="pill healthy">{healthy_count} healthy</span>
          <span class="pill degraded">{degraded_count} degraded</span>
          <span class="pill down">{down_count} down</span>
        </div>

        <div class="body">
          <div class="list">
            <div class="filters">
              <span class="{filter_all_class}" onclick="ShowAll">All</span>
              <span class="{filter_degraded_class}" onclick="ShowDegraded">Degraded</span>
              <span class="{filter_down_class}" onclick="ShowDown">Down</span>
            </div>

            <div class="rows">
              <for each="{services}" as="service">
                <div class="row {service.row_class}" onclick="Select({$index})">
                  <span class="dot {service.state}">*</span>
                  <span class="name">{service.name}</span>
                  <span class="region">{service.region}</span>
                  <span class="latency">{service.latency}</span>
                </div>
              </for>
            </div>
          </div>

          <div class="detail">
            <span class="detail-title">{selected_name}</span>
            <span class="detail-sub">{selected_region}</span>

            <div class="kv"><span class="k">State</span><span class="v">{selected_state}</span></div>
            <div class="kv"><span class="k">Latency</span><span class="v">{selected_latency}</span></div>
            <div class="kv"><span class="k">Load</span><span class="v">{selected_load}%</span></div>

            <div class="meter">
              <progress value="{selected_load}" max="100" width="26" />
            </div>

            <if condition="{selected_is_down}">
              <div class="alert">This service needs attention.</div>
              <button onclick="Restart">Restart service</button>
            </if>
          </div>
        </div>

        <div class="footer">
          <span>{total_count} services</span>
          <span class="spacer"></span>
          <span class="hint">click a row to select   .   filter above</span>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --raised: rgb(31, 38, 47);
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);
          --healthy: rgb(63, 185, 80);
          --degraded: rgb(210, 153, 34);
          --down: rgb(248, 81, 73);

          display: block;
          width: 100%;
          height: 100%;
          background-color: var(--bg);
          color: rgb(230, 237, 243);
        }
        .app {
          display: flex;
          flex-direction: column;
          height: 100%;
        }

        .header {
          display: flex;
          align-items: center;
          gap: 2;
          background-color: var(--surface);
          border-bottom: solid;
          border-color: var(--border);
          padding: 0 2;
        }
        .brand {
          color: var(--accent);
          font-weight: bold;
        }
        .title {
          color: var(--muted);
        }
        .spacer {
          flex-grow: 1;
        }
        .pill {
          padding: 0 1;
          font-weight: bold;
        }
        .pill.healthy { color: var(--healthy); }
        .pill.degraded { color: var(--degraded); }
        .pill.down { color: var(--down); }

        .body {
          display: flex;
          flex-grow: 1;
          gap: 1;
          padding: 1 2;
        }

        .list {
          display: flex;
          flex-direction: column;
          flex-grow: 1;
        }
        .filters {
          display: flex;
          gap: 1;
          margin-bottom: 1;
        }
        .chip {
          border: round;
          border-color: var(--border);
          color: var(--muted);
          padding: 0 1;
          transition: border-color 0.2s ease, color 0.2s ease;
        }
        .chip:hover {
          border-color: var(--accent);
          color: var(--accent);
        }
        .chip.active {
          border-color: var(--accent);
          color: var(--accent);
          font-weight: bold;
        }

        .rows {
          display: flex;
          flex-direction: column;
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 0 1;
          flex-grow: 1;
          overflow-y: auto;
        }
        .row {
          display: flex;
          gap: 1;
          align-items: center;
          padding: 0 1;
          transition: background-color 0.2s ease;
        }
        .row:hover {
          background-color: var(--raised);
        }
        .row.selected {
          background-color: var(--raised);
          border-left: tall;
          border-color: var(--accent);
        }
        .row.hidden {
          display: none;
        }
        .dot.healthy { color: var(--healthy); }
        .dot.degraded { color: var(--degraded); }
        .dot.down { color: var(--down); }
        .name {
          width: 18;
          font-weight: bold;
        }
        .region {
          width: 12;
          color: var(--muted);
        }
        .latency {
          flex-grow: 1;
          color: var(--muted);
          text-align: right;
        }

        .detail {
          display: flex;
          flex-direction: column;
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 1 2;
          width: 34;
        }
        .detail-title {
          color: var(--accent);
          font-weight: bold;
        }
        .detail-sub {
          color: var(--muted);
          margin-bottom: 1;
        }
        .kv {
          display: flex;
          justify-content: space-between;
          width: 100%;
        }
        .k { color: var(--muted); }
        .v { font-weight: bold; }
        .meter {
          margin-top: 1;
        }
        .alert {
          color: var(--down);
          margin-top: 1;
        }
        button {
          border: tall;
          border-color: var(--down);
          background-color: var(--bg);
          color: var(--down);
          padding: 0 1;
          margin-top: 1;
          text-align: center;
          transition: background-color 0.2s ease, color 0.2s ease;
        }
        button:hover, button:focus {
          background-color: var(--down);
          color: var(--bg);
        }

        .footer {
          display: flex;
          align-items: center;
          gap: 2;
          border-top: solid;
          border-color: var(--border);
          background-color: var(--surface);
          color: var(--muted);
          padding: 0 2;
        }
        .hint {
          color: var(--border);
        }
      </style>
    )html";

  Dashboard() {
    BindCollection("services", &services, [this](const Service& service) {
      const bool visible = filter == "all" || filter == service.state;
      std::string row_class = service.name == selected ? "selected" : "";
      if (!visible) {
        row_class += " hidden";
      }
      return std::make_shared<ManualStructVisitor>(
          std::map<std::string, std::string, std::less<>>{
              {"name", service.name},
              {"region", service.region},
              {"state", service.state},
              {"row_class", row_class},
              {"latency", service.state == "down"
                              ? std::string("--")
                              : std::to_string(service.latency_ms) + " ms"},
          });
    });

    Bind(filter);
    Bind(selected);
    Bind(healthy_count);
    Bind(degraded_count);
    Bind(down_count);
    Bind(total_count);
    Bind(selected_name);
    Bind(selected_region);
    Bind(selected_state);
    Bind(selected_latency);
    Bind(selected_load);
    Bind(selected_is_down);
    Bind(filter_all_class);
    Bind(filter_degraded_class);
    Bind(filter_down_class);
    Bind(ShowAll);
    Bind(ShowDegraded);
    Bind(ShowDown);
    Bind(Restart);

    Import("Select", [this](std::string index_str) {
      size_t index = std::stoull(index_str);
      if (index < services.size()) {
        selected = services[index].name;
      }
    });
  }

 private:
  int Count(std::string_view state) const {
    int total = 0;
    for (const Service& service : services) {
      total += service.state == state ? 1 : 0;
    }
    return total;
  }

  const Service* Find(const std::string& name) const {
    for (const Service& service : services) {
      if (service.name == name) {
        return &service;
      }
    }
    return nullptr;
  }

  Service* Find(const std::string& name) {
    return const_cast<Service*>(std::as_const(*this).Find(name));
  }
};

int main() {
  auto app = Ref<Dashboard>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
