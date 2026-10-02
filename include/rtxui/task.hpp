// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TASK_HPP_
#define RTXUI_TASK_HPP_

#include <functional>
#include <rtxui/rtxui_export.hpp>

namespace rtxui {

/// Schedules `task` to run on the application's event loop, after the current
/// callback returns. Safe to call from any thread: on a thread that runs an
/// event loop it posts to that loop; from any other thread (a worker) it posts
/// to the application's, which is how a worker hands back its result, since
/// only the UI thread may touch component state. A task posted after the
/// event loop is gone is dropped.
///
///   std::thread([] {
///     auto result = SlowWork();
///     rtxui::PostTask([result] { /* update bound state */ });
///   }).detach();
RTXUI_EXPORT void PostTask(std::function<void()> task);

}  // namespace rtxui

#endif  // RTXUI_TASK_HPP_
