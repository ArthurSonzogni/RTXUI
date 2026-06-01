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
          { text: 'Reactive Model', link: '/reactivity' }
        ]
      },
      {
        text: 'Guide',
        items: [
          { text: 'Hello World', link: '/guide/hello-world' },
          { text: 'Value Interpolation', link: '/guide/interpolation' },
          { text: 'Event Bindings', link: '/guide/bindings' },
          { text: 'Conditional Rendering', link: '/guide/conditionals' },
          { text: 'Loops', link: '/guide/loops' },
          { text: 'Markdown', link: '/guide/markdown' }
        ]
      },
      {
        text: 'Advanced Features',
        items: [
          { text: 'Animations & Hover', link: '/animations' },
          { text: 'Scrolling Containers', link: '/guide/scrolling' },
          { text: 'Unicode & CJK', link: '/guide/unicode' },
          { text: 'Positioning & Layering', link: '/guide/positioning' }
        ]
      },
      {
        text: 'Reference',
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
