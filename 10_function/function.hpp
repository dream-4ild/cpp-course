//
// Created by dmitry on 6/18/24.
//
#include <functional>

template <bool, typename...>
class Function_impl;

template <bool isMoveOnly, typename Ret, typename... Args>
class Function_impl<isMoveOnly, Ret(Args...)> {
 private:
  using invoke_ptr_t = Ret (*)(void*, Args...);

  static constexpr size_t BUFFER_SIZE_ = 16;

  template <typename Func>
  static constexpr bool is_large() {
    return sizeof(Func) > BUFFER_SIZE_ ||
           !std::is_trivially_copy_constructible_v<Func>;
  }

  enum operation {
    _destroy_ptr,
    _copy_ptr,
    _get_type_info
  };

  template <typename Func>
  static constexpr const std::type_info& manager_operation(void*& ptr,
                                                           void* fptr,
                                                           operation op) {
    switch (op) {
      case _destroy_ptr:
        if constexpr (is_large<Func>()) {
          delete reinterpret_cast<Func*>(fptr);
        } else {
          reinterpret_cast<Func*>(fptr)->~Func();
        }

        return typeid(void);  // ?

      case _copy_ptr:
        if constexpr (is_large<Func>()) {
          if constexpr (isMoveOnly) {
            ptr = new Func(std::move(*reinterpret_cast<Func*>(fptr)));
          } else {
            ptr = new Func(*reinterpret_cast<Func*>(fptr));
          }
        } else {
          if constexpr (isMoveOnly) {
            new (reinterpret_cast<Func*>(ptr))
                Func(std::move(*reinterpret_cast<Func*>(fptr)));
          } else {
            new (reinterpret_cast<Func*>(ptr))
                Func(*reinterpret_cast<Func*>(fptr));
          }
        }
        return typeid(void);  // ?

      default:
        return typeid(Func);
    }
  }

  using manager_ptr = const std::type_info& (*)(void*& ptr, void* fptr,
                                                operation op);

  template <typename Func>
  static constexpr Ret invoker(Func* ptr, Args... args) {
    return std::invoke(*ptr, std::forward<Args>(args)...);
  }

 private:
  alignas(max_align_t) mutable char buffer_[BUFFER_SIZE_]{};
  void* ptr_{};  // можно объединить в одно поле?
  invoke_ptr_t invoke_ptr;
  manager_ptr manager_;

 public:
  template <typename Func>
    requires(requires(std::decay_t<Func>* func, Args... args) {
              {
                std::invoke(*func, std::forward<Args>(args)...)
              } -> std::convertible_to<Ret>;
            })
  Function_impl(Func&& func)
      : invoke_ptr(
            reinterpret_cast<invoke_ptr_t>(&invoker<std::decay_t<Func>>)),
        manager_(&manager_operation<std::decay_t<Func>>) {
    using bare_type = std::decay_t<Func>;

    if constexpr (is_large<bare_type>()) {
      ptr_ = new bare_type(std::forward<Func>(func));
    } else {
      new (buffer_) bare_type(std::forward<Func>(func));
      ptr_ = buffer_;
    }
  }

  constexpr Function_impl()
      : ptr_(nullptr), invoke_ptr(nullptr), manager_(nullptr) {}

  constexpr Function_impl(nullptr_t) : Function_impl() {}

  constexpr Function_impl(const Function_impl& other)
      : ptr_(buffer_), invoke_ptr(other.invoke_ptr), manager_(other.manager_) {
    manager_(ptr_, other.ptr_, _copy_ptr);
  }

  constexpr Function_impl(Function_impl&& other) noexcept
      : invoke_ptr(other.invoke_ptr), manager_(other.manager_) {
    if (other.ptr_ == other.buffer_) {
      ptr_ = buffer_;
      std::copy(other.buffer_, other.buffer_ + BUFFER_SIZE_, buffer_);
    } else {
      ptr_ = other.ptr_;
    }

    other.ptr_       = nullptr;
    other.invoke_ptr = nullptr;
    other.manager_   = nullptr;
  }

  constexpr Function_impl& operator=(const Function_impl& other) & {
    if (this != &other) {
      if (ptr_) {
        void* cnt_ptr = nullptr;
        manager_(cnt_ptr, ptr_, _destroy_ptr);
      } else {
        ptr_ = buffer_;
      }

      manager_   = other.manager_;
      invoke_ptr = other.invoke_ptr;
      manager_(ptr_, other.ptr_, _copy_ptr);
    }
    return *this;
  }

  constexpr Function_impl& operator=(Function_impl&& other) & noexcept {
    if (this != &other) {
      if (ptr_) {
        void* cnt_ptr = nullptr;
        manager_(cnt_ptr, ptr_, _destroy_ptr);
      }

      if (other.ptr_ == other.buffer_) {
        ptr_ = buffer_;
        std::copy(other.buffer_, other.buffer_ + BUFFER_SIZE_, buffer_);
      } else {
        ptr_ = other.ptr_;
      }

      manager_   = other.manager_;
      invoke_ptr = other.invoke_ptr;

      other.ptr_       = nullptr;
      other.invoke_ptr = nullptr;
      other.manager_   = nullptr;
    }
    return *this;
  }

  template <typename Func>
    requires(!std::is_same_v<std::remove_reference_t<Func>,
                             Function_impl<isMoveOnly, Ret(Args...)>> &&
             requires(std::decay_t<Func>* func, Args... args) {
               {
                 std::invoke(*func, std::forward<Args>(args)...)
               } -> std::convertible_to<Ret>;
             })
  Function_impl& operator=(Func&& func) {
    using bare_type = std::decay_t<Func>;

    if (ptr_) {
      void* cnt_ptr = nullptr;
      manager_(cnt_ptr, ptr_, _destroy_ptr);
    }
    invoke_ptr = reinterpret_cast<invoke_ptr_t>(&invoker<std::decay_t<Func>>);

    manager_ = &manager_operation<std::decay_t<Func>>;

    if constexpr (is_large<bare_type>()) {
      ptr_ = new bare_type(std::forward<Func>(func));
    } else {
      new (buffer_) bare_type(std::forward<Func>(func));
      ptr_ = buffer_;
    }

    return *this;
  }

  constexpr Ret operator()(Args... args) const {
    if (!ptr_) {
      throw std::bad_function_call();
    }

    return invoke_ptr(ptr_, std::forward<Args>(args)...);
  }

  explicit operator bool() const {
    return ptr_ != nullptr;
  }

  [[nodiscard]] const std::type_info& target_type() const noexcept {
    void* cnt_ptr = nullptr;
    return (ptr_ ? manager_(cnt_ptr, ptr_, _get_type_info) : typeid(void));
  }

  template <typename T>
  T* target() {
    if (target_type() == typeid(T)) {
      return reinterpret_cast<T*>(ptr_);
    }
    return nullptr;
  }

  template <typename T>
  const T* target() const {
    if (target_type() == typeid(T)) {
      return reinterpret_cast<const T*>(ptr_);
    }
    return nullptr;
  }

  bool operator==(const Function_impl& other) const {
    return ptr_ == other.ptr_;
  };

  constexpr ~Function_impl() {
    if (ptr_) {
      void* cnt_ptr = nullptr;
      manager_(cnt_ptr, ptr_, _destroy_ptr);
    }
  }
};

template <typename...>
class Function;

template <typename Ret, typename... Args>
class Function<Ret(Args...)> : public Function_impl<false, Ret(Args...)> {
  using Function_impl<false, Ret(Args...)>::Function_impl;
};

template <typename...>
class MoveOnlyFunction;

template <typename Ret, typename... Args>
class MoveOnlyFunction<Ret(Args...)>
    : public Function_impl<true, Ret(Args...)> {
  using Function_impl<true, Ret(Args...)>::Function_impl;
};

namespace helper {
template <typename>
struct function_guide_helper;

template <typename Ret, typename Func, bool Nx, typename... Args>
struct function_guide_helper<Ret (Func::*)(Args...) noexcept(Nx)> {
  using type = Ret(Args...);
};

template <typename Ret, typename Func, bool Nx, typename... Args>
struct function_guide_helper<Ret (Func::*)(Args...) & noexcept(Nx)> {
  using type = Ret(Args...);
};

template <typename Ret, typename Func, bool Nx, typename... Args>
struct function_guide_helper<Ret (Func::*)(Args...) const noexcept(Nx)> {
  using type = Ret(Args...);
};

template <typename Ret, typename Func, bool Nx, typename... Args>
struct function_guide_helper<Ret (Func::*)(Args...) const & noexcept(Nx)> {
  using type = Ret(Args...);
};
}  // namespace helper

template <typename Ret, typename... Args>
Function(Ret (*)(Args...)) -> Function<Ret(Args...)>;

template <typename Func,
          typename Signature = typename helper::function_guide_helper<
              decltype(&Func::operator())>::type>
Function(Func) -> Function<Signature>;

template <typename Ret, typename... Args>
MoveOnlyFunction(Ret (*)(Args...)) -> MoveOnlyFunction<Ret(Args...)>;

template <typename Func,
          typename Signature = typename helper::function_guide_helper<
              decltype(&Func::operator())>::type>
MoveOnlyFunction(Func) -> MoveOnlyFunction<Signature>;