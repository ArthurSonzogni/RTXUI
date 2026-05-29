<template>
  <ClientOnly>
    <div class="wasm-terminal-container">
      <iframe
        :src="`/terminal.html?src=${encodeURIComponent(src)}&cols=${cols}&rows=${rows}`"
        class="terminal-iframe"
        :style="{ height: `${rows * 19 + 24}px` }"
        frameborder="0"
        scrolling="no"
      ></iframe>
    </div>
  </ClientOnly>
</template>

<script setup>
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
</script>

<style scoped>
.wasm-terminal-container {
  margin: 1.5rem 0;
  border-radius: 12px;
  overflow: hidden;
  border: 1px solid var(--vp-c-border);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  background: #0f172a;
}
.terminal-iframe {
  width: 100%;
  border: none;
  display: block;
}
</style>
