<script setup>
import { ref, computed } from 'vue'
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
    default: 40
  }
})

const activeTab = ref('demo')

const fullscreenUrl = computed(() => {
  const resolvedSrc = withBase(props.src)
  const resolvedTerminal = withBase('/terminal.html')
  return `${resolvedTerminal}?src=${encodeURIComponent(resolvedSrc)}&fullscreen=1`
})
</script>

<template>
  <div class="tabs-container">
    <div class="tabs-nav">
      <button 
        class="tab-btn" 
        :class="{ active: activeTab === 'demo' }" 
        @click="activeTab = 'demo'"
      >
        Demo
      </button>
      <button 
        class="tab-btn" 
        :class="{ active: activeTab === 'source' }" 
        @click="activeTab = 'source'"
      >
        Source
      </button>
      <a
        :href="fullscreenUrl"
        target="_blank"
        rel="noopener noreferrer"
        class="tab-btn tab-link"
        title="Open standalone fullscreen version in a new tab"
      >
        <span>Fullscreen Demo</span>
        <svg class="external-icon" viewBox="0 0 24 24" width="13" height="13">
          <path fill="currentColor" d="M14 3v2h3.59l-9.83 9.83 1.41 1.41L19 6.41V10h2V3m-2 16H5V5h7V3H5a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7h-2v7Z"/>
        </svg>
      </a>
    </div>
    <div class="tab-content">
      <div v-show="activeTab === 'demo'">
        <WasmTerminal :src="src" :cols="cols" :rows="rows" />
      </div>
      <div v-show="activeTab === 'source'">
        <slot name="source" />
      </div>
    </div>
  </div>
</template>

<style scoped>
.tabs-container {
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  margin: 20px 0;
  overflow: hidden;
  background-color: var(--vp-c-bg-soft);
}
.tabs-nav {
  display: flex;
  align-items: center;
  background-color: var(--vp-c-bg-mute);
  border-bottom: 1px solid var(--vp-c-divider);
  padding: 0 12px;
}
.tab-btn {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 10px 16px;
  font-size: 14px;
  font-weight: 500;
  color: var(--vp-c-text-2);
  border: none;
  background: none;
  cursor: pointer;
  text-decoration: none;
  border-bottom: 2px solid transparent;
  transition: all 0.2s ease;
}
.tab-btn:hover {
  color: var(--vp-c-text-1);
}
.tab-btn.active {
  color: var(--vp-c-brand-1);
  border-bottom-color: var(--vp-c-brand-1);
  font-weight: 600;
}
.tab-link {
  text-decoration: none;
}
.external-icon {
  opacity: 0.6;
  transition: opacity 0.2s;
}
.tab-link:hover .external-icon {
  opacity: 1;
}
.tab-content {
  padding: 16px;
}

/* Ensure code blocks inside tabs don't have extra margins */
.tab-content :deep(div[class*='language-']) {
  margin: 0;
}
</style>
