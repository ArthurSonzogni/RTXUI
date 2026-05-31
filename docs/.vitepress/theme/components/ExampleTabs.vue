<script setup>
import { ref } from 'vue'

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
  background-color: var(--vp-c-bg-mute);
  border-bottom: 1px solid var(--vp-c-divider);
  padding: 0 12px;
}
.tab-btn {
  padding: 10px 16px;
  font-size: 14px;
  font-weight: 500;
  color: var(--vp-c-text-2);
  border: none;
  background: none;
  cursor: pointer;
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
.tab-content {
  padding: 16px;
}

/* Ensure code blocks inside tabs don't have extra margins */
.tab-content :deep(div[class*='language-']) {
  margin: 0;
}
</style>
