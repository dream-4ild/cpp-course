#include <memory>

template <typename T, typename Alloc = std::allocator<T>>
class List {
 private:
  struct base_node {
    base_node* prev;
    base_node* next;

    base_node() = default;

    base_node(base_node* prev, base_node* next) : prev(prev), next(next) {}

    base_node(const base_node&) = default;

    base_node(base_node&&) = default;

    base_node& operator=(const base_node&) = default;

    base_node& operator=(base_node&&) = default;
  };

  struct node : public base_node {
    T value;

    node() = default;

    template <typename U>
    node(base_node* prev, base_node* next, U&& value)
        : base_node(prev, next), value(std::forward<U>(value)) {}

    node(const node&) = default;

    node(node&&) = default;

    node& operator=(const node&) = default;

    node& operator=(node&&) = default;
  };

  using default_alloc = Alloc;
  using node_alloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<node>;

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
  [[no_unique_address]] default_alloc T_alloc_;
  [[no_unique_address]] node_alloc node_alloc_;

  template <typename U, typename Allocator>
  void insert_(base_iterator<true> it, U&& value, Allocator& alloc) {
    static_assert(std::is_convertible_v<U, T>);
    node* new_node = std::allocator_traits<Allocator>::allocate(alloc, 1);

    try {
      std::allocator_traits<Allocator>::construct(
          alloc, new_node, it.get_node()->prev, it.get_node(),
          std::forward<U>(value));

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

  void clear_() {
    while (sz_ > 0) {
      erase_(--end(), node_alloc_);
    }
  }

 public:
  using iterator               = base_iterator<false>;
  using const_iterator         = base_iterator<true>;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  List() : List(Alloc()) {}

  explicit List(const Alloc& alloc)
      : fake_node_(&fake_node_, &fake_node_),
        sz_(0),
        T_alloc_(alloc),
        node_alloc_(T_alloc_) {}

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
        clear_();

        throw;
      }
    }
  }

  List(const List& other)
      : List(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(
                other.get_allocator())) {
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

  List(List&& other) noexcept
      : fake_node_(other.fake_node_),
        sz_(other.sz_),
        T_alloc_(other.T_alloc_),
        node_alloc_(T_alloc_) {
    other.fake_node_ = base_node(&other.fake_node_, &other.fake_node_);

    begin().get_node()->prev   = &fake_node_;
    (--end()).get_node()->next = &fake_node_;

    other.sz_ = 0;
  }

  List& operator=(const List& other) & {
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

    for (int i = 0; i < (int)other.size(); ++i, ++it) {
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

  List& operator=(List&& other) & noexcept(
      std::allocator_traits<Alloc>::is_always_equal::value ||
      std::is_nothrow_move_constructible_v<T>) {
    if (std::allocator_traits<
            Alloc>::propagate_on_container_move_assignment::value ||
        (T_alloc_ == other.T_alloc_)) {
      clear_();

      fake_node_       = other.fake_node_;
      other.fake_node_ = base_node(&other.fake_node_, &other.fake_node_);

      begin().get_node()->prev   = &fake_node_;
      (--end()).get_node()->next = &fake_node_;

      std::swap(sz_, other.sz_);

      if constexpr (std::allocator_traits<
                        Alloc>::propagate_on_container_move_assignment::value) {
        T_alloc_    = other.T_alloc_;
        node_alloc_ = other.node_alloc_;
      }

    } else {
      auto old_end = --end();

      for (auto it = other.begin(); it != other.end(); ++it) {
        try {
          insert(end(), std::move(*it));
        } catch (...) {
          --it;
          for (; end() != old_end; --it) {
            *it = std::move(*(--end()));
            pop_back();
          }

          throw;
        }
      }

      ++old_end;
      for (auto it = begin(); it != old_end; ++it) {
        pop_front();
      }

      other.clear_();
    }

    return *this;
  }

  [[nodiscard]] size_t size() const {
    return sz_;
  }

  template <typename U>
  void insert(const_iterator it, U&& value) {
    insert_(it, std::forward<U>(value), node_alloc_);
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

  template <typename U>
  void push_back(U&& value) {
    insert(end(), std::forward<U>(value));
  }

  template <typename U>
  void push_front(U&& value) {
    insert(begin(), std::forward<U>(value));
  }

  void pop_back() {
    erase(--end());
  }

  void pop_front() {
    erase(begin());
  }

  ~List() {
    clear_();
  }
};