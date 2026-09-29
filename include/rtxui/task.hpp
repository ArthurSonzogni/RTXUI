// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TASK_HPP_
#define RTXUI_TASK_HPP_

#include <functional>
#include <rtxui/rtxui_export.hpp>

namespace rtxui {

/// Schedules `task` on the calling thread's event loop, to run after the
/// current callback returns. Must be called from a thread running an event
/// loop, i.e. the UI thread. From a worker thread, use TaskPoster().
RTXUI_EXPORT void PostTask(std::function<void()> task);

/// Returns a function that schedules a task on the calling thread's event
/// loop. Call it on the UI thread; the returned function may then be handed
/// to, and called from, any thread. Only the UI thread may touch component
/// state, so this is how a worker thread delivers its result.
RTXUI_EXPORT auto TaskPoster() -> std::function<void(std::function<void()>)>;

}  // namespace rtxui

#endif  // RTXUI_TASK_HPP_
