#include <memory>

template <size_t N>
class StackStorage {
 public:
  void* current = arr;
  char arr[N];
  size_t space = N;

  StackStorage() : arr() {}

  StackStorage(const StackStorage&) = delete;

  StackStorage& operator=(const StackStorage&) = delete;

  bool operator==(const StackStorage& other) const {
    return arr = other.arr_;
  }
};

template <typename T, size_t N>
class StackAllocator {
 private:
  StackStorage<N>* store_;

  template <typename U, size_t M>
  friend class StackAllocator;

 public:
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T value_type;
  typedef T* pointer;

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other) : store_(other.store_) {}

  explicit StackAllocator(StackStorage<N>& store) : store_(&store) {}

  [[nodiscard]] pointer allocate(size_t n_) const {
    if (std::align(alignof(T), sizeof(T) * n_, store_->current, store_->space))
        [[likely]] {
      auto result = reinterpret_cast<pointer>(store_->current);

      store_->current =
          reinterpret_cast<char*>(store_->current) + sizeof(T) * n_;
      store_->space -= sizeof(T) * n_;

      return result;
    }
    throw std::bad_alloc();
  }

  StackAllocator(const StackAllocator&)            = default;
  StackAllocator& operator=(const StackAllocator&) = default;

  void destroy(pointer ptr) const {
    ptr->~T();
  }

  void deallocate(pointer, size_t) const {}

  template <typename U, size_t M>
  bool operator==(const StackAllocator<U, M>&) {
    return false;
  }

  template <typename U>
  bool operator==(const StackAllocator<U, N>& other) const {
    return store_ == other.store_;
  }

  template <typename U, size_t M>
  bool operator!=(const StackAllocator<U, M>&) {
    return true;
  }

  template <typename U>
  bool operator!=(const StackAllocator<U, N>& other) const {
    return store_ != other.store_;
  }

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };
};
