#include <cmath>
#include <iostream>
#include <vector>

// #include "../6_list_stack_allocator/list.h"

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

  template <typename, typename, typename, typename, typename>
  friend class UnorderedMap;

  struct node : public base_node {
    T value;

    node() = default;

    template <typename... U>
    node(base_node* prev, base_node* next, U&&... value)
        : base_node(prev, next), value(std::forward<U>(value)...) {}

    node(base_node* prev, base_node* next) : base_node(prev, next), value() {}

    node(const node&) = default;

    node(node&&) = default;

    node& operator=(const node&) = default;

    node& operator=(node&&) = default;
  };

  using default_alloc = Alloc;
  using node_alloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<node>;
  using pointer_alloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<size_t>;

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

    [[nodiscard]] pointer operator->() const {
      return &(static_cast<node*>(node_)->value);
    }

    [[nodiscard]] base_node* get_base_node() const {
      return node_;
    }

    [[nodiscard]] node* get_node() const {
      return static_cast<node*>(node_);
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

  base_node fake_node_;
  size_t sz_;
  [[no_unique_address]] default_alloc T_alloc_;
  [[no_unique_address]] node_alloc node_alloc_;
  [[no_unique_address]] pointer_alloc PTR_alloc;

  template <typename Allocator, typename... Args>
  void insert_(base_iterator<true> it, Allocator& alloc, Args&&... args) {
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
  void erase_(base_iterator<true> it, Allocator& alloc) {
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
      erase_(--end(), node_alloc_);
    }
  }

 public:
  using iterator               = base_iterator<false>;
  using const_iterator         = base_iterator<true>;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using move_iterator          = std::move_iterator<iterator>;
  using const_move_iterator    = std::move_iterator<const_iterator>;

  List() : List(Alloc()) {}

  explicit List(const Alloc& alloc)
      : fake_node_(&fake_node_, &fake_node_),
        sz_(0),
        T_alloc_(alloc),
        node_alloc_(T_alloc_),
        PTR_alloc(T_alloc_) {}

  List(size_t n_) : List(n_, Alloc()) {}

  List(size_t n_, const T& value) : List(n_, value, Alloc()) {}

  List(size_t n_, const Alloc& alloc) : List(alloc) {
    while (sz_ < n_) {
      insert_(end(), node_alloc_);
    }
  }

  List(size_t n_, const T& value, const Alloc& alloc) : List(alloc) {
    while (sz_ < n_) {
      insert_(end(), node_alloc_, value);
    }
  }

  List(const List& other)
      : List(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(
                other.get_allocator())) {
    for (auto it = other.cbegin(); it != other.cend(); ++it) {
      insert_(end(), node_alloc_, *it);
    }
  }

  List(List&& other) noexcept
      : fake_node_(other.fake_node_),
        sz_(other.sz_),
        T_alloc_(other.T_alloc_),
        node_alloc_(T_alloc_),
        PTR_alloc(T_alloc_) {
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

    int old_sz = size();

    auto it = other.cbegin();

    for (int i = 0; i < (int)other.size(); ++i, ++it) {
      try {
        insert_(end(), new_node_alloc, *it);

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
    clear_();

    if (other.empty()) {
      return *this;
    }

    fake_node_.next = other.fake_node_.next;
    fake_node_.prev = other.fake_node_.prev;

    other.fake_node_ = base_node(&other.fake_node_, &other.fake_node_);

    begin().get_base_node()->prev   = &fake_node_;
    (--end()).get_base_node()->next = &fake_node_;

    std::swap(sz_, other.sz_);

    if constexpr (std::allocator_traits<
                      Alloc>::propagate_on_container_move_assignment::value) {
      T_alloc_    = other.T_alloc_;
      node_alloc_ = other.node_alloc_;
    }

    //    } else {
    //      auto old_end = --end();
    //
    //      for (auto it = other.begin(); it != other.end(); ++it) {
    //        try {
    //          insert(end(), std::move(*it));
    //        } catch (...) {
    //          --it;
    //          for (; end() != old_end; --it) {
    //            *it = std::move(*(--end()));
    //            pop_back();
    //          }
    //
    //          throw;
    //        }
    //      }
    //
    //      ++old_end;
    //      for (auto it = begin(); it != old_end; ++it) {
    //        pop_front();
    //      }
    //
    //      other.clear_();
    //    }

    return *this;
  }

  [[nodiscard]] size_t size() const {
    return sz_;
  }

  [[nodiscard]] bool empty() const {
    return size() == 0;
  }

  template <typename... Args>
  void insert(const_iterator it, Args&&... args) {
    insert_(it, node_alloc_, std::forward<Args>(args)...);
  }

  template <typename value_type>
  struct SSS {
    using pr_alloc = typename std::allocator_traits<
        Alloc>::template rebind_alloc<value_type>;

    base_node fake_node_;
    [[no_unique_address]] node_alloc node_alloc_;
    [[no_unique_address]] pointer_alloc PTR_alloc;
    [[no_unique_address]] pr_alloc PAIR_ALLOC;

    SSS(base_node fake, node_alloc node_alloc_, pointer_alloc PTR_alloc)
        : fake_node_(fake),
          node_alloc_(node_alloc_),
          PTR_alloc(PTR_alloc),
          PAIR_ALLOC(node_alloc_) {}

    template <typename Key, typename Value>
    node* _insert_(Key&& key, Value&& value) {
      char* new_node = reinterpret_cast<char*>(
          std::allocator_traits<node_alloc>::allocate(node_alloc_, 1));

      try {
        std::allocator_traits<pointer_alloc>::construct(
            PTR_alloc, reinterpret_cast<base_node*>(new_node), &fake_node_,
            &fake_node_);

        static_assert(!std::is_constructible_v<value_type,
                                               const std::remove_reference<Key>,
                                               std::remove_reference<Value>>);

        std::allocator_traits<pr_alloc>::construct(
            PAIR_ALLOC, reinterpret_cast<value_type*>(new_node + 16),
            std::forward<Key>(key), std::forward<Value>(value));

        //        std::allocator_traits<pointer_alloc>::construct(
        //            PTR_alloc, reinterpret_cast<base_node*>(new_node +
        //            sizeof(T) - 8), 0);

      } catch (...) {
        std::allocator_traits<node_alloc>::deallocate(
            node_alloc_, reinterpret_cast<node*>(new_node), 1);
        throw;
      }

      return reinterpret_cast<node*>(new_node);
    }
  };

  void _link_and_insert_(const_iterator it, node* new_node) {
    new_node->prev = it.get_base_node()->prev;
    new_node->next = it.get_base_node();

    it.get_base_node()->prev->next = new_node;
    it.get_base_node()->prev       = new_node;
    ++sz_;
  }

  void _delete_node_without_unlink_(node* old_node) {
    std::allocator_traits<node_alloc>::destroy(node_alloc_,
                                               static_cast<node*>(old_node));
    std::allocator_traits<node_alloc>::deallocate(
        node_alloc_, static_cast<node*>(old_node), 1);
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

  move_iterator mbegin() noexcept {
    return move_iterator(fake_node_.next);
  }

  move_iterator mend() noexcept {
    return move_iterator(&fake_node_);
  }

  [[nodiscard]] const default_alloc& get_allocator() const {
    return T_alloc_;
  }

  template <typename... U>
  void push_back(U&&... value) {
    insert(end(), std::forward<U>(value)...);
  }

  template <typename... U>
  void push_front(U&&... value) {
    insert(begin(), std::forward<U>(value)...);
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

  bool operator==(const List& other) const {
    return &fake_node_ == &other.fake_node_;
  }

  bool operator!=(const List& other) const {
    return &fake_node_ != &other.fake_node_;
  }
};

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 public:
  using key_type        = const Key;
  using mapped_type     = Value;
  using NodeType        = std::pair<key_type, mapped_type>;
  using value_type      = std::pair<key_type, mapped_type>;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  using hasher          = Hash;
  using key_equal       = Equal;
  using allocator_type  = Alloc;
  using reference       = value_type&;
  using const_reference = const value_type&;
  using pointer = typename std::allocator_traits<allocator_type>::pointer;
  using const_pointer =
      typename std::allocator_traits<allocator_type>::const_pointer;

 private:
  class list_node : public value_type {
   public:
    size_type hash;

    template <typename T>
    list_node(T&& value, hasher& _hash, size_type _table_size)
        : value_type(std::forward<T>(value)),
          hash(get_hash_(this->first, _hash, _table_size)) {}

    template <typename... Args>
    list_node(Args&&... args)
        : value_type(std::forward<Args>(args)...), hash(0) {}

    list_node(const list_node&) = default;
    list_node(list_node&&)      = default;

    list_node& operator=(const list_node&) = default;
    list_node& operator=(list_node&&)      = default;
  };

  template <bool is_const>
  class base_iterator {
   private:
    using iterator_type = std::conditional_t<
        is_const, typename List<list_node, allocator_type>::const_iterator,
        typename List<list_node, allocator_type>::iterator>;

    iterator_type iter_;

    friend class UnorderedMap;

   public:
    using value_type = std::conditional_t<is_const, const NodeType, NodeType>;
    using reference  = std::conditional_t<is_const, const NodeType&, NodeType&>;
    using pointer    = std::conditional_t<is_const, const NodeType*, NodeType*>;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    // base_iterator(const base_iterator<false>& other) : iter_(other.iter_) {}

    base_iterator() = default;

    base_iterator(iterator_type it) : iter_(it) {}

    base_iterator& operator=(const base_iterator<false>& other) {
      iter_ = other.iter_;
      return *this;
    }

    reference operator*() const {
      return static_cast<reference>(*iter_);
    }

    pointer operator->() const {
      return iter_.operator->();
    }

    base_iterator& operator++() {
      ++iter_;
      return *this;
    }

    base_iterator operator++(int) {
      auto res = *this;
      ++iter_;
      return res;
    }

    template <bool IsConst>
    bool operator==(const base_iterator<IsConst>& other) const {
      return iter_ == other.iter_;
    }

    template <bool IsConst>
    bool operator!=(const base_iterator<IsConst>& other) const {
      return iter_ != other.iter_;
    }
  };

  using local_iterator_allocator =
      typename std::allocator_traits<allocator_type>::template rebind_alloc<
          typename List<list_node, allocator_type>::iterator>;

 public:
  using iterator               = base_iterator<false>;
  using const_iterator         = base_iterator<true>;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
  [[no_unique_address]] allocator_type alloc_;
  [[no_unique_address]] mutable hasher hash_;
  [[no_unique_address]] mutable key_equal equal_;
  List<list_node, allocator_type> list_;
  std::vector<typename List<list_node, allocator_type>::iterator,
              local_iterator_allocator>
      table_;
  double max_load_factor_;

  static constexpr size_type k_default_table_size   = 19;
  static constexpr double k_default_max_load_factor = 1.;

  static size_type get_hash_(key_type& key, hasher& _hash, size_type _size) {
    return _hash(key) % _size;
  }

  bool check_equal_(key_type& first, key_type& second) const {
    return equal_(first, second);
  }

  void do_rehash_if_need_() {
    if (static_cast<double>(size()) / table_.size() > max_load_factor_) {
      // std::cout << "rehash " << size() << '\n';
      rehash(2 * table_.size() - 1);
    }
  }

  template <typename T, typename U>
  std::pair<iterator, bool> insert_(T&& key, U&& value) {  // without rehash

    typename decltype(list_)::template SSS<value_type> sss(
        list_.fake_node_, list_.node_alloc_, list_.PTR_alloc);

    auto new_node = sss._insert_(std::forward<T>(key), std::forward<U>(value));

    size_type key_hash = get_hash_(new_node->value.first, hash_, table_.size());

    new_node->value.hash = key_hash;

    if (table_[key_hash] == list_.end()) {
      list_._link_and_insert_(list_.begin(), new_node);
      table_[key_hash] = list_.begin();

      do_rehash_if_need_();
      return std::make_pair(table_[key_hash], true);
    }

    for (auto it = table_[key_hash]; it != list_.end(); ++it) {
      if (it->hash != key_hash) {
        list_._link_and_insert_(it, new_node);

        do_rehash_if_need_();
        return std::make_pair(--it, true);
      }

      if (check_equal_(it->first, new_node->value.first)) {
        list_._delete_node_without_unlink_(new_node);
        return std::make_pair(it, false);
      }
    }

    list_._link_and_insert_(list_.end(), new_node);

    auto cnt_it = --list_.end();

    do_rehash_if_need_();
    return std::make_pair(cnt_it, true);
  }

  template <typename T>
  std::pair<iterator, bool> insert_for_rehash_(
      T* node, decltype(list_)& _list,
      decltype(table_)& _table) {  // without rehash
    // static_assert(std::is_constructible_v<value_type, T>);

    size_type key_hash = get_hash_(node->value.first, hash_, _table.size());

    if (_table[key_hash] == _list.end()) {
      _list._link_and_insert_(_list.begin(), node);
      _table[key_hash] = _list.begin();

      return std::make_pair(_table[key_hash], true);
    }

    for (auto it = _table[key_hash]; iterator(it) != end(); ++it) {
      if (it->hash == key_hash) {
        _list._link_and_insert_(it, node);

        return std::make_pair(--it, true);
      }

      if (check_equal_(it->first, node->value.first)) {
        return std::make_pair(it, false);
      }
    }

    _list._link_and_insert_(_list.end(), node);

    auto cnt_it = --_list.end();

    do_rehash_if_need_();
    return std::make_pair(cnt_it, true);
  }

  template <typename T>
  std::pair<iterator, bool> insert_(
      T&& node, decltype(list_)& _list,
      decltype(table_)& _table) {  // without rehash
    // static_assert(std::is_constructible_v<value_type, T>);

    value_type new_value = std::forward<T>(node);

    size_type key_hash = get_hash_(new_value.first, hash_, _table.size());

    if (_table[key_hash] == _list.end()) {
      _list.insert(_list.begin(), std::move(new_value), hash_, _table.size());
      _table[key_hash] = _list.begin();

      do_rehash_if_need_();
      return std::make_pair(_table[key_hash], true);
    }

    for (auto it = _table[key_hash]; iterator(it) != end(); ++it) {
      if (it->hash == key_hash) {
        _list.insert(it, new_value, hash_, _table.size());

        do_rehash_if_need_();
        return std::make_pair(--it, true);
      }

      if (check_equal_(it->first, new_value.first)) {
        return std::make_pair(it, false);
      }
    }

    _list.insert(_list.end(), new_value);

    auto cnt_it = --_list.end();

    do_rehash_if_need_();
    return std::make_pair(cnt_it, true);
  }

 public:
  UnorderedMap()
      : alloc_(),
        hash_(),
        equal_(),
        list_(alloc_),
        table_(k_default_table_size, list_.end(),
               local_iterator_allocator(alloc_)),
        max_load_factor_(k_default_max_load_factor) {}

  UnorderedMap(const UnorderedMap& other)
      : alloc_(std::allocator_traits<allocator_type>::
                   select_on_container_copy_construction(other.alloc_)),
        hash_(other.hash_),
        equal_(other.equal_),
        list_(other.list_),
        table_(other.table_),
        max_load_factor_(other.max_load_factor_) {}

  UnorderedMap(UnorderedMap&& other)
      : alloc_(std::move(other.alloc_)),
        hash_(std::move(other.hash_)),
        equal_(std::move(other.equal_)),
        list_(std::move(other.list_)),
        table_(std::move(other.table_)),
        max_load_factor_(other.max_load_factor_) {}

  UnorderedMap& operator=(
      const UnorderedMap& other) & {  // TODO alloctor may throw(((
    if constexpr (std::is_same_v<
                      typename std::allocator_traits<allocator_type>::
                          propagate_on_container_copy_assignment,
                      std::true_type>) {
      alloc_ = other.alloc_;
    }

    list_            = other.list_;   // may throw, but it's ok
    table_           = other.table_;  // nothrow
    hash_            = other.hash_;
    equal_           = other.equal_;
    max_load_factor_ = other.max_load_factor_;

    return *this;
  }

  UnorderedMap& operator=(
      UnorderedMap&& other) & {  // TODO alloctor may throw(((
    if constexpr (std::allocator_traits<allocator_type>::
                      propagate_on_container_move_assignment::value) {
      alloc_ = std::move(other.alloc_);
    }

    list_  = std::move(other.list_);   // may throw, but it's ok
    table_ = std::move(other.table_);  // nothrow
    hash_  = std::move(other.hash_);
    // equal_           = std::move(other.equal_);
    max_load_factor_ = other.max_load_factor_;

    return *this;
  }

  void swap(UnorderedMap& other) {  // TODO лажа
    if constexpr (std::is_same_v<
                      typename std::allocator_traits<
                          allocator_type>::propagate_on_container_swap,
                      std::true_type>) {
      std::swap(alloc_, other.alloc_);
    }

    auto other_list = std::move(other.list_);
    other.list_     = std::move(list_);
    list_           = std::move(other_list);

    std::swap(table_, other.table_);
    std::swap(hash_, other.hash_);
    std::swap(equal_, other.equal_);
    std::swap(max_load_factor_, other.max_load_factor_);
  }

  //  void swap(UnorderedMap& other) {
  //    auto other_list = std::move(other.list_);
  //    other.list_     = std::move(list_);
  //    list_           = std::move(other_list);
  //
  //    std::swap(table_, other.table_);
  //    std::swap(hash_, other.hash_);
  //    std::swap(equal_, other.equal_);
  //    std::swap(max_load_factor_, other.max_load_factor_);
  //  }

  ~UnorderedMap() = default;

  template <typename InputIt>
  void insert(InputIt first, InputIt last) {
    for (auto it = first; it != last; ++it) {
      insert(std::forward<decltype(*it)>(*it));
    }
  }
  template <typename InputIt>
  void erase(InputIt first, InputIt last) {
    auto p = first;
    ++p;
    for (auto it = first; it != last; it = p, ++p) {
      erase(it);
    }
  }

  allocator_type get_allocator() const {
    return alloc_;
  }

  template <typename K, typename V>
  std::pair<iterator, bool> insert(std::pair<K, V>&& node) {
    return insert_(std::move(node.first), std::move(node.second));
  }

  //  std::pair<iterator, bool> insert(value_type&& node) {
  //    return insert_(std::move(node.first), std::move(node.second));
  //  }

  std::pair<iterator, bool> insert(const value_type& node) {
    return insert_(node.first, node.second);
  }

  //  std::pair<iterator, bool> insert(value_type& node) {
  //    return insert_(node.first, node.second);
  //  }

  template <typename T, typename U>
  std::pair<iterator, bool> emplace(T&& key, U&& value) {
    return insert_(std::forward<T>(key), std::forward<U>(value));
  }

  iterator erase(iterator pos) {
    auto res = pos;
    ++res;

    if (table_[pos.iter_->hash] == pos.iter_) {
      if (res == end() || res.iter_->hash != pos.iter_->hash) {
        table_[pos.iter_->hash] = list_.end();
      } else {
        ++table_[pos.iter_->hash];
      }
    }

    list_.erase(pos.iter_);
    return res;
  }

  iterator find(key_type& key) {  // TODO разобраться с const
    size_type key_hash = get_hash_(key, hash_, table_.size());

    for (auto it = table_[key_hash]; it != list_.end(); ++it) {
      if (key_hash != it->hash) {
        return end();
      }

      if (check_equal_(it->first, key)) {
        return it;
      }
    }

    return end();
  }

  const_iterator find(key_type& key) const {  // TODO разобраться с const
    size_type key_hash = get_hash_(key, hash_, table_.size());

    for (auto it = table_[key_hash]; it != list_.end(); ++it) {
      if (key_hash != it->hash) {
        return end();
      }

      if (check_equal_(it->first, key)) {
        return const_iterator(it);
      }
    }

    return end();
  }

  mapped_type& operator[](key_type&& key) {
    auto elem = find(key);
    if (elem != end()) {
      return elem->second;
    }

    return emplace(std::move(key), mapped_type()).first->second;
  }

  mapped_type& operator[](const key_type& key) {
    auto elem = find(key);
    if (elem != end()) {
      return elem->second;
    }

    return emplace(key, mapped_type()).first->second;
  }

  mapped_type& at(const key_type& key) {
    auto elem = find(key);
    if (elem != end()) {
      return elem->second;
    }

    throw std::out_of_range("at corrupted");
  }

  const mapped_type& at(const key_type& key) const {
    auto elem = find(key);
    if (elem != end()) {
      return elem->second;
    }

    throw std::out_of_range("at corrupted");
  }

  size_type size() const {
    return list_.size();
  }

  double load_factor() const noexcept {
    return static_cast<double>(size()) / table_.size();
  }

  [[nodiscard]] double max_load_factor() const noexcept {
    return max_load_factor_;
  }

  void max_load_factor(double _f) noexcept {
    max_load_factor_ = _f;
  }

  void rehash(size_type new_size) {  // TODO move operator= may throw и вообще
                                     // нет гарантий)))
    // if (new_size <= table_.size()) return;
    decltype(list_) new_list(alloc_);
    decltype(table_) new_table(new_size, list_.end(),
                               local_iterator_allocator(alloc_));
    auto it = list_.begin();
    auto p  = it;
    ++p;
    for (; it != list_.end(); it = p, ++p) {
      it.get_node()->prev->next = it.get_node()->next;
      it.get_node()->next->prev = it.get_node()->prev;
      --list_.sz_;
      insert_for_rehash_(it.get_node(), new_list, new_table);
    }

    list_  = std::move(new_list);
    table_ = std::move(new_table);
  }

  void reserve(size_type count) {
    rehash(std::ceil(static_cast<double>(count) / max_load_factor_));
  }

  iterator begin() noexcept {
    return list_.begin();
  }

  const_iterator begin() const noexcept {
    return list_.begin();
  }

  const_iterator cbegin() const noexcept {
    return list_.cbegin();
  }

  iterator end() noexcept {
    return list_.end();
  }

  const_iterator end() const noexcept {
    return list_.end();
  }

  const_iterator cend() const noexcept {
    return list_.cend();
  }

  reverse_iterator rbegin() noexcept {
    return list_.rbegin();
  }

  const_reverse_iterator rbegin() const noexcept {
    return list_.rbegin();
  }

  const_reverse_iterator crbegin() const noexcept {
    return list_.crbegin();
  }

  reverse_iterator rend() noexcept {
    return list_.rend();
  }

  const_reverse_iterator rend() const noexcept {
    return list_.rend();
  }

  const_reverse_iterator crend() const noexcept {
    return list_.crend();
  }
};