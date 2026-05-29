<template>
  <ClientOnly>
    <div class="wasm-terminal-container">
      <div ref="terminalRef" class="terminal-div"></div>
    </div>
  </ClientOnly>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount } from 'vue'

const props = defineProps({
  src: {
    type: String,
    required: true
  },
  cols: {
    type: Number,
    default: 80
  },
  rows: {
    type: Number,
    default: 15
  }
})

const terminalRef = ref(null)
let xtermInstance = null

// Dynamically load stylesheets and scripts from CDN to avoid SSR issues
const loadStylesheet = (url) => {
  return new Promise((resolve) => {
    const link = document.createElement('link')
    link.rel = 'stylesheet'
    link.href = url
    link.onload = resolve
    document.head.appendChild(link)
  })
}

const loadScript = (url) => {
  return new Promise((resolve) => {
    const script = document.createElement('script')
    script.src = url
    script.onload = resolve
    document.body.appendChild(script)
  })
}

onMounted(async () => {
  // 1. Load xterm.js stylesheet and scripts dynamically
  await loadStylesheet('https://cdn.jsdelivr.net/npm/xterm@5.3.0/css/xterm.css')
  await loadScript('https://cdn.jsdelivr.net/npm/xterm@5.3.0/lib/xterm.js')

  const Terminal = window.Terminal

  // 2. Initialize Terminal
  xtermInstance = new Terminal({
    cols: props.cols,
    rows: props.rows,
    cursorBlink: true,
    theme: {
      background: '#0f172a', // Slate 900
      foreground: '#f8fafc', // Slate 50
      cursor: '#38bdf8'       // Sky 400
    }
  })

  xtermInstance.open(terminalRef.value)

  // 3. Expose dimensions to Emscripten TTY
  window.rtxui_columns = xtermInstance.cols
  window.rtxui_lines = xtermInstance.rows
  window.rtxui_input_queue = []

  // Hook terminal input
  xtermInstance.onData((data) => {
    for (let i = 0; i < data.length; ++i) {
      const code = data.charCodeAt(i);
      if (window.rtxui_on_input) {
        const cb = window.rtxui_on_input;
        cb(code);
      } else {
        window.rtxui_input_queue.push(code);
      }
    }
  })

  // Hook stdout output
  window.rtxui_on_output = (str) => {
    xtermInstance.write(str)
  }

  // 4. Load the Emscripten JS Module
  window.Module = {
    preRun: [],
    postRun: [],
    print: (text) => {},
    printErr: (text) => {},
    setStatus: (text) => {}
  }

  await loadScript(props.src)
})

onBeforeUnmount(() => {
  if (xtermInstance) {
    xtermInstance.dispose()
  }
  // Cleanup global hooks
  window.rtxui_on_output = null
  window.rtxui_on_input = null
  window.rtxui_input_queue = null
})
</script>

<style scoped>
.wasm-terminal-container {
  margin: 1.5rem 0;
  border-radius: 12px;
  overflow-x: auto;
  overflow-y: hidden;
  border: 1px solid var(--vp-c-border);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  background: #0f172a;
  padding: 12px;
}
.terminal-div {
  width: max-content;
  min-width: 100%;
}
</style>
