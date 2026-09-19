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
meaning of its own -- a hard break and a [tab stop](/guide/typography). A
carriage return becomes a line break, as CSS treats it.

## Malformed Text

Byte sequences that are not valid UTF-8 are replaced the same way, one `U+FFFD`
per bad byte. This is not pedantry about encodings: the engine and the terminal
have to agree on how many cells a sequence occupies, and they can only agree
about sequences that can actually be decoded.

The sharpest case is a truncated one. A lead byte announces how many bytes
follow it, so `E4 BD` at the end of a string claims a third byte that is not
there and takes whatever comes next instead -- the character after it is
absorbed and simply disappears from the display. Replacing per byte rather than
per sequence is what keeps the following character intact.

Valid text is never touched, including the parts of Unicode that are easy to
mistake for damage: wide characters, combining marks, zero-width characters and
four-byte code points all pass through as written.

## Zero-Width Characters

Format characters occupy no cell, and are measured that way: the zero-width
space and non-joiner, the bidi marks, embeddings, overrides and isolates, the
word joiner and invisible operators, and the byte-order mark. These arrive in
ordinary text -- a zero-width space pasted from a web page, a bidi mark inside a
filename, a byte-order mark at the head of a file -- and counting one as a cell
would put everything after it on that line a column to the left of where the
layout expects it.

Nothing is required of you to get any of this: it applies to every string a
template interpolates, whatever its source.

## Layout Alignment

Because widths are computed from display columns rather than byte or code
point counts, centering and alignment stay correct when ASCII and wide
characters are mixed on the same line.

<ExampleTabs src="/wasm/rtxui_example_cjk.js">
<template #source>

<<< @/../example/cjk.cpp

</template>
</ExampleTabs>
