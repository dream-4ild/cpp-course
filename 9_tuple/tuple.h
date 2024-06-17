//
// Created by dmitry on 6/12/24.
//

#pragma once

#include <ctime>
#include <type_traits>
#include <utility>

template <typename...>
class Tuple;

namespace helper {
template <typename T, typename = std::void_t<>>
struct is_compiled_copy_list_initial_from_empty : std::false_type {};

template <typename T>
struct is_compiled_copy_list_initial_from_empty<T, std::void_t<decltype(T{})>>
    : std::true_type {};

template <typename T>
struct bare_type {
  using type = typename std::remove_cv_t<typename std::remove_reference_t<T>>;
};

template <typename T>
using bare_type_t = typename bare_type<T>::type;

template <typename T>
struct is_Tuple : std::false_type {};

template <typename... Args>
struct is_Tuple<Tuple<Args...>> : std::true_type {};

template <typename T>
static constexpr bool is_Tuple_v = is_Tuple<bare_type_t<T>>::value;

template <typename...>
struct concat_tuple_types;

template <typename... Args1, typename... Args2, typename... Rest>
struct concat_tuple_types<Tuple<Args1...>, Tuple<Args2...>, Rest...> {
  using type =
      typename concat_tuple_types<Tuple<Args1..., Args2...>, Rest...>::type;
};

template <typename... Args>
struct concat_tuple_types<Tuple<Args...>> {
  using type = Tuple<Args...>;
};

template <typename... Args>
using concat_tuple_types_t = concat_tuple_types<Args...>::type;

struct private_tuple_cat_tag {};

}  // namespace helper

template <typename Head, typename... Tail>
class Tuple<Head, Tail...> {
 public:
  Head head_;
  [[no_unique_address]] Tuple<Tail...> tail_;

  static constexpr bool is_default_constr =
      (std::is_default_constructible_v<Head> && ... &&
       std::is_default_constructible_v<Tail>);

  static constexpr bool copy_list_initial_from_empty =
      (helper::is_compiled_copy_list_initial_from_empty<Head>::value && ... &&
       helper::is_compiled_copy_list_initial_from_empty<Tail>::value);

  static constexpr bool is_copy_constr =
      (std::is_copy_constructible_v<Head> && ... &&
       std::is_copy_constructible_v<Tail>);

  static constexpr bool is_copy_assign =
      (std::is_copy_assignable_v<Head> && ... &&
       std::is_copy_assignable_v<Tail>);

  static constexpr bool is_move_constr =
      (std::is_move_constructible_v<Head> && ... &&
       std::is_move_constructible_v<Tail>);

  static constexpr bool is_move_assign =
      (std::is_move_assignable_v<Head> && ... &&
       std::is_nothrow_move_assignable_v<Tail>);

  static constexpr bool is_convertible_from_const_ref =
      (std::is_convertible_v<const Head&, Head> && ... &&
       std::is_convertible_v<const Tail&, Tail>);

  template <typename HHead, typename... TTail>
  static constexpr bool is_constructable_from =
      (std::is_constructible_v<Head, HHead> && ... &&
       std::is_constructible_v<Tail, TTail>);

  template <typename HHead, typename... TTail>
  static constexpr bool is_constructable_from_with_double_ref =
      (std::is_constructible_v<Head, HHead&&> && ... &&
       std::is_constructible_v<Tail, TTail&&>);

  template <typename HHead, typename... TTail>
  static constexpr bool is_convertable_from =
      (std::is_convertible_v<HHead, Head> && ... &&
       std::is_convertible_v<TTail, Tail>);

  template <typename HHead, typename... TTail>
  static constexpr bool is_convertable_from_back =
      (std::is_convertible_v<HHead, Head> && ... &&
       std::is_convertible_v<TTail, Tail>);

  template <typename HHead, typename... TTail>
  static constexpr bool is_assign_to_ref_from =
      (std::is_assignable_v<Head&, HHead> && ... &&
       std::is_assignable_v<Tail&, TTail>);

  // три конструктора для работы с tuple_cat
  template <typename HeadTuples, typename... TailTuples>
    requires((!std::is_same_v<HeadTuples, Tuple<>> &&
              helper::is_Tuple_v<HeadTuples>) &&
             ... && helper::is_Tuple_v<TailTuples>)
  Tuple(helper::private_tuple_cat_tag, HeadTuples&& htpl, TailTuples&&... ttpls)
      : head_(htpl.head_),
        tail_(helper::private_tuple_cat_tag(),
              std::forward<decltype(htpl.tail_)>(htpl.tail_),
              std::forward<TailTuples>(ttpls)...) {}

  template <typename HeadTuples, typename... TailTuples>
    requires(std::is_same_v<HeadTuples, Tuple<>>)
  Tuple(helper::private_tuple_cat_tag, HeadTuples&&, TailTuples&&... ttpls)
      : Tuple(helper::private_tuple_cat_tag(),
              std::forward<TailTuples>(ttpls)...) {}

  template <typename Tpl>
  Tuple(helper::private_tuple_cat_tag, Tpl&& tpl)
      : Tuple(std::forward<Tpl>(tpl)) {}

  template <typename... Args>
  friend class Tuple;

  template <size_t N, typename Tpl>
    requires(helper::is_Tuple_v<Tpl>)
  friend constexpr decltype(auto) get(Tpl&&);

  template <typename T, typename... Types>
  friend constexpr T& get(Tuple<Types...>&);

  template <typename T, typename... Types>
  friend constexpr T&& get(Tuple<Types...>&&);

  template <typename T, typename... Types>
  friend constexpr const T& get(const Tuple<Types...>&);

  template <typename T, typename... Types>
  friend constexpr const T&& get(const Tuple<Types...>&&);

  template <typename... Tuples>
  friend constexpr decltype(auto) tupleCat(Tuples&&...);

 public:
  constexpr explicit(!copy_list_initial_from_empty) Tuple()
    requires(is_default_constr)
      : head_(), tail_(){};

  constexpr explicit(!is_convertible_from_const_ref)
      Tuple(const Head& head_, const Tail&... tail_)
    requires(is_copy_constr)
      : head_(head_), tail_(tail_...) {}

  template <typename UHead, typename... UTail>
    requires(sizeof...(Tail) == sizeof...(UTail) &&
             is_constructable_from<UHead, UTail...>)
  constexpr explicit(!is_convertable_from<UHead, UTail...>)
      Tuple(UHead&& head_, UTail&&... tail_)
      : head_(std::forward<UHead>(head_)),
        tail_(std::forward<UTail>(tail_)...) {}

  template <typename UHead, typename... UTail>
    requires(sizeof...(Tail) == sizeof...(UTail) &&
             is_constructable_from<const UHead&, const UTail&...> &&
             (sizeof...(Tail) != 0 ||
              !(std::is_constructible_v<Head, const Tuple<UHead>&> ||
                std::is_convertible_v<const Tuple<UHead>&, Head> ||
                std::is_same_v<Head, UHead>)))
  explicit(!is_convertable_from_back<UHead, UTail...>)
      Tuple(const Tuple<UHead, UTail...>& other)
      : head_(other.head_), tail_(other.tail_) {}

  template <typename UHead, typename... UTail>
    requires(sizeof...(Tail) == sizeof...(UTail) &&
             is_constructable_from<UHead &&, UTail && ...> &&
             (sizeof...(Tail) != 0 ||
              !(std::is_constructible_v<Head, Tuple<UHead> &&> ||
                std::is_convertible_v<Tuple<UHead> &&, Head> ||
                std::is_same_v<Head, UHead>)))
  explicit(!is_convertable_from_back<UHead&&, UTail&&...>)
      Tuple(Tuple<UHead, UTail...>&& other)
      : head_(std::forward<UHead>(other.head_)),
        tail_(std::move(other.tail_)) {}

  Tuple(const Tuple& other)
    requires(is_copy_constr)
      : head_(other.head_), tail_(other.tail_) {}

  // тут убрал requires(is_move_constr) потому что тот некорректный static_assert не проходил
  Tuple(Tuple&& other) noexcept
      : head_(std::move(other.head_)), tail_(std::move(other.tail_)) {}

  Tuple& operator=(const Tuple& other)
    requires(is_copy_assign)
  {
    if (this != &other) {
      head_ = other.head_;
      tail_ = other.tail_;
    }
    return *this;
  }  // TODO странно подчеркивает

  Tuple& operator=(Tuple&& other) noexcept
    requires(is_move_assign)
  {
    if (this != &other) {
      head_ = std::move(other.head_);
      tail_ = std::move(other.tail_);
    }
    return *this;
  }

  template <typename... UTypes>
    requires(sizeof...(Tail) + 1 == sizeof...(UTypes) &&
             is_assign_to_ref_from<const UTypes&...>)
  Tuple& operator=(const Tuple<UTypes...>& other) {
    head_ = other.head_;
    tail_ = other.tail_;
    return *this;
  }

  template <typename... UTypes>
    requires(sizeof...(Tail) + 1 == sizeof...(UTypes) &&
             is_assign_to_ref_from<UTypes && ...>)
  Tuple& operator=(Tuple<UTypes...>&& other) {
    head_ = std::forward<decltype(other.head_)>(other.head_);
    tail_ = std::move(other.tail_);
    return *this;
  }

  template <typename T, typename U>
  Tuple(const std::pair<T, U>& pr) : head_(pr.first), tail_(pr.second) {}

  template <typename T, typename U>
  Tuple(std::pair<T, U>&& pr)
      : head_(std::move(pr.first)), tail_(std::move(pr.second)) {}
};

template <typename T, typename U>
Tuple(const std::pair<T, U>&) -> Tuple<T, U>;

template <typename T, typename U>
Tuple(std::pair<T, U>&&) -> Tuple<T, U>;

template <>
class Tuple<> {
 private:
  template <typename... Args>
  friend class Tuple;

  constexpr Tuple() = default;

  constexpr Tuple(const Tuple&) = default;
  constexpr Tuple(Tuple&&)      = default;

  constexpr Tuple& operator=(const Tuple&) = default;
  constexpr Tuple& operator=(Tuple&&)      = default;
};

// почему то нет в utility, хотя должно быть с 23 плюсов
template <class T, class U>
[[nodiscard]] constexpr auto&& forward_like(U&& x) noexcept {
  constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
  if constexpr (std::is_lvalue_reference_v<T&&>) {
    if constexpr (is_adding_const)
      return std::as_const(x);
    else
      return static_cast<U&>(x);
  } else {
    if constexpr (is_adding_const)
      return std::move(std::as_const(x));
    else
      return std::move(x);
  }
}

template <size_t N, typename Tpl>
  requires(helper::is_Tuple_v<Tpl>)
constexpr decltype(auto) get(Tpl&& tuple) {
  if constexpr (N > 0) {
    return get<N - 1>(std::forward<Tpl>(tuple).tail_);
  } else {
    return forward_like<Tpl>(tuple.head_);
  }
}

template <typename T, typename... Types>
constexpr T& get(Tuple<Types...>& tuple) {
  if constexpr (std::is_same_v<decltype(tuple.head_), T>) {
    return tuple.head_;
  } else {
    return get<T>(tuple.tail_);
  }
}

template <typename T, typename... Types>
constexpr T&& get(Tuple<Types...>&& tuple) {
  if constexpr (std::is_same_v<decltype(tuple.head_), T>) {
    return tuple.head_;
  } else {
    return get<T>(tuple.tail_);
  }
}

template <typename T, typename... Types>
constexpr const T& get(const Tuple<Types...>& tuple) {
  if constexpr (std::is_same_v<decltype(tuple.head_), const T>) {
    return tuple.head_;
  } else {
    return get<T>(tuple.tail_);
  }
}

template <typename T, typename... Types>
constexpr const T&& get(const Tuple<Types...>&& tuple) {
  if constexpr (std::is_same_v<decltype(tuple.head_), const T>) {
    return tuple.head_;
  } else {
    return get<T>(tuple.tail_);
  }
}

template <class... Types>
constexpr Tuple<Types...> makeTuple(Types&&... args) {
  return Tuple<Types...>(std::forward<Types>(args)...);
}

template <typename... Tuples>
constexpr decltype(auto) tupleCat(Tuples&&... tuples) {
  using tpl = helper::concat_tuple_types_t<helper::bare_type_t<Tuples>...>;
  return tpl(helper::private_tuple_cat_tag(), std::forward<Tuples>(tuples)...);
}
