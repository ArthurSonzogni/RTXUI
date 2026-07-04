#ifndef RTXUI_LAYOUT_LAYOUT_ARENA_HPP
#define RTXUI_LAYOUT_LAYOUT_ARENA_HPP

#include <algorithm>
#include <memory>
#include <vector>
#include <cstdlib>
#include <iostream>

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
          return ptr;
        }
      }
    }

    size_t new_cap = std::max(capacity_ * 2, size + alignment + 65536);
    char* new_buf = new char[new_cap];
    allocated_chunks_.push_back({new_buf, new_cap});
    current_chunk_idx_ = allocated_chunks_.size() - 1;
    buffer_ = new_buf;
    capacity_ = new_cap;
    used_ = 0;

    void* ptr = buffer_ + used_;
    size_t space = capacity_ - used_;
    if (std::align(alignment, size, ptr, space)) {
      used_ = capacity_ - space + size;
      return ptr;
    }
    return nullptr;
  }

  void Reset() {
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

extern thread_local LayoutArena g_layout_arena;

template <typename T>
struct LayoutArenaAllocator {
  using value_type = T;
  LayoutArenaAllocator() = default;
  template <typename U>
  constexpr LayoutArenaAllocator(const LayoutArenaAllocator<U>&) noexcept {}

  T* allocate(std::size_t n) {
    void* ptr = g_layout_arena.Allocate(n * sizeof(T), alignof(T));
    if (!ptr) {
      std::cerr << "Fatal: Out of memory in LayoutArena allocator\n";
      std::abort();
    }
    return static_cast<T*>(ptr);
  }

  void deallocate(T* p, std::size_t n) noexcept {
    // No-op! Memory is reclaimed via ResetLayoutArena()
  }
};

template <typename T, typename U>
bool operator==(const LayoutArenaAllocator<T>&, const LayoutArenaAllocator<U>&) { return true; }
template <typename T, typename U>
bool operator!=(const LayoutArenaAllocator<T>&, const LayoutArenaAllocator<U>&) { return false; }

} // namespace rtxui

#endif // RTXUI_LAYOUT_LAYOUT_ARENA_HPP
