#pragma once

#include <type_traits>

namespace dcpl {

template <typename T>
using remove_cvr = typename std::remove_reference_t<typename std::remove_cv_t<T>>;

template <typename T, typename... Ts>
struct one_of : std::disjunction<std::is_same<T, Ts>...> { };

template <typename T, typename... Ts>
constexpr bool one_of_v = one_of<T, Ts...>::value;

}

