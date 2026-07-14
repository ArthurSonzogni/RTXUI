<script setup lang="ts">
defineProps<{
  name: string
  values: string
  description: string
  animatable?: boolean
  inherited?: boolean
  shorthand?: boolean
}>()
</script>

<template>
  <div class="css-prop" tabindex="0">
    <code class="css-prop-decl">
      <span class="css-prop-name">{{ name }}</span><span class="css-prop-colon">: </span><span class="css-prop-values">{{ values }}</span>
    </code>
    <span class="css-prop-tags">
      <span v-if="animatable" class="css-tag css-tag-animatable" title="Can be animated with the transition property">✦ animatable</span>
      <span v-if="inherited" class="css-tag css-tag-inherited" title="Inherited by descendant elements">↓ inherited</span>
      <span v-if="shorthand" class="css-tag css-tag-shorthand" title="Expands into several longhand properties">⧉ shorthand</span>
    </span>
    <span class="css-prop-info" aria-hidden="true">ⓘ</span>
    <span class="css-prop-tooltip" role="tooltip">{{ description }}</span>
  </div>
</template>

<style scoped>
.css-prop {
  position: relative;
  display: flex;
  align-items: baseline;
  flex-wrap: wrap;
  gap: 0.5rem;
  padding: 0.45rem 0.75rem;
  margin: 0.25rem 0;
  border-radius: 6px;
  background-color: var(--vp-c-bg-soft);
  outline: none;
}
.css-prop:hover,
.css-prop:focus-visible {
  background-color: var(--vp-c-bg-alt);
}

.css-prop-decl {
  background: none;
  padding: 0;
  font-size: 0.9rem;
  line-height: 1.5;
}
.css-prop-name {
  color: var(--vp-c-brand-1);
  font-weight: 600;
}
.css-prop-colon {
  color: var(--vp-c-text-2);
}
.css-prop-values {
  color: var(--vp-c-text-1);
}

.css-prop-tags {
  display: inline-flex;
  gap: 0.35rem;
  flex-wrap: wrap;
}
.css-tag {
  font-size: 0.68rem;
  line-height: 1;
  padding: 0.25rem 0.45rem;
  border-radius: 999px;
  white-space: nowrap;
  border: 1px solid transparent;
  cursor: default;
}
.css-tag-animatable {
  color: var(--vp-c-purple-1, #8b5cf6);
  background-color: var(--vp-c-purple-soft, rgba(139, 92, 246, 0.14));
}
.css-tag-inherited {
  color: var(--vp-c-green-1, #10b981);
  background-color: var(--vp-c-green-soft, rgba(16, 185, 129, 0.14));
}
.css-tag-shorthand {
  color: var(--vp-c-yellow-1, #d97706);
  background-color: var(--vp-c-yellow-soft, rgba(217, 119, 6, 0.14));
}

.css-prop-info {
  margin-left: auto;
  color: var(--vp-c-text-3);
  font-size: 0.8rem;
  cursor: default;
}

.css-prop-tooltip {
  position: absolute;
  left: 0.5rem;
  right: 0.5rem;
  bottom: calc(100% + 4px);
  z-index: 30;
  padding: 0.5rem 0.7rem;
  border-radius: 6px;
  background-color: var(--vp-c-bg-elv);
  color: var(--vp-c-text-1);
  border: 1px solid var(--vp-c-divider);
  box-shadow: var(--vp-shadow-2);
  font-size: 0.82rem;
  line-height: 1.45;
  opacity: 0;
  visibility: hidden;
  transition: opacity 0.12s ease;
  pointer-events: none;
}
.css-prop:hover .css-prop-tooltip,
.css-prop:focus-visible .css-prop-tooltip {
  opacity: 1;
  visibility: visible;
}
</style>
