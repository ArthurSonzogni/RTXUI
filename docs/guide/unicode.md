# Unicode & CJK Support

RTXUI includes native support for Unicode grapheme cluster parsing and string layout width calculation.

### Wide Characters

Wide characters (such as Chinese, Japanese, and Korean) take up exactly two terminal cell columns. RTXUI correctly calculates these widths to ensure they align properly in flexbox and grid layouts.

```cpp
class CJKApp : public Component<CJKApp> {
 public:
  std::string_view view = R"html(
      <div class="container">
        <div>Chinese: 中文 (Width = 4 cells)</div>
        <div>Japanese: 日本語 (Width = 6 cells)</div>
        <div>Korean: 한국어 (Width = 6 cells)</div>
      </div>
    )html";

  CJKApp() {
    Import<rtxui::div>();
  }
};
```

### Layout Alignment

Because RTXUI understands character widths, it can accurately center text or justify content even when mixing ASCII and Unicode characters.

<ExampleTabs src="/wasm/rtxui_example_cjk.js">
<template #source>

<<< @/../example/cjk.cpp

</template>
</ExampleTabs>
