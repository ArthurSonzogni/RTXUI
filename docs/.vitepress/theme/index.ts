import DefaultTheme from 'vitepress/theme'
import WasmTerminal from './components/WasmTerminal.vue'
import ExampleTabs from './components/ExampleTabs.vue'
import CssProperty from './components/CssProperty.vue'
import PlaygroundFullscreen from './components/PlaygroundFullscreen.vue'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('WasmTerminal', WasmTerminal)
    app.component('ExampleTabs', ExampleTabs)
    app.component('CssProperty', CssProperty)
    app.component('PlaygroundFullscreen', PlaygroundFullscreen)
  }
}
