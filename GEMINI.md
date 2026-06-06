# Engineering Standards

## Testing & Quality
- **Regression Testing**: ALWAYS add a regression test for every bug encountered and fixed. These should be automated unit tests (e.g. in `src/rtxui/component/component_test.cpp` or `src/rtxui/style/style_test.cpp`) that verify the fix and prevent future regressions.
