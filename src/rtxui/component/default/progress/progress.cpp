// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/progress/progress.hpp"
#include <algorithm>
#include <cmath>

namespace rtxui {

void progress::InitReflection() {
  Bind(value);
  Bind(max);
  Bind(width);
  Bind(filled_track);
  Bind(empty_track);
  Component<progress>::InitReflection();
}

std::string_view progress::Setup() {
  return R"html(<span class="filled">{filled_track}</span><span class="empty">{empty_track}</span><style>
      self {
        display: inline-block;
      }
      .filled {
        color: #38bdf8;
      }
      .empty {
        color: #444;
      }
    </style>)html";
}

bool progress::Digest() {
  int track_w = std::max(1, width);
  double range = max;
  int pos = 0;
  if (range > 0) {
    pos = static_cast<int>(
        std::round(std::clamp(value / range, 0.0, 1.0) * track_w));
  }
  pos = std::clamp(pos, 0, track_w);

  filled_track = "";
  for (int i = 0; i < pos; ++i) {
    filled_track += "█";
  }
  empty_track = "";
  for (int i = pos; i < track_w; ++i) {
    empty_track += " ";
  }

  return Component<progress>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("progress", []() { return Ref<progress>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
