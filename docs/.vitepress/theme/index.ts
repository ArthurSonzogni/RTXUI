import DefaultTheme from 'vitepress/theme'
import './custom.css'
import WasmTerminal from './components/WasmTerminal.vue'
import { h } from 'vue'

export default {
  extends: DefaultTheme,
  Layout() {
    return h(DefaultTheme.Layout, null, {
      'home-hero-actions-after': () => h('div', { class: 'hero-reference' }, [
        h('span', { class: 'ref-label' }, 'Reference:'),
        h('a', { class: 'ref-btn', href: '/cpp_api' }, 'C++'),
        h('a', { class: 'ref-btn', href: '/html_reference' }, 'HTML'),
        h('a', { class: 'ref-btn', href: '/css_reference' }, 'CSS')
      ])
    })
  },
  enhanceApp({ app }) {
    app.component('WasmTerminal', WasmTerminal)
  }
}
