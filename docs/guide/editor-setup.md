# Editor Syntax Highlighting

Component templates live in C++ raw string literals. By default an editor
shows them as a plain string. With a little setup it can highlight the HTML
inside, and the CSS inside `<style>` blocks, as if they were separate files.

## Name the Language in the Delimiter

A raw string literal can carry a delimiter between the quote and the
parenthesis. Use it to name the embedded language:

```cpp
class Counter : public rtxui::Component<Counter> {
 public:
  std::string_view view = R"html(
    <div class="counter">Hello</div>
    <style>
      .counter { color: rgb(59, 130, 246); }
    </style>
  )html";
};
```

The delimiter has no effect on the compiled program, but editors that
understand language injection use it to pick a parser. Supported names are
the tree-sitter parser names: `html`, `css`, `xml`, `markdown`. A bare `R"(`
carries no language and stays a plain string.

## Neovim

[nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter) already
ships a C++ injection rule that parses a raw string's content with the parser
named by its delimiter. Two things are needed:

1. **Enable tree-sitter highlighting.** In the nvim-treesitter setup:

   ```lua
   require("nvim-treesitter.configs").setup({
     highlight = { enable = true },
   })
   ```

2. **Install the parsers**: `:TSInstall cpp html css`. With `auto_install =
   true` they are installed the first time a matching file is opened.

The HTML parser injects CSS into `<style>` elements on its own, so styles
inside an `R"html(` template are highlighted too.

To check that it works, put the cursor on a tag inside a template and run
`:Inspect`. It should list captures such as `@tag.html`.
