// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TESTING_ENVIRONMENT_HPP_
#define RTXUI_TESTING_ENVIRONMENT_HPP_

#include <cstdlib>

namespace rtxui::testing {

// setenv() and unsetenv() are POSIX; Windows spells them _putenv_s(), where
// an empty value removes the variable.
inline void SetEnv(const char* name, const char* value) {
#if defined(_WIN32)
  _putenv_s(name, value);
#else
  setenv(name, value, /*overwrite=*/1);
#endif
}

inline void UnsetEnv(const char* name) {
#if defined(_WIN32)
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}

}  // namespace rtxui::testing

#endif  // RTXUI_TESTING_ENVIRONMENT_HPP_
