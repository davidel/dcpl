#pragma once

#include <type_traits>

namespace dcpl {

template <typename T>
using remove_cvr = typename std::remove_reference_t<typename std::remove_cv_t<T>>;

}
