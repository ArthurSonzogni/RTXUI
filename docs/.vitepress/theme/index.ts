import DefaultTheme from 'vitepress/theme'
import WasmTerminal from './components/WasmTerminal.vue'
import ExampleTabs from './components/ExampleTabs.vue'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('WasmTerminal', WasmTerminal)
    app.component('ExampleTabs', ExampleTabs)
  }
}
