<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { withBase } from 'vitepress'

const props = defineProps({
  src: {
    type: String,
    default: '/wasm/rtxui_example_playground.js'
  }
})

const containerRef = ref(null)
const isBrowserFullscreen = ref(false)

const terminalUrl = computed(() => {
  const resolvedSrc = withBase(props.src)
  const resolvedTerminal = withBase('/terminal.html')
  return `${resolvedTerminal}?src=${encodeURIComponent(resolvedSrc)}&fullscreen=1`
})

const standaloneUrl = computed(() => {
  return terminalUrl.value
})

const toggleFullscreen = () => {
  if (!containerRef.value) return
  if (!document.fullscreenElement) {
    containerRef.value.requestFullscreen().catch((err) => {
      console.warn('Fullscreen request failed:', err)
    })
  } else {
    document.exitFullscreen().catch((err) => {
      console.warn('Exit fullscreen failed:', err)
    })
  }
}

const onFullscreenChange = () => {
  isBrowserFullscreen.value = !!document.fullscreenElement
}

onMounted(() => {
  document.addEventListener('fullscreenchange', onFullscreenChange)
})

onUnmounted(() => {
  document.removeEventListener('fullscreenchange', onFullscreenChange)
})
</script>

<template>
  <ClientOnly>
    <div
      ref="containerRef"
      class="playground-container"
      :class="{ 'is-fullscreen': isBrowserFullscreen }"
    >
      <div class="playground-toolbar">
        <div class="toolbar-left">
          <span class="badge">PLAYGROUND</span>
          <span class="title">RTXUI WebAssembly REPL</span>
          <span class="hint">Live HTML/CSS Editor (Left) &amp; Terminal Preview (Right)</span>
        </div>

        <div class="toolbar-right">
          <a
            :href="standaloneUrl"
            target="_blank"
            rel="noopener noreferrer"
            class="action-btn link-btn"
            title="Open terminal in a clean standalone browser tab"
          >
            ↗ Standalone
          </a>

          <button
            class="action-btn fullscreen-btn"
            @click="toggleFullscreen"
            :title="isBrowserFullscreen ? 'Exit fullscreen mode' : 'Enter full screen mode'"
          >
            {{ isBrowserFullscreen ? '✕ Exit' : '⛶ Fullscreen' }}
          </button>
        </div>
      </div>

      <div class="terminal-wrapper">
        <iframe
          :src="terminalUrl"
          class="terminal-iframe"
          frameborder="0"
          scrolling="no"
          allow="fullscreen"
        ></iframe>
      </div>
    </div>
  </ClientOnly>
</template>

<style scoped>
.playground-container {
  display: flex;
  flex-direction: column;
  width: 100%;
  height: calc(100vh - 140px);
  min-height: 640px;
  background: #0f172a;
  border: 1px solid var(--vp-c-border);
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.25);
  margin: 1.5rem 0 2.5rem 0;
}

.playground-container.is-fullscreen {
  position: fixed;
  inset: 0;
  z-index: 999999;
  width: 100vw;
  height: 100vh;
  margin: 0;
  border-radius: 0;
  border: none;
}

.playground-toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 10px;
  padding: 8px 16px;
  background: #1e293b;
  border-bottom: 1px solid #334155;
  color: #f8fafc;
  font-size: 13px;
}

.toolbar-left {
  display: flex;
  align-items: center;
  gap: 10px;
}

.badge {
  background: #3b82f6;
  color: white;
  font-size: 11px;
  font-weight: 700;
  padding: 2px 8px;
  border-radius: 4px;
  letter-spacing: 0.5px;
}

.title {
  font-weight: 600;
  color: #f1f5f9;
}

.hint {
  color: #94a3b8;
  font-size: 12px;
}

.toolbar-right {
  display: flex;
  align-items: center;
  gap: 12px;
}

.action-btn {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  background: #334155;
  color: #f8fafc;
  border: 1px solid #475569;
  border-radius: 6px;
  padding: 5px 12px;
  font-size: 12px;
  font-weight: 500;
  cursor: pointer;
  text-decoration: none;
  transition: all 0.15s ease;
}

.action-btn:hover {
  background: #475569;
  color: white;
}

.action-btn.fullscreen-btn {
  background: #2563eb;
  border-color: #3b82f6;
}

.action-btn.fullscreen-btn:hover {
  background: #1d4ed8;
}

.terminal-wrapper {
  flex: 1;
  width: 100%;
  height: 100%;
  overflow: hidden;
  background: #0f172a;
}

.terminal-iframe {
  width: 100%;
  height: 100%;
  border: none;
  display: block;
}

@media (max-width: 768px) {
  .hint {
    display: none;
  }
}
</style>
