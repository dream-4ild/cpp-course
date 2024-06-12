//
// Created by dmitry on 6/12/24.
//

#pragma once

#include <type_traits>

namespace helper {
template <typename T, typename = std::void_t<>>
struct is_compiled_copy_list_initial_from_empty : std::false_type {};

template <typename T>
struct is_compiled_copy_list_initial_from_empty<T, std::void_t<decltype(T{})>>
    : std::true_type {};

}  // namespace helper

template <typename...>
class Tuple;

namespace helper {
template <size_t N, typename Head, typename... Tail>
struct Nth_type : Nth_type<N - 1, Tail...> {};

template <typename Head, typename... Tail>
struct Nth_type<0, Head, Tail...> {
  using type = Head;
};

template <size_t, typename...>
struct Nth_type_in_tuple;

template <size_t N, typename... Args>
struct Nth_type_in_tuple<N, Tuple<Args...>> {
  using type = Nth_type<N, Args...>::type;
};

template <size_t N, typename Tpl>
using Nth_type_in_tuple_t = Nth_type_in_tuple<N, Tpl>::type;

}  // namespace helper

template <typename Head, typename... Tail>
class Tuple<Head, Tail...> {
 private:
  Head head_;
  [[no_unique_address]] Tuple<Tail...> tail_;

  using TL = Tuple<Tail...>;

  static constexpr bool is_default_constr =
      std::is_default_constructible_v<Head> && TL::is_default_constr;

  static constexpr bool copy_list_initial_from_empty =
      helper::is_compiled_copy_list_initial_from_empty<Head>::value &&
      TL::copy_list_initial_from_empty;

  static constexpr bool is_copy_constr =
      std::is_copy_constructible_v<Head> && TL::is_copy_constr;

  static constexpr bool is_move_constr =
      std::is_move_constructible_v<Head> && TL::is_move_constr;

  static constexpr bool is_convertible_from_const_ref =
      std::is_convertible_v<const Head&, Head> &&
      TL::is_convertible_from_const_ref;

  template <typename HHead, typename... TTail>
  static constexpr bool is_constructable_from =
      std::is_constructible_v<Head, HHead> &&
      TL::template is_constructable_from<TTail...>;

  template <typename HHead, typename... TTail>
  static constexpr bool is_convertable_from =
      std::is_convertible_v<Head, HHead> &&
      TL::template is_convertable_from<TTail...>;

  template <typename HHead, typename... TTail>
  static constexpr bool is_convertable_from_back =
      std::is_convertible_v<HHead, Head> &&
      TL::template is_convertable_from_back<TTail...>;

  template <typename HHead, typename... TTail>
  static constexpr bool is_convertable_from_back_with_double_ampersand =
      std::is_convertible_v<HHead&&, Head> &&
      TL::template is_convertable_from_back_with_double_ampersand<TTail...>;

  template <typename HHead, typename... TTail>
  static constexpr bool is_constructable_from_with_double_ampersand =
      std::is_constructible_v<Head, HHead&&> &&
      TL::template is_constructable_from_with_double_ampersand<TTail...>;

  template <typename... Args>
  friend class Tuple;

  template <size_t N, typename Tpl>
  friend constexpr auto get(Tpl&&)
      -> helper::Nth_type_in_tuple_t<N, std::remove_reference_t<Tpl>>&;

 public:
  template <typename = std::void_t<>>
    requires(is_default_constr)
  constexpr explicit(!copy_list_initial_from_empty) Tuple() {}

  template <typename = std::void_t<>>
    requires(is_copy_constr)
  constexpr explicit(!is_convertible_from_const_ref)
      Tuple(const Head& head_, const Tail&... tail_)
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
             is_constructable_from<UHead, UTail...> &&
             (sizeof...(Tail) != 0 ||
              !(std::is_constructible_v<Head, const Tuple<UHead>&> ||
                std::is_convertible_v<const Tuple<UHead>&, Head> ||
                std::is_same_v<Head, UHead>)))
  explicit(!is_convertable_from_back<UHead, UTail...>)
      Tuple(const Tuple<UHead, UTail...>& other)
      : head_(other.head_), tail_(other.tail_) {}

  template <typename UHead, typename... UTail>
    requires(sizeof...(Tail) == sizeof...(UTail) &&
             is_constructable_from_with_double_ampersand<UHead, UTail...> &&
             (sizeof...(Tail) != 0 ||
              !(std::is_constructible_v<Head, Tuple<UHead> &&> ||
                std::is_convertible_v<Tuple<UHead> &&, Head> ||
                std::is_same_v<Head,
                               UHead>)))  ///// лажа со спусками и выше тоже
  explicit(!is_convertable_from_back_with_double_ampersand<UHead, UTail...>)
      Tuple(Tuple<UHead, UTail...>&& other)
      : head_(std::move(other.head_)), tail_(std::move(other.tail_)) {}

  template <typename = std::void_t<>>
    requires(is_copy_constr)
  constexpr Tuple(const Tuple& other)
      : head_(other.head_), tail_(other.tail_) {}

  template <typename = std::void_t<>>
    requires(is_move_constr)
  constexpr Tuple(Tuple&& other)
      : head_(std::move(other.head_)), tail_(std::move(other.tail_)) {}
};

template <>
class Tuple<> {
 private:
  static constexpr bool is_default_constr             = true;
  static constexpr bool copy_list_initial_from_empty  = true;
  static constexpr bool is_copy_constr                = true;
  static constexpr bool is_move_constr                = true;
  static constexpr bool is_convertible_from_const_ref = true;

  template <typename...>
  static constexpr bool is_constructable_from = true;

  template <typename...>
  static constexpr bool is_constructable_from_with_double_ampersand = true;

  template <typename...>
  static constexpr bool is_convertable_from = true;

  template <typename...>
  static constexpr bool is_convertable_from_back_with_double_ampersand = true;

  template <typename...>
  static constexpr bool is_convertable_from_back = true;

  template <typename... Args>
  friend class Tuple;

  Tuple()             = default;
  Tuple(const Tuple&) = default;
  Tuple(Tuple&&)      = default;
};

namespace helper {}  // namespace helper

template <size_t N, typename Tpl>
constexpr auto get(Tpl&& tuple)
    -> helper::Nth_type_in_tuple_t<N, std::remove_reference_t<Tpl>>& {
  if constexpr (N > 0) {
    return get<N - 1>(std::forward<Tpl>(tuple).tail_);
  } else {
    return tuple.head_;
  }
}