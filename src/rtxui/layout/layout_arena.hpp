#ifndef RTXUI_LAYOUT_LAYOUT_ARENA_HPP
#define RTXUI_LAYOUT_LAYOUT_ARENA_HPP

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

// Under AddressSanitizer, what Reset() frees is poisoned until allocated
// again, so that a pointer kept into a previous layout is reported where it is
// read rather than reading whatever the next layout put there.
#if defined(__SANITIZE_ADDRESS__)
#define RTXUI_LAYOUT_ARENA_POISON 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define RTXUI_LAYOUT_ARENA_POISON 1
#endif
#endif
#ifdef RTXUI_LAYOUT_ARENA_POISON
#include <sanitizer/asan_interface.h>
#define RTXUI_ARENA_POISON(ptr, size) ASAN_POISON_MEMORY_REGION(ptr, size)
#define RTXUI_ARENA_UNPOISON(ptr, size) ASAN_UNPOISON_MEMORY_REGION(ptr, size)
#else
#define RTXUI_ARENA_POISON(ptr, size) ((void)(ptr), (void)(size))
#define RTXUI_ARENA_UNPOISON(ptr, size) ((void)(ptr), (void)(size))
#endif

namespace rtxui {

class LayoutArena {
 public:
  struct Chunk {
    char* ptr = nullptr;
    size_t capacity = 0;
  };

  LayoutArena() = default;
  ~LayoutArena() {
    for (const auto& chunk : allocated_chunks_) {
      RTXUI_ARENA_UNPOISON(chunk.ptr, chunk.capacity);
      delete[] chunk.ptr;
    }
  }

  LayoutArena(const LayoutArena&) = delete;
  LayoutArena& operator=(const LayoutArena&) = delete;

  void* Allocate(size_t size, size_t alignment) {
    if (current_chunk_idx_ < allocated_chunks_.size()) {
      void* ptr = buffer_ + used_;
      size_t space = capacity_ - used_;
      if (std::align(alignment, size, ptr, space)) {
        used_ = capacity_ - space + size;
        RTXUI_ARENA_UNPOISON(ptr, size);
        return ptr;
      }
      current_chunk_idx_++;
      if (current_chunk_idx_ < allocated_chunks_.size()) {
        buffer_ = allocated_chunks_[current_chunk_idx_].ptr;
        capacity_ = allocated_chunks_[current_chunk_idx_].capacity;
        used_ = 0;
        ptr = buffer_ + used_;
        space = capacity_ - used_;
        if (std::align(alignment, size, ptr, space)) {
          used_ = capacity_ - space + size;
          RTXUI_ARENA_UNPOISON(ptr, size);
          return ptr;
        }
      }
    }

    size_t new_cap = std::max(capacity_ * 2, size + alignment + 65536);
    char* new_buf = new char[new_cap];
    RTXUI_ARENA_POISON(new_buf, new_cap);
    allocated_chunks_.push_back({new_buf, new_cap});
    current_chunk_idx_ = allocated_chunks_.size() - 1;
    buffer_ = new_buf;
    capacity_ = new_cap;
    used_ = 0;

    void* ptr = buffer_ + used_;
    size_t space = capacity_ - used_;
    if (std::align(alignment, size, ptr, space)) {
      used_ = capacity_ - space + size;
      RTXUI_ARENA_UNPOISON(ptr, size);
      return ptr;
    }
    return nullptr;
  }

  void Reset() {
    for (const auto& chunk : allocated_chunks_) {
      RTXUI_ARENA_POISON(chunk.ptr, chunk.capacity);
    }
    current_chunk_idx_ = 0;
    used_ = 0;
    if (!allocated_chunks_.empty()) {
      buffer_ = allocated_chunks_[0].ptr;
      capacity_ = allocated_chunks_[0].capacity;
    } else {
      buffer_ = nullptr;
      capacity_ = 0;
    }
  }

 private:
  std::vector<Chunk> allocated_chunks_;
  size_t current_chunk_idx_ = 0;
  char* buffer_ = nullptr;
  size_t capacity_ = 0;
  size_t used_ = 0;
};

// Layout arenas are double-buffered: ResetLayoutArena() flips to the other
// arena and resets it before the new layout tree is built. Memory backing the
// previous frame's tree is therefore never reused during the current frame,
// so a box shared_ptr or a fragment pointer that accidentally survives one
// Draw() (e.g. a local held across Draw() in an event handler) still points
// at intact memory. Neither may outlive two frames.
// The fragments one layout produced, for the next one to reuse. Defined in
// layout.cpp.
struct LayoutCache;

struct LayoutArenas {
  LayoutArenas();
  ~LayoutArenas();
  LayoutArenas(const LayoutArenas&) = delete;
  LayoutArenas& operator=(const LayoutArenas&) = delete;

  LayoutArena arenas[2];
  // What was laid out in each arena. Emptied before its arena is reset, since
  // it holds fragments allocated there. Declared after `arenas`, so destroyed
  // before them.
  std::unique_ptr<LayoutCache> caches[2];
  // Whether a layout may reuse the last one's fragments. A Screen turns it
  // on; code laying out by hand, which changes styles without telling the
  // elements, does not.
  bool reuse_fragments = false;
  int active = 0;
};

// The arena layout allocates from, in the LayoutArenas of the current scope.
LayoutArena& ActiveLayoutArena();

// Makes `arenas` the ones layout uses on this thread while it lives. Each
// screen lays out in its own: with one pair shared by the thread, a second
// screen's frames reset the arena still holding the first screen's tree.
// Outside any scope, a default pair per thread is used.
class ScopedLayoutArenas {
 public:
  explicit ScopedLayoutArenas(LayoutArenas& arenas);
  ~ScopedLayoutArenas();
  ScopedLayoutArenas(const ScopedLayoutArenas&) = delete;
  ScopedLayoutArenas& operator=(const ScopedLayoutArenas&) = delete;

 private:
  LayoutArenas* previous_;
};

template <typename T>
struct LayoutArenaAllocator {
  using value_type = T;
  LayoutArenaAllocator() = default;
  template <typename U>
  constexpr LayoutArenaAllocator(const LayoutArenaAllocator<U>&) noexcept {}

  T* allocate(std::size_t n) {
    void* ptr = ActiveLayoutArena().Allocate(n * sizeof(T), alignof(T));
    if (!ptr) {
      std::cerr << "Fatal: Out of memory in LayoutArena allocator\n";
      std::abort();
    }
    return static_cast<T*>(ptr);
  }

  void deallocate(T* /*p*/, std::size_t /*n*/) noexcept {
    // No-op! Memory is reclaimed via ResetLayoutArena()
  }
};

template <typename T, typename U>
bool operator==(const LayoutArenaAllocator<T>&,
                const LayoutArenaAllocator<U>&) {
  return true;
}
template <typename T, typename U>
bool operator!=(const LayoutArenaAllocator<T>&,
                const LayoutArenaAllocator<U>&) {
  return false;
}

}  // namespace rtxui

#endif  // RTXUI_LAYOUT_LAYOUT_ARENA_HPP
