# Markdown Component

The built-in `<markdown>` component renders Markdown text as regular RTXUI
elements. The Markdown source is converted to HTML internally, so the result
participates in layout and styling like any other part of the tree.

## Basic Usage

Pass the Markdown source through the `content` attribute. For anything longer
than a single line, bind a C++ string — C++ string literals give you real
newlines, which attribute text does not:

```cpp
class MyDoc : public Component<MyDoc> {
  std::string my_content = R"md(
# Hello World

This is **bold** and *italic*.
)md";

  std::string_view view = R"html(
    <markdown content="{my_content}" />
  )html";

  MyDoc() { Bind(my_content); }
};
```

Because `content` is a reactive binding, updating `my_content` re-renders the
Markdown on the next frame. This is how a live preview or a document viewer
that reloads files works — see the [complete example](#complete-example)
below, which reads a `.md` file from disk.

## Styling

The `stylesheet` attribute takes CSS scoped to the component. Target the
generated tags — `h1`, `p`, `ul`, `code`, `blockquote`, and so on:

```cpp
std::string my_styles = R"css(
  h1 { color: #3b82f6; border-bottom: solid; }
  code { background-color: #1e293b; }
)css";
```

```html
<markdown content="{my_content}" stylesheet="{my_styles}" />
```

`stylesheet` is reactive too: changing the bound string restyles the rendered
document.

## Tables

Markdown pipe-tables are parsed into standard HTML table elements (`<table>`,
`<thead>`, `<tbody>`, `<tr>`, `<th>`, `<td>`), which you can style through the
`stylesheet` attribute:

```markdown
| Name     | Role     |
|----------|----------|
| Ada      | Engineer |
| Grace    | Admiral  |
```

```css
table { border: solid; border-color: #334155; }
th { font-weight: bold; border-bottom: solid; border-color: #334155; padding-left: 1; }
td { padding-left: 1; }
```

Escaped pipes (`\|`) inside cells and inline formatting inside cells are
supported.

## Supported Syntax

The built-in parser supports:

- Headings: `#` through `######`
- Paragraphs, separated by blank lines
- Blockquotes, starting with `>`
- Fenced code blocks, using ` ``` `
- Lists: ordered (`1. `) and unordered (`- `, `* `, `+ `)
- Inline formatting: `**bold**`, `*italic*`, `` `code` ``
- Links: `[text](url)`
- Tables: pipe-tables with inline formatting and escaped pipes (`\|`)

## Complete Example

The example below loads a Markdown file from disk, renders it, and lets you
edit the stylesheet live.

<ExampleTabs src="/wasm/rtxui_example_markdown.js">
<template #source>

<<< @/../example/markdown.cpp

</template>
</ExampleTabs>
