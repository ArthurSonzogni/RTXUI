import DefaultTheme from 'vitepress/theme'
import WasmTerminal from './components/WasmTerminal.vue'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('WasmTerminal', WasmTerminal)
  }
}
