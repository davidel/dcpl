#pragma once

#include <algorithm>
#include <climits>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "dcpl/assert.h"
#include "dcpl/stdns_override.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"

namespace dcpl {

namespace py = pybind11;

template <typename T>
std::span<const T> unpack_buffer(const py::buffer_info& buf) {
  DCPL_CHECK_EQ(buf.shape.size(), 1) << "Only 1D vectors can be unpacked to spans: "
                                     << buf.shape;

  return std::span<const T>(reinterpret_cast<const T*>(buf.ptr), buf.shape[0]);
}

template <typename S, typename T>
py::array_t<S> vec2array(const std::vector<T>& vec) {
  py::array_t<S> arr(vec.size());
  S* ptr = arr.mutable_data();

  for (const auto& v : vec) {
    *ptr++ = static_cast<S>(v);
  }

  return arr;
}

template <typename C>
py::tuple to_tuple(const C& vals) {
  py::tuple tuple(vals.size());
  std::size_t i = 0;

  for (const auto& v : vals) {
    tuple[i++] = v;
  }

  return tuple;
}

template <typename T, typename P>
std::vector<T> py_seq2vec(const P& vals) {
  std::vector<T> v;

  v.reserve(vals.size());
  for (const auto& val : vals) {
    v.push_back(py::cast<T>(val));
  }

  return v;
}

template <typename T>
std::vector<T> to_vector(const py::tuple& vals) {
  return py_seq2vec<T>(vals);
}

template <typename T>
std::vector<T> to_vector(const py::list& vals) {
  return py_seq2vec<T>(vals);
}

template <typename T>
std::vector<T> to_vector(const py::args& vals) {
  return py_seq2vec<T>(vals);
}

template <typename T, typename P, typename F>
void for_each_arg(const P& args, const F& fn) {
  for (const auto& arg : args) {
    fn(py::cast<T>(arg));
  }
}

struct slicevals {
  py::ssize_t start = 0;
  py::ssize_t stop = 0;
  py::ssize_t step = 0;
  py::ssize_t length = 0;
};

inline slicevals deslice(std::size_t size, const py::slice& py_slice) {
  slicevals slice;

  DCPL_ASSERT(py_slice.compute(size, &slice.start, &slice.stop,
                               &slice.step, &slice.length))
      << "Unable to compute slice arguments";

  return slice;
}

template <typename F>
void for_each_slice(const slicevals& slice, const F& fn) {
  if (slice.step > 0) {
    for (py::ssize_t i = slice.start; i < slice.stop; i += slice.step) {
      fn(i);
    }
  } else if (slice.step < 0) {
    for (py::ssize_t i = slice.start; i > slice.stop; i += slice.step) {
      fn(i);
    }
  } else {
    DCPL_CHECK_EQ(slice.start, slice.stop) << "Invalid slice: start=" << slice.start
                                           << " stop=" << slice.stop << " step="
                                           << slice.step;
  }
}

template <typename S>
py::object get_object(const py::dict& dict, const S& key) {
  py::object kobj = py::cast(key);
  PyObject* value = PyDict_GetItem(dict.ptr(), kobj.ptr());

  return value != nullptr ? py::reinterpret_steal<py::object>(value) : py::none();
}

template <typename T, typename S>
std::optional<T> get_value(const py::dict& dict, const S& key) {
  py::object value = get_object(dict, key);

  if (value.is_none()) {
    return std::nullopt;
  }

  return value.cast<T>();
}

template <typename T, typename S>
T get_value_or(const py::dict& dict, const S& key, const T& defval) {
  std::optional<T> value = get_value<T>(dict, key);

  return value.value_or(defval);
}

inline umaxint_t to_maxint(const py::object& value) {
  umaxint_t result =
      static_cast<umaxint_t>(PyLong_AsUnsignedLongLongMask(value.ptr()));
  std::size_t ull_nbits = CHAR_BIT * sizeof(unsigned long long);

  for (std::size_t nbits = ull_nbits; nbits < MAXINT_NBITS; nbits += ull_nbits) {
    py::object shift = py::reinterpret_steal<py::object>(PyLong_FromSize_t(nbits));
    py::object sh_value =
        py::reinterpret_steal<py::object>(PyNumber_Rshift(value.ptr(), shift.ptr()));

    result |= static_cast<umaxint_t>(PyLong_AsUnsignedLongLongMask(sh_value.ptr())) << nbits;
  }

  return result;
}

inline py::object from_maxint(umaxint_t value) {
  py::object result = py::reinterpret_steal<py::object>(PyLong_FromUnsignedLongLong(
      static_cast<unsigned long long>(value)));
  std::size_t ull_nbits = CHAR_BIT * sizeof(unsigned long long);

  for (std::size_t nbits = ull_nbits; nbits < MAXINT_NBITS && (value >> nbits) != 0;
       nbits += ull_nbits) {
    py::object part = py::reinterpret_steal<py::object>(PyLong_FromUnsignedLongLong(
        static_cast<unsigned long long>(value >> nbits)));
    py::object shift = py::reinterpret_steal<py::object>(PyLong_FromSize_t(nbits));
    py::object sh_value =
        py::reinterpret_steal<py::object>(PyNumber_Lshift(part.ptr(), shift.ptr()));

    result = py::reinterpret_steal<py::object>(PyNumber_Or(sh_value.ptr(), result.ptr()));
  }

  return result;
}

}

