# Unicode & CJK Support

RTXUI measures text in grapheme clusters and computes display widths per
cluster, so layout works with any script the terminal can render.

## Wide Characters

Wide characters (such as Chinese, Japanese, and Korean ideographs) occupy two
terminal cell columns. RTXUI accounts for this when measuring text, so wide
characters align correctly in flexbox and grid layouts.

## Control Characters

Text is displayed, never executed. An application shows text it did not write --
a filename, a log line, a response from the network -- and a byte like `ESC` in
there is a command to the terminal rather than a character: it would repaint,
move the cursor, or recolour the interface, and everything after it on the line
would end up somewhere other than where the layout put it.

Every control character except newline and tab is therefore replaced with `U+FFFD`
(the replacement character) before measurement, so it occupies exactly the one
cell it is counted as. Newline and tab survive because layout gives them a
meaning of its own -- a hard break and a [tab stop](/guide/css/typography). A
carriage return becomes a line break, as CSS treats it.

Nothing is required of you to get this: it applies to every string a template
interpolates, whatever its source.

## Layout Alignment

Because widths are computed from display columns rather than byte or code
point counts, centering and alignment stay correct when ASCII and wide
characters are mixed on the same line.

<ExampleTabs src="/wasm/rtxui_example_cjk.js">
<template #source>

<<< @/../example/cjk.cpp

</template>
</ExampleTabs>
