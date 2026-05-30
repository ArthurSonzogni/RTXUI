import DefaultTheme from 'vitepress/theme'
import './custom.css'
import WasmTerminal from './components/WasmTerminal.vue'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('WasmTerminal', WasmTerminal)
  }
}
