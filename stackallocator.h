#include <memory>

template <size_t N>
class StackStorage {
 public:
  void* current = arr;
  char arr[N];
  size_t space = N;

  StackStorage() : arr() {}

  bool operator==(const StackStorage& other) const {
    return arr = other.arr_;
  }

  bool operator!=(const StackStorage& other) const {
    return arr != other.arr;
  }
};

template <typename T, size_t N>
class StackAllocator {
 private:
  StackStorage<N>& store_;

  template <typename U, size_t M>
  friend class StackAllocator;

 public:
  using value_type = T;
  using pointer    = T*;

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other) : store_(other.store_) {}

  explicit StackAllocator(StackStorage<N>& store) : store_(store) {}

  StackAllocator& operator=(const StackAllocator& other) {
    store_ = other.store_;
    return *this;
  }

  ~StackAllocator() = default;

  [[nodiscard]] pointer allocate(size_t n_) const {
    if (std::align(alignof(T), sizeof(T) * n_, store_.current, store_.space))
        [[likely]] {
      auto result = reinterpret_cast<pointer>(store_.current);

      store_.current = reinterpret_cast<char*>(store_.current) + sizeof(T) * n_;
      store_.space -= sizeof(T) * n_;

      return result;
    }
    throw std::bad_alloc();
  }

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

  StackAllocator select_on_container_construction() const {
    return *this;
  }

  using propagate_on_container_copy_assignment = std::true_type;

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };
};

namespace base {
struct base_node {
  base_node* prev;
  base_node* next;
};

template <typename T, typename Alloc>
class base_list {
 protected:
  struct node : public base_node {
    T value;
  };

  using default_alloc = Alloc;
  using node_alloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<node>;

  default_alloc T_alloc_;
  node_alloc node_alloc_;

  explicit base_list(const default_alloc& alloc)
      : T_alloc_(alloc), node_alloc_(alloc) {}
};
}  // namespace base

template <typename T, typename Alloc = std::allocator<T>>
class List : private base::base_list<T, Alloc> {
 private:
  using typename base::base_list<T, Alloc>::node;
  using base_node = base::base_node;
  using typename base::base_list<T, Alloc>::default_alloc;
  using typename base::base_list<T, Alloc>::node_alloc;

  template <bool is_const>
  class base_iterator {
   private:
    base_node* node_;

    friend class List;

   public:
    using value_type = typename std::conditional<is_const, const T, T>::type;
    using reference  = typename std::conditional<is_const, const T&, T&>::type;
    using pointer    = typename std::conditional<is_const, const T*, T*>::type;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

    base_iterator() = default;

    base_iterator(const base_iterator<false>& other) : node_(other.node_) {}

    base_iterator(const base_node* node_)
        : node_(const_cast<base_node*>(node_)) {}

    base_iterator& operator=(const base_iterator<false> other) {
      node_ = other.node_;
      return *this;
    }

    reference operator*() const {
      return static_cast<node*>(node_)->value;
    }

    base_iterator& operator++() {
      node_ = node_->next;
      return *this;
    }

    base_iterator operator++(int) {
      auto res = *this;
      node_    = node_->next;
      return res;
    }

    base_iterator& operator--() {
      node_ = node_->prev;
      return *this;
    }

    base_iterator operator--(int) {
      auto res = *this;
      node_    = node_->prev;
      return res;
    }

    [[nodiscard]] base_node* get_node() const {
      return node_;
    }

    bool operator==(const base_iterator<true>& other) {
      return node_ == other.node_;
    }

    bool operator==(const base_iterator<false>& other) {
      return node_ == other.node_;
    }

    bool operator!=(const base_iterator<true>& other) {
      return node_ != other.node_;
    }

    bool operator!=(const base_iterator<false>& other) {
      return node_ != other.node_;
    }
  };

  base_node fake_node_;
  size_t sz_;

  using base::base_list<T, Alloc>::T_alloc_;
  using base::base_list<T, Alloc>::node_alloc_;
  using base_list = typename base::base_list<T, Alloc>;

  template <typename Allocator>
  void insert_(base_iterator<true> it, const T& value, Allocator& alloc) {
    node* new_node = std::allocator_traits<Allocator>::allocate(alloc, 1);

    try {
      std::allocator_traits<Allocator>::construct(
          alloc, new_node, node{{it.get_node()->prev, it.get_node()}, value});

    } catch (...) {
      std::allocator_traits<Allocator>::deallocate(alloc, new_node, 1);
      throw;
    }

    it.get_node()->prev->next = new_node;
    it.get_node()->prev       = new_node;
    ++sz_;
  }

  template <typename Allocator>
  void erase_(base_iterator<true> it, Allocator& alloc) {
    auto old_node = it.get_node();

    old_node->prev->next = old_node->next;
    old_node->next->prev = old_node->prev;

    std::allocator_traits<Allocator>::destroy(alloc,
                                              static_cast<node*>(old_node));
    std::allocator_traits<Allocator>::deallocate(
        alloc, static_cast<node*>(old_node), 1);
    --sz_;
  }

 public:
  using iterator               = base_iterator<false>;
  using const_iterator         = base_iterator<true>;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  List() : List(Alloc()) {}

  explicit List(const Alloc& alloc)
      : base_list(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(
                alloc)),
        fake_node_{&fake_node_, &fake_node_},
        sz_(0) {}

  List(size_t n_) : List(n_, Alloc()) {}

  List(size_t n_, const T& value) : List(n_, value, Alloc()) {}

  List(size_t n_, const Alloc& alloc) : List(alloc) {
    while (sz_ < n_) {
      node* new_node =
          std::allocator_traits<node_alloc>::allocate(node_alloc_, 1);

      try {
        std::allocator_traits<node_alloc>::construct(node_alloc_, new_node);

      } catch (...) {
        std::allocator_traits<node_alloc>::deallocate(node_alloc_, new_node, 1);
        throw;
      }

      auto cend = --end();

      new_node->prev = cend.get_node();
      new_node->next = cend.get_node()->next;

      cend.get_node()->next->prev = new_node;
      cend.get_node()->next       = new_node;

      ++sz_;
    }
  }

  List(size_t n_, const T& value, const Alloc& alloc) : List(alloc) {
    while (sz_ < n_) {
      try {
        insert_(end(), value, node_alloc_);

      } catch (...) {
        while (sz_ > 0) {
          erase_(--end(), node_alloc_);
        }

        throw;
      }
    }
  }

  List(const List& other) : List(other.get_allocator()) {
    auto it = other.cbegin();
    for (size_t i = 0; i < other.size(); ++i, ++it) {
      node* new_node =
          std::allocator_traits<node_alloc>::allocate(node_alloc_, 1);

      try {
        std::allocator_traits<node_alloc>::construct(
            node_alloc_, new_node, static_cast<node&>(*(it.get_node())));
      } catch (...) {
        std::allocator_traits<node_alloc>::deallocate(node_alloc_, new_node, 1);
        throw;
      }

      auto cend = --end();

      new_node->prev = cend.get_node();
      new_node->next = cend.get_node()->next;

      cend.get_node()->next->prev = new_node;
      cend.get_node()->next       = new_node;

      ++sz_;
    }
  }

  List& operator=(const List& other) {
    auto new_alloc = std::allocator_traits<
                         Alloc>::propagate_on_container_copy_assignment::value
                         ? other.get_allocator()
                         : T_alloc_;
    auto new_node_alloc =
        std::allocator_traits<
            Alloc>::propagate_on_container_copy_assignment::value
            ? typename std::allocator_traits<Alloc>::template rebind_alloc<
                  node>(new_alloc)
            : node_alloc_;

    int old_sz = size();

    auto it = other.cbegin();

    for (int i = 0; i < (int)other.size(); ++i) {
      try {
        node* new_node =
            std::allocator_traits<node_alloc>::allocate(node_alloc_, 1);

        try {
          std::allocator_traits<node_alloc>::construct(
              node_alloc_, new_node, static_cast<node&>(*(it.get_node())));
        } catch (...) {
          std::allocator_traits<node_alloc>::deallocate(node_alloc_, new_node,
                                                        1);
          throw;
        }

        auto cend = --end();

        new_node->prev = cend.get_node();
        new_node->next = cend.get_node()->next;

        cend.get_node()->next->prev = new_node;
        cend.get_node()->next       = new_node;

        ++sz_;

      } catch (...) {
        for (; i > 0; --i) {
          erase_(--end(), new_node_alloc);
        }
        throw;
      }
    }

    while (--old_sz > -1) {
      erase_(begin(), new_node_alloc);
    }

    if constexpr (std::allocator_traits<
                      Alloc>::propagate_on_container_copy_assignment::value) {
      T_alloc_    = new_alloc;
      node_alloc_ = new_node_alloc;
    }

    return *this;
  }

  [[nodiscard]] size_t size() const {
    return sz_;
  }

  void insert(const_iterator it, const T& value) {
    insert_(it, value, node_alloc_);
  }

  void erase(const_iterator it) noexcept(std::is_nothrow_destructible_v<T>) {
    erase_(it, node_alloc_);
  }

  iterator begin() noexcept {
    return iterator{fake_node_.next};
  }

  const_iterator begin() const noexcept {
    return const_iterator{fake_node_.next};
  }

  const_iterator cbegin() const noexcept {
    return const_iterator{fake_node_.next};
  }

  iterator end() noexcept {
    return iterator{&fake_node_};
  }

  const_iterator end() const noexcept {
    return const_iterator{&fake_node_};
  }

  const_iterator cend() const noexcept {
    return const_iterator{&fake_node_};
  }

  reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  [[nodiscard]] const default_alloc& get_allocator() const {
    return T_alloc_;
  }

  void push_back(const T& value) {
    insert(end(), value);
  }

  void push_front(const T& value) {
    insert(begin(), value);
  }

  void pop_back() {
    erase(--end());
  }

  void pop_front() {
    erase(begin());
  }

  ~List() {
    while (sz_ != 0) {
      erase_(--end(), node_alloc_);
    }
  }
};
