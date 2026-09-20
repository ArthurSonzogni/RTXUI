import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid({
  base: '/RTXUI/',
  title: 'RTXUI',
  description: 'Terminal user interfaces built from HTML templates, CSS, and plain C++ state.',
  themeConfig: {
    logo: {
      light: '/logo-light.png',
      dark: '/logo-dark.png'
    },
    nav: [
      { text: 'Guide', link: '/guide/getting-started', activeMatch: '/guide/' },
      { text: 'Playground', link: '/playground', activeMatch: '^/playground' },
      { text: 'Reference', link: '/html_reference', activeMatch: '/(cpp_api|html_reference|css_reference)' },
      { text: 'Examples', link: '/guide/examples', activeMatch: '^/guide/examples' }
    ],
    sidebar: [
      {
        text: 'Start',
        items: [
          { text: 'Getting Started', link: '/guide/getting-started' },
          { text: 'Hello World', link: '/guide/hello-world' },
          { text: 'Reactivity', link: '/reactivity' },
          { text: 'Playground', link: '/playground' }
        ]
      },
      {
        text: 'Templates',
        collapsed: false,
        items: [
          { text: 'Interpolation', link: '/guide/interpolation' },
          { text: 'Event Handlers', link: '/guide/bindings' },
          { text: 'Conditional Rendering', link: '/guide/conditionals' },
          { text: 'Loops & Lists', link: '/guide/loops' },
          { text: 'Form Elements', link: '/guide/forms' },
          { text: 'Keyboard Focus & Navigation', link: '/guide/html/focus' }
        ]
      },
      {
        text: 'Styling',
        collapsed: false,
        items: [
          { text: 'Basics & Selectors', link: '/guide/css/basics' },
          { text: 'Box Model & Dimensions', link: '/guide/css/box-model' },
          { text: 'Flexbox', link: '/guide/css/flexbox' },
          { text: 'Grid', link: '/guide/css/grid' },
          { text: 'Positioning & Layers', link: '/guide/css/positioning' },
          { text: 'Typography', link: '/guide/typography' },
          { text: 'Scrolling', link: '/guide/scrolling' },
          { text: 'Transitions & Hover States', link: '/guide/css/animations' },
          { text: 'Media Queries', link: '/guide/css/media-queries' }
        ]
      },
      {
        text: 'Components in C++',
        collapsed: false,
        items: [
          { text: 'Creating Components', link: '/guide/cpp/components' },
          { text: 'Slots & Composition', link: '/guide/cpp/slots' },
          { text: 'State Bindings & Reflection', link: '/guide/cpp/bindings' },
          { text: 'Navigating the DOM', link: '/guide/cpp/dom' },
          { text: 'Screen & Lifecycle', link: '/guide/cpp/lifecycle' }
        ]
      },
      {
        text: 'Going Further',
        collapsed: false,
        items: [
          { text: 'HTML/CSS Hot-Reloading', link: '/guide/hot-reload' },
          { text: 'Playground Guide', link: '/guide/playground' },
          { text: 'Unicode & CJK', link: '/guide/unicode' },
          { text: 'Markdown', link: '/guide/markdown' },
          { text: 'Cookbook', link: '/guide/cookbook' },
          { text: 'Examples', link: '/guide/examples' }
        ]
      },
      {
        text: 'Reference',
        collapsed: false,
        items: [
          { text: 'HTML Elements', link: '/html_reference' },
          { text: 'CSS Properties', link: '/css_reference' },
          { text: 'C++ API', link: '/cpp_api' }
        ]
      }
    ],
    outline: { level: [2, 3] },
    search: {
      provider: 'local'
    },
    socialLinks: [
      { icon: 'github', link: 'https://github.com/ArthurSonzogni/RTXUI' }
    ]
  },
  vite: {
    build: {
      chunkSizeWarningLimit: 1500
    }
  }
})
