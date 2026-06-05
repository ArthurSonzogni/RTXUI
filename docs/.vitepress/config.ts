import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'RTXUI',
  description: 'C++ Reactive Terminal Rendering Engine',
  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Guide', link: '/guide/hello-world' },
      { text: 'Reference', link: '/cpp_api' }
    ],
    sidebar: [
      {
        text: 'Introduction',
        items: [
          { text: 'Getting Started', link: '/' },
          { text: 'Hello World', link: '/guide/hello-world' },
          { text: 'Reactive Model', link: '/reactivity' }
        ]
      },
      {
        text: 'Core Concepts (HTML & Logic)',
        collapsed: false,
        items: [
          { text: 'Value Interpolation', link: '/guide/interpolation' },
          { text: 'Event Handlers & Inputs', link: '/guide/bindings' },
          { text: 'Conditional Rendering', link: '/guide/conditionals' },
          { text: 'Loops & Lists', link: '/guide/loops' },
          { text: 'Focus & Tab Navigation', link: '/guide/html/focus' },
          { text: 'Form Elements', link: '/guide/forms' }
        ]
      },
      {
        text: 'C++ Integration Guide',
        collapsed: true,
        items: [
          { text: 'Creating Components', link: '/guide/cpp/components' },
          { text: 'Component Slots & Composition', link: '/guide/cpp/slots' },
          { text: 'State & Collections Reflection', link: '/guide/cpp/bindings' },
          { text: 'Navigating the DOM', link: '/guide/cpp/dom' },
          { text: 'Screen Loop & Lifecycle', link: '/guide/cpp/lifecycle' }
        ]
      },
      {
        text: 'CSS & Layout Guide',
        collapsed: true,
        items: [
          { text: 'Typography & Text Styling', link: '/guide/typography' },
          { text: 'Styling Basics & Selectors', link: '/guide/css/basics' },
          { text: 'Box Model & Spacing', link: '/guide/css/box-model' },
          { text: 'Flexbox Layouts', link: '/guide/css/flexbox' },
          { text: 'Positioning & Layers', link: '/guide/css/positioning' },
          { text: 'Responsive Media Queries', link: '/guide/css/media-queries' },
          { text: 'Transitions & Animations', link: '/guide/css/animations' }
        ]
      },
      {
        text: 'Specialized Features',
        collapsed: true,
        items: [
          { text: 'Scrolling Containers', link: '/guide/scrolling' },
          { text: 'Unicode & CJK', link: '/guide/unicode' },
          { text: 'Markdown rendering', link: '/guide/markdown' }
        ]
      },
      {
        text: 'Reference Manual',
        collapsed: false,
        items: [
          { text: 'C++ API Reference', link: '/cpp_api' },
          { text: 'HTML Element Reference', link: '/html_reference' },
          { text: 'CSS Attribute Reference', link: '/css_reference' }
        ]
      }
    ],
    socialLinks: [
      { icon: 'github', link: 'https://github.com/ArthurSonzogni/RTXUI' }
    ]
  }
})
