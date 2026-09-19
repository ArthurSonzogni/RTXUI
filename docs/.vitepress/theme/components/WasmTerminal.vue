<template>
  <ClientOnly>
    <div class="wasm-terminal-container">
      <iframe
        :src="terminalUrl"
        class="terminal-iframe"
        :style="{ height: iframeHeight, width: iframeWidth }"
        frameborder="0"
        scrolling="no"
      ></iframe>
    </div>
  </ClientOnly>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { withBase } from 'vitepress'

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

const terminalUrl = computed(() => {
  const resolvedSrc = withBase(props.src)
  const resolvedTerminal = withBase('/terminal.html')
  return `${resolvedTerminal}?src=${encodeURIComponent(resolvedSrc)}&cols=${props.cols}&rows=${props.rows}`
})

const iframeHeight = ref(`${props.rows * 19 + 24}px`)
const iframeWidth = ref('100%')

const handleMessage = (event) => {
  if (event.data && event.data.type === 'rtxui-terminal-resize') {
    iframeHeight.value = `${event.data.height}px`
    iframeWidth.value = `${event.data.width}px`
  }
}

onMounted(() => {
  window.addEventListener('message', handleMessage)
})

onUnmounted(() => {
  window.removeEventListener('message', handleMessage)
})
</script>

<style scoped>
.wasm-terminal-container {
  margin: 1.5rem 0;
  border-radius: 12px;
  overflow: auto;
  border: 1px solid var(--vp-c-border);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  background: #0f172a;
  max-width: 100%;
  width: max-content;
}
.terminal-iframe {
  border: none;
  display: block;
}
</style>
