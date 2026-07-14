# Unicode & CJK Support

RTXUI measures text in grapheme clusters and computes display widths per
cluster, so layout works with any script the terminal can render.

## Wide Characters

Wide characters (such as Chinese, Japanese, and Korean ideographs) occupy two
terminal cell columns. RTXUI accounts for this when measuring text, so wide
characters align correctly in flexbox and grid layouts.

## Layout Alignment

Because widths are computed from display columns rather than byte or code
point counts, centering and alignment stay correct when ASCII and wide
characters are mixed on the same line.

<ExampleTabs src="/wasm/rtxui_example_cjk.js">
<template #source>

<<< @/../example/cjk.cpp

</template>
</ExampleTabs>
