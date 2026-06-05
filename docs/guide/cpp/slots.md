# Component Slots & Composition

To build modular, reusable UI layouts, RTXUI supports **Component Slots**. This allows you to write wrapper components (such as layout grids, panels, cards, or dialogs) that receive and arrange arbitrary child markup passed from their parents.

RTXUI supports both **default slots** (for simple child wrapping) and **named slots** (for multi-zone layouts).

---

## 1. Default Slots (`<slot>`)

A default slot acts as a placeholder for any child element nested inside your custom component's tag.

To declare a slot in your component's template, use the `<slot></slot>` tag:

```cpp
class MyPanel : public Component<MyPanel> {
 public:
  std::string_view view = R"html(
    <div class="panel-border">
      <slot></slot>
    </div>
  )html";
};
```

When using `MyPanel` inside a parent component, any children you nest inside `<MyPanel>` will automatically project into the `<slot></slot>` placeholder:

```html
<MyPanel>
  <p>This paragraph is projected inside the panel border.</p>
</MyPanel>
```

---

## 2. Named Slots (`<slot.name>`)

For complex components that have multiple customizable content zones (e.g. a Header, a Body, and a Footer), you can use named slots.

### Declaring Named Slots
Define slots with dot-separated names inside your component template (e.g. `<slot.header>` and `<slot.footer>`):

```cpp
class PageLayout : public Component<PageLayout> {
 public:
  std::string_view view = R"html(
    <div class="layout">
      <div class="header-zone">
        <slot.header>Default Header Content</slot.header>
      </div>
      <div class="body-zone">
        <slot></slot> <!-- Default slot for general content -->
      </div>
      <div class="footer-zone">
        <slot.footer></slot.footer>
      </div>
    </div>
  )html";
};
```

### Projecting to Named Slots
When consuming a component with named slots, wrap the projected elements inside `<template.name>` tags:

```html
<PageLayout>
  <!-- Projects to <slot.header> -->
  <template.header>
    <h1>Welcome to my App</h1>
  </template.header>

  <!-- Projects to <slot> (default slot) -->
  <p>Here is some page body content...</p>

  <!-- Projects to <slot.footer> -->
  <template.footer>
    <span>Status: Ready</span>
  </template.footer>
</PageLayout>
```

---

## Interactive Demo

Below is the live demo showcasing a reusable Card component using both named slots (for header and footer) and a default slot (for the body content):

<ExampleTabs src="/wasm/rtxui_example_slots.js">
<template #source>

<<< @/../example/slots.cpp

</template>
</ExampleTabs>
