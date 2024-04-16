#include <memory>

template <typename Del, typename T>
concept Deleter = requires(Del del, T* ptr) {
  requires std::is_class_v<Del>;
  del.operator()(ptr);
};

namespace my {
struct base_control_block {
  size_t shared_count;
  size_t weak_count;

  base_control_block(size_t sh_cnt, size_t wk_cnt)
      : shared_count(sh_cnt), weak_count(wk_cnt) {}

  virtual void destroy_shared(void*) = 0;

  virtual ~base_control_block() = default;
};

template <typename U, Deleter<U> Del = std::default_delete<U>,
          typename Alloc = std::allocator<U>>
struct regular_control_block : base_control_block {
  [[no_unique_address]] Del del;
  [[no_unique_address]] Alloc alloc;

  regular_control_block(size_t sh_cnt, size_t wk_cnt, Del del, Alloc alloc)
      : base_control_block(sh_cnt, wk_cnt), del(del), alloc(alloc) {}

  regular_control_block(size_t sh_cnt, size_t wk_cnt)
      : base_control_block(sh_cnt, wk_cnt) {}

  regular_control_block(size_t sh_cnt, size_t wk_cnt, Del del)
      : base_control_block(sh_cnt, wk_cnt), del(del) {}

  regular_control_block(const regular_control_block&) = default;
  regular_control_block(regular_control_block&&)      = default;

  regular_control_block& operator=(const regular_control_block&) = default;
  regular_control_block& operator=(regular_control_block&&)      = default;

  void destroy_shared(void* ptr) override {
    --shared_count;  // TODO --?
    if (!shared_count) {
      del(static_cast<U*>(ptr));

      if (!weak_count) {
        using block_alloc = typename std::allocator_traits<
            Alloc>::template rebind_alloc<regular_control_block>;
        block_alloc b_l(alloc);

        std::allocator_traits<block_alloc>::destroy(b_l, this);
        std::allocator_traits<block_alloc>::deallocate(b_l, this, 1);
      }
    }
  }
};

template <typename U, typename Alloc = std::allocator<U>>
struct make_shared_control_block : base_control_block {
  U value;
  [[no_unique_address]] Alloc alloc;

  template <typename Y>
    requires(std::is_same_v<Y, U> || std::is_same_v<Y, U>)
  explicit make_shared_control_block(size_t sh_cnt, size_t wk_cnt, Y&& value)
      : make_shared_control_block(sh_cnt, wk_cnt, std::forward<Y>(value),
                                  Alloc()) {}

  template <typename Y>
    requires(std::is_same_v<Y, U> || std::is_same_v<Y, U>)
  explicit make_shared_control_block(size_t sh_cnt, size_t wk_cnt, Y&& value,
                                     Alloc alloc)
      : base_control_block(sh_cnt, wk_cnt),
        value(std::forward<Y>(value)),
        alloc(alloc) {}

  make_shared_control_block(const make_shared_control_block&) = default;
  make_shared_control_block(make_shared_control_block&&)      = default;

  make_shared_control_block& operator=(const make_shared_control_block&) =
      default;
  make_shared_control_block& operator=(make_shared_control_block&&) = default;

  void destroy_shared(void* ptr) override {
    --shared_count;

    if (!shared_count) {
      std::allocator_traits<Alloc>::destroy(alloc, &this->value);

      if (!weak_count) {
        using CB_alloc = std::allocator<make_shared_control_block>;
        CB_alloc cb_alloc;
        std::allocator_traits<CB_alloc>::deallocate(cb_alloc, this, 1);
      }
    }
  }
};
}  // namespace my

template <typename T>
class SharedPtr {
 private:
  T* ptr_                     = nullptr;
  my::base_control_block* bc_ = nullptr;

  explicit SharedPtr(my::make_shared_control_block<T>* ptr)
      : ptr_(&ptr->value), bc_(ptr) {}

  template <typename U>
  friend class SharedPtr;

  template <typename U, typename... Args, typename Alloc>
  friend SharedPtr<U> allocateShared(Alloc&, Args&&...);

 public:
  using value_type = T;  // TODO remove_ref??
  using reference  = T&;
  using pointer    = T*;

  SharedPtr() = default;

  template <typename U>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  explicit SharedPtr(U* ptr)
      : ptr_(ptr), bc_(new my::regular_control_block<U>(1, 0)) {}

  template <typename U>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  SharedPtr(const SharedPtr<U>& other) noexcept
      : ptr_(other.ptr_), bc_(other.bc_) {
    ++bc_->shared_count;
  }
  SharedPtr(const SharedPtr& other) noexcept
      : ptr_(other.ptr_), bc_(other.bc_) {
    ++bc_->shared_count;
  }

  template <typename U>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  explicit SharedPtr(const SharedPtr<U>& other, T* ptr) noexcept  // TODO
      : ptr_(ptr), bc_(other.bc_) {
    ++bc_->shared_count;
  }

  template <typename U>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  explicit SharedPtr(SharedPtr<U>&& other) noexcept
      : ptr_(other.ptr_), bc_(other.bc_) {
    other.bc_  = nullptr;
    other.ptr_ = nullptr;
  }

  SharedPtr(SharedPtr&& other) noexcept : ptr_(other.ptr_), bc_(other.bc_) {
    other.bc_  = nullptr;
    other.ptr_ = nullptr;
  }

  template <typename U, Deleter<U> Del>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  SharedPtr(U* ptr, Del del)
      : ptr_(ptr), bc_(new my::regular_control_block<U, Del>(1, 0, del)) {}

  template <typename U, Deleter<U> Del, typename Alloc>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  SharedPtr(U* ptr, Del del, Alloc alloc) : ptr_(ptr) {
    using bc_alloc =
        typename std::allocator_traits<Alloc>::template rebind_alloc<
            typename my::regular_control_block<U, Del, Alloc>>;
    bc_alloc tmp_alloc(alloc);

    auto* new_bc_ptr = std::allocator_traits<bc_alloc>::allocate(tmp_alloc, 1);
    std::allocator_traits<bc_alloc>::construct(tmp_alloc, new_bc_ptr, 1, 0, del,
                                               alloc);
    bc_ = new_bc_ptr;
  }

  template <typename U>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  SharedPtr& operator=(const SharedPtr<U>& other) & noexcept {
    if (bc_) {
      bc_->destroy_shared(ptr_);
    }

    bc_  = other.bc_;
    ptr_ = other.ptr_;
    ++bc_->shared_count;

    return *this;
  }

  SharedPtr& operator=(const SharedPtr& other) & noexcept {
    if (&other != this) {
      if (bc_) {
        bc_->destroy_shared(ptr_);
      }

      bc_  = other.bc_;
      ptr_ = other.ptr_;
      ++bc_->shared_count;
    }

    return *this;
  }

  template <typename U>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  SharedPtr& operator=(SharedPtr<U>&& other) & noexcept {
    if (bc_) {
      bc_->destroy_shared(ptr_);
    }

    bc_  = other.bc_;
    ptr_ = other.ptr_;

    other.bc_  = nullptr;
    other.ptr_ = nullptr;

    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) & noexcept {
    if (this != &other) {
      if (bc_) {
        bc_->destroy_shared(ptr_);
      }

      bc_  = other.bc_;
      ptr_ = other.ptr_;

      other.bc_  = nullptr;
      other.ptr_ = nullptr;
    }

    return *this;
  }

  [[nodiscard]] size_t use_count() const noexcept {
    return (bc_ ? bc_->shared_count : 0);
  }

  template <typename U>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  void reset(U* ptr) noexcept {
    SharedPtr(ptr).swap(*this);
  }

  void reset() noexcept {
    SharedPtr().swap(*this);
  }

  template <typename U>
    requires(std::is_base_of_v<T, U> || std::is_same_v<T, U>)
  void swap(SharedPtr<U>& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(bc_, other.bc_);
  }

  reference operator*() const noexcept {
    return *ptr_;
  }

  pointer get() const noexcept {
    return operator->();
  }

  pointer operator->() const noexcept {
    return ptr_;
  }

  ~SharedPtr() {
    if (bc_) {
      bc_->destroy_shared(ptr_);
    }
  }
};

template <typename T, typename... Args, typename Alloc = std::allocator<T>>
SharedPtr<T> allocateShared(Alloc& alloc, Args&&... args) {
  using CB_Alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<
      my::make_shared_control_block<T>>;
  CB_Alloc cb_alloc(alloc);

  auto* ptr = std::allocator_traits<CB_Alloc>::allocate(cb_alloc, 1);
  std::allocator_traits<Alloc>::construct(
      alloc, ptr, 1, 0, std::move(T(std::forward<Args>(args)...)), alloc);

  return SharedPtr(ptr);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  std::allocator<T> cnt_alloc;
  return allocateShared<T, Args...>(cnt_alloc, std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {};