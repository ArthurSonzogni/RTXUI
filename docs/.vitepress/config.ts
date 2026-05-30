import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'RTXUI',
  description: 'C++ Reactive Terminal Rendering Engine',
  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Tutorial', link: '/tutorial' },
      {
        text: 'Reference',
        items: [
          { text: 'C++ API', link: '/cpp_api' },
          { text: 'HTML Elements', link: '/html_reference' },
          { text: 'CSS Attributes', link: '/css_reference' }
        ]
      }
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
        text: 'Guides',
        items: [
          { text: 'Tutorial', link: '/tutorial' }
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
