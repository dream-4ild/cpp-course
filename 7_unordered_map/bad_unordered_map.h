#include <cmath>
#include <iostream>
#include <vector>

// #include "../6_list_stack_allocator/list.h"

#include <memory>
#include "my_list.h"

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