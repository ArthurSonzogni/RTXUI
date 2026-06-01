# Markdown Component

RTXUI provides a built-in `<markdown>` component that allows you to render formatted text using Markdown syntax. It automatically converts Markdown to HTML and renders it as standard RTXUI elements.

## Basic Usage

To use the `<markdown>` component, provide your Markdown text to the `content` attribute:

```html
<markdown content="# Hello World\nThis is **bold** and *italic*." />
```

## Styling Markdown

One of the most powerful features of the `<markdown>` component is the ability to style the generated HTML tags using the `stylesheet` attribute. This stylesheet is scoped to the component and allows you to target standard tags like `h1`, `p`, `ul`, `code`, etc.

```html
<markdown 
  content="# Styled Title"
  stylesheet="h1 { color: #3b82f6; border-bottom: solid; }"
/>
```

## Example Demo

Below is an interactive demo showing the Markdown component in action. You can see how the Markdown source is rendered and styled in real-time.

<WasmTerminal example="rtxui_example_markdown" />

## Supported Syntax

The built-in parser supports:

- **Headings**: `#` through `######`
- **Paragraphs**: Separated by blank lines
- **Blockquotes**: Starting with `>`
- **Fenced Code Blocks**: Using ` ``` `
- **Lists**: Ordered (`1. `) and Unordered (`- `, `* `, `+ `)
- **Inline Formatting**: `**bold**`, `*italic*`, `` `code` ``
- **Links**: `[text](url)`

## Integration with Reactivity

Since `content` and `stylesheet` are reactive properties, you can bind them to variables in your component:

```cpp
class MyDoc : public Component<MyDoc> {
 public:
  std::string my_content = "# Dynamic Content";
  std::string my_styles = "h1 { color: red; }";

  void InitReflection() override {
    Bind(my_content);
    Bind(my_styles);
  }

  std::string_view view = R"html(
    <markdown content="{my_content}" stylesheet="{my_styles}" />
  )html";
};
```
