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
    base_node(base_node&&)      = default;

    base_node& operator=(const base_node&) = default;
    base_node& operator=(base_node&&)      = default;
  };

  struct node : public base_node {
    T value;

    node() = default;

    template <typename U>
    node(base_node* prev, base_node* next, U&& value)
        : base_node(prev, next), value(std::forward<U>(value)) {}

    node(base_node* prev, base_node* next) : base_node(prev, next), value() {}

    node(const node&) = default;
    node(node&&)      = default;

    node& operator=(const node&) = default;
    node& operator=(node&&)      = default;
  };

  using default_alloc = Alloc;
  using node_alloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<node>;

  template <bool is_const>
  class base_iterator {
   private:
    base_node* node_;

    friend class List;

    [[nodiscard]] base_node* get_base_node() const {
      return node_;
    }

    [[nodiscard]] node* get_node() const {
      return static_cast<node*>(node_);
    }

   public:
    using value_type      = typename std::conditional_t<is_const, const T, T>;
    using reference       = typename std::conditional_t<is_const, const T&, T&>;
    using pointer         = typename std::conditional_t<is_const, const T*, T*>;
    using difference_type = std::ptrdiff_t;
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

    [[nodiscard]] pointer operator->() const {
      return &(static_cast<node*>(node_)->value);
    }

    template <bool IsConst>
    bool operator==(const base_iterator<IsConst>& other) const {
      return node_ == other.node_;
    }

    template <bool IsConst>
    bool operator!=(const base_iterator<IsConst>& other) const {
      return node_ != other.node_;
    }
  };

 public:
  using iterator               = base_iterator<false>;
  using const_iterator         = base_iterator<true>;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
  base_node fake_node_;
  size_t sz_;
  [[no_unique_address]] default_alloc T_alloc_;
  [[no_unique_address]] node_alloc node_alloc_;

  template <typename Allocator, typename... Args>
  void emplace_(const_iterator it, Allocator& alloc, Args&&... args) {
    node* new_node = std::allocator_traits<Allocator>::allocate(alloc, 1);

    try {
      std::allocator_traits<Allocator>::construct(
          alloc, new_node, it.get_base_node()->prev, it.get_base_node(),
          std::forward<Args>(args)...);

    } catch (...) {
      std::allocator_traits<Allocator>::deallocate(alloc, new_node, 1);
      throw;
    }

    it.get_base_node()->prev->next = new_node;
    it.get_base_node()->prev       = new_node;
    ++sz_;
  }

  template <typename Allocator>
  void erase_(const_iterator it, Allocator& alloc) {
    auto old_node = it.get_base_node();

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
      pop_back();
    }
  }

 public:
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
      emplace_(end(), node_alloc_);
    }
  }

  List(size_t n_, const T& value, const Alloc& alloc) : List(alloc) {
    while (sz_ < n_) {
      push_back(value);
    }
  }

  List(const List& other)
      : List(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(
                other.get_allocator())) {
    for (auto& elem : other) {
      push_back(elem);
    }
  }

  List(List&& other) noexcept
      : fake_node_(other.fake_node_),
        sz_(other.sz_),
        T_alloc_(std::move(other.T_alloc_)),
        node_alloc_(std::move(T_alloc_)) {
    other.fake_node_ = base_node(&other.fake_node_, &other.fake_node_);

    begin().get_base_node()->prev   = &fake_node_;
    (--end()).get_base_node()->next = &fake_node_;

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

    int old_sz = static_cast<int>(size());

    auto it = other.cbegin();

    size_t i = 0;
    try {
      for (; i < other.size(); ++i, ++it) {
        emplace_(end(), new_node_alloc, *it);
      }
    } catch (...) {
      for (; i > 0; --i) {
        erase_(--end(), new_node_alloc);
      }
      throw;
    }

    while (--old_sz != -1) {
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

      begin().get_base_node()->prev   = &fake_node_;
      (--end()).get_base_node()->next = &fake_node_;

      std::swap(sz_, other.sz_);

      if constexpr (std::allocator_traits<
                        Alloc>::propagate_on_container_move_assignment::value) {
        T_alloc_    = other.T_alloc_;
        node_alloc_ = other.node_alloc_;
      }

    } else {
      auto old_end = --end();

      auto it = other.begin();
      try {
        for (; it != other.end(); ++it) {
          push_back(std::move(*it));
        }
      } catch (...) {
        --it;
        for (; end() != old_end; --it) {
          *it = std::move(*(--end()));
          pop_back();
        }

        throw;
      }

      ++old_end;
      for (auto iter = begin(); iter != old_end; ++iter) {
        pop_front();
      }

      other.clear_();
    }

    return *this;
  }

  [[nodiscard]] size_t size() const {
    return sz_;
  }

  template <typename... Args>
  void insert(const_iterator it, Args&&... args) {
    emplace_(it, node_alloc_, std::forward<Args>(args)...);
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

  void push_back(T&& value) {
    insert(end(), std::move(value));
  }

  void push_front(const T& value) {
    insert(begin(), value);
  }

  void push_front(T&& value) {
    insert(begin(), std::move(value));
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