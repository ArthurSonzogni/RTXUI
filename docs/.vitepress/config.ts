import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid({
  title: 'RTXUI',
  description: 'C++ Reactive Terminal Rendering Engine',
  themeConfig: {
    nav: [
      { text: 'Guide', link: '/guide/hello-world', activeMatch: '/guide/' },
      { text: 'API Reference', link: '/cpp_api', activeMatch: '/(cpp_api|html_reference|css_reference)' },
      { text: 'Interactive Demos', link: '/guide/examples' },
      { text: 'Hot Reloading', link: '/guide/hot-reload' }
    ],
    sidebar: [
      {
        text: '🚀 Essentials',
        items: [
          { text: 'Introduction', link: '/' },
          { text: 'Getting Started', link: '/guide/getting-started' },
          { text: 'Hello World', link: '/guide/hello-world' },
          { text: 'How Reactivity Works', link: '/reactivity' }
        ]
      },
      {
        text: '🛠️ Templates & Logic (HTML)',
        collapsed: false,
        items: [
          { text: 'Value Interpolation', link: '/guide/interpolation' },
          { text: 'Event Handlers & Bindings', link: '/guide/bindings' },
          { text: 'Conditional Rendering', link: '/guide/conditionals' },
          { text: 'Loops & Lists', link: '/guide/loops' },
          { text: 'Focus & Tab Navigation', link: '/guide/html/focus' },
          { text: 'Form & Input Controls', link: '/guide/forms' }
        ]
      },
      {
        text: '🎨 Styling & Layout (CSS)',
        collapsed: true,
        items: [
          { text: 'Styling Basics & Selectors', link: '/guide/css/basics' },
          { text: 'Typography & Text Styling', link: '/guide/typography' },
          { text: 'Box Model & Spacing', link: '/guide/css/box-model' },
          { text: 'Flexbox Layouts', link: '/guide/css/flexbox' },
          { text: 'Positioning & Layers', link: '/guide/css/positioning' },
          { text: 'Scrolling Containers', link: '/guide/scrolling' },
          { text: 'Transitions & Animations', link: '/guide/css/animations' },
          { text: 'Responsive Media Queries', link: '/guide/css/media-queries' }
        ]
      },
      {
        text: '💻 C++ Integration Guide',
        collapsed: true,
        items: [
          { text: 'Creating Custom Components', link: '/guide/cpp/components' },
          { text: 'Slots & Component Composition', link: '/guide/cpp/slots' },
          { text: 'State & Reflection Binding', link: '/guide/cpp/bindings' },
          { text: 'Navigating the DOM Tree', link: '/guide/cpp/dom' },
          { text: 'Screen Loop & Lifecycle', link: '/guide/cpp/lifecycle' }
        ]
      },
      {
        text: '⚡ Developer Tooling & Workflow',
        collapsed: false,
        items: [
          { text: 'Hot Reloading / Live Preview', link: '/guide/hot-reload' },
          { text: 'Unicode & CJK Support', link: '/guide/unicode' },
          { text: 'Markdown Rendering', link: '/guide/markdown' }
        ]
      },
      {
        text: '📚 Cookbook & Recipes',
        collapsed: true,
        items: [
          { text: 'Common UI Recipes', link: '/guide/cookbook' }
        ]
      },
      {
        text: '📖 Reference Manuals',
        collapsed: false,
        items: [
          { text: 'HTML Element Reference', link: '/html_reference' },
          { text: 'CSS Property Reference', link: '/css_reference' },
          { text: 'C++ API Reference', link: '/cpp_api' }
        ]
      }
    ],
    socialLinks: [
      { icon: 'github', link: 'https://github.com/ArthurSonzogni/RTXUI' }
    ]
  }
})
