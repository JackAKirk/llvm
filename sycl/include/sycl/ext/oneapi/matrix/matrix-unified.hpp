//===------- matrix-unified.hpp - SYCL matrix extension ----*- C++ -*------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// ===--------------------------------------------------------------------=== //

#pragma once
#include <sycl/ext/oneapi/matrix/matrix-tensorcores.hpp>

namespace sycl {
__SYCL_INLINE_VER_NAMESPACE(_V1) {
namespace ext {
namespace oneapi {
namespace experimental {
namespace matrix {

template <typename Group, typename T, size_t NumRows, size_t NumCols, use Use,
          layout Layout, typename T2>
inline __SYCL_ALWAYS_INLINE void
joint_matrix_fill(Group sg,
                  joint_matrix<T, Use, NumRows, NumCols, Layout, Group> &res,
                  const T2& v) {
  std::ignore = sg;
#if defined(__SYCL_DEVICE_ONLY__)
#if defined(__NVPTX__)
  res.wi_marray = v;
#endif // defined(__NVPTX__)
#else
  std::ignore = res;
  std::ignore = v;
  throw runtime_error(
      "This version of the matrix extension is only currently supported on "
      "Nvidia devices",
      PI_ERROR_INVALID_DEVICE);
#endif // defined(__SYCL_DEVICE_ONLY__)
}

template <typename Group, typename S, typename T, size_t NumRows,
          size_t NumCols, use Use, access::address_space Space,
          std::enable_if_t<std::is_same<S, std::remove_const_t<T>>::value,
                           bool> = true>
void joint_matrix_load(
    Group sg,
    joint_matrix<S, Use, NumRows, NumCols,
                 sycl::ext::oneapi::experimental::matrix::layout::dynamic,
                 Group> &res,
    multi_ptr<T, Space> src, size_t stride,
    sycl::ext::oneapi::experimental::matrix::layout LayoutAcc) {
#if defined(__SYCL_DEVICE_ONLY__)
#if defined(__NVPTX__)
  sycl::ext::oneapi::detail::load_accumulator_cuda(res, src, stride, LayoutAcc);
#endif // defined(__NVPTX__)
#else
  std::ignore = sg;
  std::ignore = res;
  std::ignore = src;
  std::ignore = stride;
  throw runtime_error(
      "This version of the matrix extension is only currently supported on "
      "Nvidia devices",
      PI_ERROR_INVALID_DEVICE);
#endif // defined(__SYCL_DEVICE_ONLY__)
}

template <
    typename Group, typename S, typename T, use Use, size_t NumRows,
    size_t NumCols, matrix::layout Layout, access::address_space Space,
    std::enable_if_t<std::is_same<S, std::remove_const_t<T>>::value ||
                         (std::is_same<S, precision::tf32>::value &&
                          std::is_same<std::remove_const_t<T>, float>::value),
                     bool> = true>
void joint_matrix_load(
    Group sg, joint_matrix<S, Use, NumRows, NumCols, Layout, Group> &res,
    multi_ptr<T, Space> src, size_t stride) {
#if defined(__SYCL_DEVICE_ONLY__)
#if defined(__NVPTX__)
  sycl::ext::oneapi::detail::load_multiplicand_cuda<S, T, NumRows, NumCols, Use,
                                                    Layout, Space>(res, src,
                                                                   stride);
#endif // defined(__NVPTX__)
#else
  std::ignore = sg;
  std::ignore = res;
  std::ignore = src;
  std::ignore = stride;
  throw runtime_error(
      "This version of the matrix extension is only currently supported on "
      "Nvidia devices",
      PI_ERROR_INVALID_DEVICE);
#endif // defined(__SYCL_DEVICE_ONLY__)
}

template <typename Group, typename T, size_t NumRows, size_t NumCols,
          access::address_space Space>
void joint_matrix_store(
    Group sg,
    joint_matrix<T, use::accumulator, NumRows, NumCols,
                 sycl::ext::oneapi::experimental::matrix::layout::dynamic,
                 Group> &src,
    multi_ptr<T, Space> dst, size_t stride,
    sycl::ext::oneapi::experimental::matrix::layout LayoutAcc) {
#if defined(__SYCL_DEVICE_ONLY__)
#if defined(__NVPTX__)
  sycl::ext::oneapi::detail::joint_matrix_store_cuda<T, NumRows, NumCols,
                                                     Space>(src, dst, stride,
                                                            LayoutAcc);
#endif // defined(__NVPTX__)
#else
  std::ignore = sg;
  std::ignore = src;
  std::ignore = dst;
  std::ignore = stride;
  throw runtime_error(
      "This version of the matrix extension is only currently supported on "
      "Nvidia devices",
      PI_ERROR_INVALID_DEVICE);
#endif // defined(__SYCL_DEVICE_ONLY__)
}

template <typename Group, typename Ta, typename Tb, typename Tc, std::size_t M,
          std::size_t K, std::size_t N, layout LayoutA, layout LayoutB>
joint_matrix<Tc, use::accumulator, M, N,
             sycl::ext::oneapi::experimental::matrix::layout::dynamic, Group>
joint_matrix_mad(
    Group sg, joint_matrix<Ta, use::a, M, K, LayoutA, Group> &A,
    joint_matrix<Tb, use::b, K, N, LayoutB, Group> &B,
    joint_matrix<Tc, use::accumulator, M, N,
                 sycl::ext::oneapi::experimental::matrix::layout::dynamic,
                 Group> &C) {
#if defined(__SYCL_DEVICE_ONLY__)
#if defined(__NVPTX__)
  if constexpr (std::is_same<Ta, Tb>::value) {
    joint_matrix<Tc, use::accumulator, M, N,
                 sycl::ext::oneapi::experimental::matrix::layout::dynamic,
                 Group>
        D;
    sycl::ext::oneapi::detail::joint_matrix_mad_cuda<Ta, Tc, M, K, N, LayoutA,
                                                     LayoutB>(D, A, B, C);
    return D;
  } else {
    assert(false && "Ta != Tb : In the CUDA backend joint_matrix_mad "
                    "requires that joint_matrix data types Ta and Tb match");
  }
#endif // defined(__NVPTX__)
#else
  std::ignore = sg;
  std::ignore = A;
  std::ignore = B;
  std::ignore = C;
  throw runtime_error(
      "This version of the matrix extension is only currently supported on "
      "Nvidia devices",
      PI_ERROR_INVALID_DEVICE);
#endif // defined(__SYCL_DEVICE_ONLY__)
}

// This function rounds the bottom 13 bits up or down, and then zeros out the
// bottom bits
float round_to_tf32(float &a) {
#if defined(__SYCL_DEVICE_ONLY__) && defined(__NVPTX__)
  int32_t tmp_int = __nvvm_f2tf32_rna(a);
  return __nvvm_bitcast_i2f(tmp_int);
#else
  uint32_t tmp_uint = reinterpret_cast<uint32_t &>(a);
  tmp_uint += 0x1000u;
  tmp_uint &= 0xFFFFE000u;
  float ret = reinterpret_cast<float &>(tmp_uint);
  return ret;
#endif // defined(__SYCL_DEVICE_ONLY__) && defined(__NVPTX__)
}

} // namespace matrix
} // namespace experimental
} // namespace oneapi
} // namespace ext
} // __SYCL_INLINE_VER_NAMESPACE(_V1)
} // namespace sycl

