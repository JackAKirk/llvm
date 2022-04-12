//===---- matrix-tensorcore.hpp - SYCL tensor cores matrix ----*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// ===--------------------------------------------------------------------=== //

#pragma once
#include <sycl/ext/oneapi/experimental/bfloat16.hpp>

__SYCL_INLINE_NAMESPACE(cl) {
namespace sycl {
namespace ext {
namespace oneapi {
namespace experimental::matrix {

enum class matrix_use { a, b, accumulator };

enum class matrix_layout { row_major, col_major, packed_a, packed_b };

template <typename T, matrix_use Use, size_t Rows = sycl::dynamic_extent,
          size_t Cols = sycl::dynamic_extent,
          matrix_layout Layout = matrix_layout::row_major,
          typename Group = sycl::sub_group, typename Cond = void>
struct joint_matrix;

#define __SYCL_JOINT_MATRIX_OVERLOAD(type, use, M, N, frag_type, frag_size)    \
  template <matrix_layout Layout>                                              \
  struct joint_matrix<                                                         \
      type, matrix_use::use, M, N, Layout, sycl::sub_group,                    \
      typename std::enable_if_t<Layout == matrix_layout::row_major ||          \
                                Layout == matrix_layout::col_major>> {         \
    frag_type data[frag_size];                                                 \
  };

#define __SYCL_JOINT_MATRIX_OVERLOAD_ARR(type, use, M, N, size)    \
  template <matrix_layout Layout>                                              \
  struct joint_matrix<                                                         \
      type, matrix_use::use, M, N, Layout, sycl::sub_group,                    \
      typename std::enable_if_t<Layout == matrix_layout::row_major ||          \
                                Layout == matrix_layout::col_major>> {         \
    sycl::marray<type, size> data;                                                 \
  };

__SYCL_JOINT_MATRIX_OVERLOAD_ARR(bfloat16, a, 16, 16, 8)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(bfloat16, b, 16, 16, 8)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(bfloat16, a, 8, 16, 4)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(bfloat16, b, 16, 32, 16)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(bfloat16, a, 32, 16, 16)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(bfloat16, b, 16, 8, 4)

// TODO this case needs to be treated carefully for element wise ops: The number of fragments actually used depends on SM version
// m8n32k16
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(half, a, 8, 16, 16)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(half, b, 16, 32, 16)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(half, accumulator, 8, 32, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD_ARR(float, accumulator, 8, 32, 8)

__SYCL_JOINT_MATRIX_OVERLOAD_ARR(half, a, 32, 16, 16)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(half, b, 16, 8, 16)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(half, accumulator, 32, 8, 8)

__SYCL_JOINT_MATRIX_OVERLOAD_ARR(half, a, 16, 16, 16)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(half, b, 16, 16, 16)
__SYCL_JOINT_MATRIX_OVERLOAD_ARR(half, accumulator, 16, 16, 8)

#undef __SYCL_JOINT_MATRIX_OVERLOAD_ARR

// m8n8k4 double only
__SYCL_JOINT_MATRIX_OVERLOAD(double, a, 8, 4, double, 1)
__SYCL_JOINT_MATRIX_OVERLOAD(double, b, 4, 8, double, 1)
__SYCL_JOINT_MATRIX_OVERLOAD(double, accumulator, 8, 8, double, 2)

// m8n32k16
// bf16 data format uint16_t implementation is deprecated
__SYCL_JOINT_MATRIX_OVERLOAD(uint16_t, a, 8, 16, int32_t, 2)
__SYCL_JOINT_MATRIX_OVERLOAD(uint16_t, b, 16, 32, int32_t, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD(bfloat16, a, 8, 16, int32_t, 2)
//__SYCL_JOINT_MATRIX_OVERLOAD(bfloat16, b, 16, 32, int32_t, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD(half, a, 8, 16, int32_t, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD(half, b, 16, 32, int32_t, 8)
__SYCL_JOINT_MATRIX_OVERLOAD(float, accumulator, 8, 32, float, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD(half, accumulator, 8, 32, int32_t, 4)

__SYCL_JOINT_MATRIX_OVERLOAD(int8_t, a, 8, 16, int32_t, 1)
__SYCL_JOINT_MATRIX_OVERLOAD(int8_t, b, 16, 32, int32_t, 4)
__SYCL_JOINT_MATRIX_OVERLOAD(uint8_t, a, 8, 16, int32_t, 1)
__SYCL_JOINT_MATRIX_OVERLOAD(uint8_t, b, 16, 32, int32_t, 4)
__SYCL_JOINT_MATRIX_OVERLOAD(int32_t, accumulator, 8, 32, int32_t, 8)

// m32n8k16
__SYCL_JOINT_MATRIX_OVERLOAD(uint16_t, a, 32, 16, int32_t, 8)
__SYCL_JOINT_MATRIX_OVERLOAD(uint16_t, b, 16, 8, int32_t, 2)
//__SYCL_JOINT_MATRIX_OVERLOAD(bfloat16, a, 32, 16, int32_t, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD(bfloat16, b, 16, 8, int32_t, 2)
//__SYCL_JOINT_MATRIX_OVERLOAD(half, a, 32, 16, int32_t, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD(half, b, 16, 8, int32_t, 8)
__SYCL_JOINT_MATRIX_OVERLOAD(float, accumulator, 32, 8, float, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD(half, accumulator, 32, 8, int32_t, 4)

__SYCL_JOINT_MATRIX_OVERLOAD(int8_t, a, 32, 16, int32_t, 4)
__SYCL_JOINT_MATRIX_OVERLOAD(int8_t, b, 16, 8, int32_t, 1)
__SYCL_JOINT_MATRIX_OVERLOAD(uint8_t, a, 32, 16, int32_t, 4)
__SYCL_JOINT_MATRIX_OVERLOAD(uint8_t, b, 16, 8, int32_t, 1)
__SYCL_JOINT_MATRIX_OVERLOAD(int32_t, accumulator, 32, 8, int32_t, 8)

// m16n16k16
__SYCL_JOINT_MATRIX_OVERLOAD(uint16_t, a, 16, 16, int32_t, 4)
__SYCL_JOINT_MATRIX_OVERLOAD(uint16_t, b, 16, 16, int32_t, 4)
//__SYCL_JOINT_MATRIX_OVERLOAD(half, a, 16, 16, int32_t, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD(half, b, 16, 16, int32_t, 8)
__SYCL_JOINT_MATRIX_OVERLOAD(float, accumulator, 16, 16, float, 8)
//__SYCL_JOINT_MATRIX_OVERLOAD(half, accumulator, 16, 16, int32_t, 4)

__SYCL_JOINT_MATRIX_OVERLOAD(int8_t, a, 16, 16, int32_t, 2)
__SYCL_JOINT_MATRIX_OVERLOAD(int8_t, b, 16, 16, int32_t, 2)
__SYCL_JOINT_MATRIX_OVERLOAD(uint8_t, a, 16, 16, int32_t, 2)
__SYCL_JOINT_MATRIX_OVERLOAD(uint8_t, b, 16, 16, int32_t, 2)
__SYCL_JOINT_MATRIX_OVERLOAD(int32_t, accumulator, 16, 16, int32_t, 8)

#undef __SYCL_JOINT_MATRIX_OVERLOAD
} // namespace experimental::matrix

namespace detail {

template <typename T, sycl::ext::oneapi::experimental::matrix::matrix_use Use,
          size_t NumRows, size_t NumCols,
          sycl::ext::oneapi::experimental::matrix::matrix_layout Layout,
          access::address_space Space, typename Cond = void>
struct joint_matrix_load_impl {
  void load(sycl::ext::oneapi::experimental::matrix::joint_matrix<
                T, Use, NumRows, NumCols, Layout, sycl::sub_group> &res,
            multi_ptr<T, Space> src, size_t stride);
};

template <sycl::ext::oneapi::experimental::matrix::matrix_layout Layout>
constexpr int get_layout_id();

template <>
constexpr int get_layout_id<
    sycl::ext::oneapi::experimental::matrix::matrix_layout::row_major>() {
  return 0;
}

template <>
constexpr int get_layout_id<
    sycl::ext::oneapi::experimental::matrix::matrix_layout::col_major>() {
  return 1;
}

template <typename T, sycl::ext::oneapi::experimental::matrix::matrix_use Use,
          size_t NumRows, size_t NumCols,
          sycl::ext::oneapi::experimental::matrix::matrix_layout Layout,
          access::address_space Space>
struct joint_matrix_load_impl<
    T, Use, NumRows, NumCols, Layout, Space,
    typename std::enable_if_t<Layout == sycl::ext::oneapi::experimental::
                                            matrix::matrix_layout::row_major ||
                              Layout == sycl::ext::oneapi::experimental::
                                            matrix::matrix_layout::col_major>> {
  void load(sycl::ext::oneapi::experimental::matrix::joint_matrix<
                T, Use, NumRows, NumCols, Layout, sycl::sub_group> &res,
            multi_ptr<T, Space> src, size_t stride) {
    if constexpr (std::is_same_v<T, uint16_t> ||
                  std::is_same_v<
                      T, sycl::ext::oneapi::experimental::bfloat16>) {
      auto tileptr = reinterpret_cast<int32_t const *>(src.get());
      auto destptr = reinterpret_cast<int32_t *>(&res.data);
      if constexpr (NumRows == 16 && NumCols == 16) {
        if constexpr (Use ==
                      sycl::ext::oneapi::experimental::matrix::matrix_use::a) {
          __mma_bf16_m16n16k16_ld_a(destptr, tileptr, stride,
                                    get_layout_id<Layout>());
        } else if constexpr (Use == sycl::ext::oneapi::experimental::matrix::
                                        matrix_use::b) {
          __mma_bf16_m16n16k16_ld_b(destptr, tileptr, stride,
                                    get_layout_id<Layout>());
        }
      } else if constexpr (NumRows == 8 && NumCols == 16) {
        __mma_bf16_m8n32k16_ld_a(destptr, tileptr, stride,
                                 get_layout_id<Layout>());
      } else if constexpr (NumRows == 16 && NumCols == 32) {
        __mma_bf16_m8n32k16_ld_b(destptr, tileptr, stride,
                                 get_layout_id<Layout>());
      } else if constexpr (NumRows == 32 && NumCols == 16) {
        __mma_bf16_m32n8k16_ld_a(destptr, tileptr, stride,
                                 get_layout_id<Layout>());
      } else if constexpr (NumRows == 16 && NumCols == 8) {
        __mma_bf16_m32n8k16_ld_b(destptr, tileptr, stride,
                                 get_layout_id<Layout>());
      }
    } else if constexpr (std::is_same_v<T, uint8_t>) {
      auto tileptr = reinterpret_cast<int32_t const *>(src.get());
      if constexpr (NumRows == 16 && NumCols == 16) {
        if constexpr (Use ==
                      sycl::ext::oneapi::experimental::matrix::matrix_use::a) {
          __imma_m16n16k16_ld_a_u8(res.data, tileptr, stride,
                                   get_layout_id<Layout>());
        } else if constexpr (Use == sycl::ext::oneapi::experimental::matrix::
                                        matrix_use::b) {
          __imma_m16n16k16_ld_b_u8(res.data, tileptr, stride,
                                   get_layout_id<Layout>());
        }
      } else if constexpr (NumRows == 8 && NumCols == 16) {
        __imma_m8n32k16_ld_a_u8(res.data, tileptr, stride,
                                get_layout_id<Layout>());
      } else if constexpr (NumRows == 16 && NumCols == 32) {
        __imma_m8n32k16_ld_b_u8(res.data, tileptr, stride,
                                get_layout_id<Layout>());
      } else if constexpr (NumRows == 32 && NumCols == 16) {
        __imma_m32n8k16_ld_a_u8(res.data, tileptr, stride,
                                get_layout_id<Layout>());
      } else if constexpr (NumRows == 16 && NumCols == 8) {
        __imma_m32n8k16_ld_b_u8(res.data, tileptr, stride,
                                get_layout_id<Layout>());
      }
    } else if constexpr (std::is_same_v<T, int8_t>) {
      auto tileptr = reinterpret_cast<int32_t const *>(src.get());
      if constexpr (NumRows == 16 && NumCols == 16) {
        if constexpr (Use ==
                      sycl::ext::oneapi::experimental::matrix::matrix_use::a) {
          __imma_m16n16k16_ld_a_s8(res.data, tileptr, stride,
                                   get_layout_id<Layout>());
        } else if constexpr (Use == sycl::ext::oneapi::experimental::matrix::
                                        matrix_use::b) {
          __imma_m16n16k16_ld_b_s8(res.data, tileptr, stride,
                                   get_layout_id<Layout>());
        }
      } else if constexpr (NumRows == 8 && NumCols == 16) {
        __imma_m8n32k16_ld_a_s8(res.data, tileptr, stride,
                                get_layout_id<Layout>());
      } else if constexpr (NumRows == 16 && NumCols == 32) {
        __imma_m8n32k16_ld_b_s8(res.data, tileptr, stride,
                                get_layout_id<Layout>());
      } else if constexpr (NumRows == 32 && NumCols == 16) {
        __imma_m32n8k16_ld_a_s8(res.data, tileptr, stride,
                                get_layout_id<Layout>());
      } else if constexpr (NumRows == 16 && NumCols == 8) {
        __imma_m32n8k16_ld_b_s8(res.data, tileptr, stride,
                                get_layout_id<Layout>());
      }
    } else if constexpr (std::is_same_v<T, half>) {
      auto tileptr = reinterpret_cast<int32_t const *>(src.get());
      auto dstptr = reinterpret_cast<int32_t *>(&res.data);
      if constexpr (NumRows == 16 && NumCols == 16) {
        if constexpr (Use ==
                      sycl::ext::oneapi::experimental::matrix::matrix_use::a) {
          __hmma_m16n16k16_ld_a(dstptr, tileptr, stride,
                                get_layout_id<Layout>());
        } else if constexpr (Use == sycl::ext::oneapi::experimental::matrix::
                                        matrix_use::b) {
          __hmma_m16n16k16_ld_b(dstptr, tileptr, stride,
                                get_layout_id<Layout>());
        } else if constexpr (Use == sycl::ext::oneapi::experimental::matrix::
                                        matrix_use::accumulator) {
          __hmma_m16n16k16_ld_c_f16(dstptr, tileptr, stride,
                                    get_layout_id<Layout>());
        }
      } else if constexpr (NumRows == 8 && NumCols == 16) {
        __hmma_m8n32k16_ld_a(dstptr, tileptr, stride,
                             get_layout_id<Layout>());
      } else if constexpr (NumRows == 16 && NumCols == 32) {
        __hmma_m8n32k16_ld_b(dstptr, tileptr, stride,
                             get_layout_id<Layout>());
      } else if constexpr (NumRows == 32 && NumCols == 16) {
        __hmma_m32n8k16_ld_a(dstptr, tileptr, stride,
                             get_layout_id<Layout>());
      } else if constexpr (NumRows == 16 && NumCols == 8) {
        __hmma_m32n8k16_ld_b(dstptr, tileptr, stride,
                             get_layout_id<Layout>());
      } else if constexpr (NumRows == 32 && NumCols == 8) {
        __hmma_m32n8k16_ld_c_f16(dstptr, tileptr, stride,
                                 get_layout_id<Layout>());
      } else if constexpr (NumRows == 8 && NumCols == 32) {
        __hmma_m8n32k16_ld_c_f16(dstptr, tileptr, stride,
                                 get_layout_id<Layout>());
      }

    } else if constexpr (std::is_same_v<T, int32_t>) {
      if constexpr (NumRows == 16 && NumCols == 16) {
        __imma_m16n16k16_ld_c(res.data, src.get(), stride,
                              get_layout_id<Layout>());
      } else if constexpr (NumRows == 8 && NumCols == 32) {
        __imma_m8n32k16_ld_c(res.data, src.get(), stride,
                             get_layout_id<Layout>());
      } else if constexpr (NumRows == 32 && NumCols == 8) {
        __imma_m32n8k16_ld_c(res.data, src.get(), stride,
                             get_layout_id<Layout>());
      }
    } else if constexpr (std::is_same_v<T, float>) {
      if constexpr (NumRows == 16 && NumCols == 16) {
        __hmma_m16n16k16_ld_c_f32(res.data, src.get(), stride,
                                  get_layout_id<Layout>());
      } else if constexpr (NumRows == 8 && NumCols == 32) {
        __hmma_m8n32k16_ld_c_f32(res.data, src.get(), stride,
                                 get_layout_id<Layout>());
      } else if constexpr (NumRows == 32 && NumCols == 8) {
        __hmma_m32n8k16_ld_c_f32(res.data, src.get(), stride,
                                 get_layout_id<Layout>());
      }
    } else if constexpr (std::is_same_v<T, double>) {
      if constexpr (Use ==
                    sycl::ext::oneapi::experimental::matrix::matrix_use::a) {
        __dmma_m8n8k4_ld_a(res.data, src.get(), stride,
                           get_layout_id<Layout>());
      } else if constexpr (Use == sycl::ext::oneapi::experimental::matrix::
                                      matrix_use::b) {
        __dmma_m8n8k4_ld_b(res.data, src.get(), stride,
                           get_layout_id<Layout>());
      } else if constexpr (Use == sycl::ext::oneapi::experimental::matrix::
                                      matrix_use::accumulator) {
        __dmma_m8n8k4_ld_c(res.data, src.get(), stride,
                           get_layout_id<Layout>());
      }
    }
  }
};

template <typename T, size_t NumRows, size_t NumCols,
          sycl::ext::oneapi::experimental::matrix::matrix_layout Layout,
          access::address_space Space, typename Cond = void>
struct joint_matrix_store_impl {
  void
  store(sycl::ext::oneapi::experimental::matrix::joint_matrix<
            T, sycl::ext::oneapi::experimental::matrix::matrix_use::accumulator,
            NumRows, NumCols, Layout, sycl::sub_group> &src,
        multi_ptr<T, Space> dst, size_t stride);
};

template <typename T, size_t NumRows, size_t NumCols,
          sycl::ext::oneapi::experimental::matrix::matrix_layout Layout,
          access::address_space Space>
struct joint_matrix_store_impl<
    T, NumRows, NumCols, Layout, Space,
    typename std::enable_if_t<Layout == sycl::ext::oneapi::experimental::
                                            matrix::matrix_layout::row_major ||
                              Layout == sycl::ext::oneapi::experimental::
                                            matrix::matrix_layout::col_major>> {
  void
  store(sycl::ext::oneapi::experimental::matrix::joint_matrix<
            T, sycl::ext::oneapi::experimental::matrix::matrix_use::accumulator,
            NumRows, NumCols, Layout, sycl::sub_group> &src,
        multi_ptr<T, Space> dst, size_t stride) {
    if (NumRows == 16 && NumCols == 16) {
      if constexpr (std::is_same_v<T, float>) {
        __hmma_m16n16k16_st_c_f32(dst.get(), src.data, stride,
                                  get_layout_id<Layout>());
      } else if constexpr (std::is_same_v<T, int32_t>) {
        __imma_m16n16k16_st_c_i32(dst.get(), src.data, stride,
                                  get_layout_id<Layout>());
      } else if constexpr (std::is_same_v<T, half>) {
        auto tileptr = reinterpret_cast<int32_t *>(dst.get());
        auto srcptr = reinterpret_cast<int32_t *>(&src.data);
        __hmma_m16n16k16_st_c_f16(tileptr, srcptr, stride,
                                  get_layout_id<Layout>());
      }
    } else if (NumRows == 8 && NumCols == 32) {
      if constexpr (std::is_same_v<T, float>) {
        __hmma_m8n32k16_st_c_f32(dst.get(), src.data, stride,
                                 get_layout_id<Layout>());
      } else if constexpr (std::is_same_v<T, int32_t>) {
        __imma_m8n32k16_st_c_i32(dst.get(), src.data, stride,
                                 get_layout_id<Layout>());
      } else if constexpr (std::is_same_v<T, half>) {
        auto tileptr = reinterpret_cast<int32_t *>(dst.get());
        auto srcptr = reinterpret_cast<int32_t  *>(&src.data);
        __hmma_m8n32k16_st_c_f16(tileptr, srcptr, stride,
                                 get_layout_id<Layout>());
      }
    } else if (NumRows == 32 && NumCols == 8) {
      if constexpr (std::is_same_v<T, float>) {
        __hmma_m32n8k16_st_c_f32(dst.get(), src.data, stride,
                                 get_layout_id<Layout>());
      } else if constexpr (std::is_same_v<T, int32_t>) {
        __imma_m32n8k16_st_c_i32(dst.get(), src.data, stride,
                                 get_layout_id<Layout>());
      } else if constexpr (std::is_same_v<T, half>) {
        auto tileptr = reinterpret_cast<int32_t *>(dst.get());
        auto srcptr = reinterpret_cast<int32_t  *>(&src.data);
        __hmma_m32n8k16_st_c_f16(tileptr, srcptr, stride,
                                 get_layout_id<Layout>());
      }
    } else if constexpr (std::is_same_v<T, double>) {
      __dmma_m8n8k4_st_c_f64(dst.get(), src.data, stride,
                             get_layout_id<Layout>());
    }
  }
};

template <typename T1, typename T2, std::size_t M, std::size_t K, std::size_t N,
          sycl::ext::oneapi::experimental::matrix::matrix_layout LayoutA,
          sycl::ext::oneapi::experimental::matrix::matrix_layout LayoutB,
          sycl::ext::oneapi::experimental::matrix::matrix_layout LayoutC,
          typename Cond = void>
struct joint_matrix_mad_impl {
  sycl::ext::oneapi::experimental::matrix::joint_matrix<
      T2, sycl::ext::oneapi::experimental::matrix::matrix_use::accumulator, M,
      N, LayoutC, sycl::sub_group>
  mad(sycl::ext::oneapi::experimental::matrix::joint_matrix<
          T1, sycl::ext::oneapi::experimental::matrix::matrix_use::a, M, K,
          LayoutA, sycl::sub_group>
          A,
      sycl::ext::oneapi::experimental::matrix::joint_matrix<
          T1, sycl::ext::oneapi::experimental::matrix::matrix_use::b, K, N,
          LayoutB, sycl::sub_group>
          B,
      sycl::ext::oneapi::experimental::matrix::joint_matrix<
          T2, sycl::ext::oneapi::experimental::matrix::matrix_use::accumulator,
          M, N, LayoutC, sycl::sub_group>
          C);
};

template <sycl::ext::oneapi::experimental::matrix::matrix_layout LayoutA,
          sycl::ext::oneapi::experimental::matrix::matrix_layout LayoutB>
constexpr int get_layout_pair_id();

template <>
constexpr int get_layout_pair_id<
    sycl::ext::oneapi::experimental::matrix::matrix_layout::row_major,
    sycl::ext::oneapi::experimental::matrix::matrix_layout::row_major>() {
  return 0;
}

template <>
constexpr int get_layout_pair_id<
    sycl::ext::oneapi::experimental::matrix::matrix_layout::row_major,
    sycl::ext::oneapi::experimental::matrix::matrix_layout::col_major>() {
  return 1;
}

template <>
constexpr int get_layout_pair_id<
    sycl::ext::oneapi::experimental::matrix::matrix_layout::col_major,
    sycl::ext::oneapi::experimental::matrix::matrix_layout::row_major>() {
  return 2;
}

template <>
constexpr int get_layout_pair_id<
    sycl::ext::oneapi::experimental::matrix::matrix_layout::col_major,
    sycl::ext::oneapi::experimental::matrix::matrix_layout::col_major>() {
  return 3;
}

template <typename T1, typename T2, std::size_t M, std::size_t K, std::size_t N,
          sycl::ext::oneapi::experimental::matrix::matrix_layout LayoutA,
          sycl::ext::oneapi::experimental::matrix::matrix_layout LayoutB,
          sycl::ext::oneapi::experimental::matrix::matrix_layout LayoutC>
struct joint_matrix_mad_impl<
    T1, T2, M, K, N, LayoutA, LayoutB, LayoutC,
    typename std::enable_if_t<
        (LayoutA == sycl::ext::oneapi::experimental::matrix::matrix_layout::
                        row_major ||
         LayoutA == sycl::ext::oneapi::experimental::matrix::matrix_layout::
                        col_major) &&
        (LayoutB == sycl::ext::oneapi::experimental::matrix::matrix_layout::
                        row_major ||
         LayoutB == sycl::ext::oneapi::experimental::matrix::matrix_layout::
                        col_major) &&
        (LayoutC == sycl::ext::oneapi::experimental::matrix::matrix_layout::
                        row_major ||
         LayoutC == sycl::ext::oneapi::experimental::matrix::matrix_layout::
                        col_major)>> {
  sycl::ext::oneapi::experimental::matrix::joint_matrix<
      T2, sycl::ext::oneapi::experimental::matrix::matrix_use::accumulator, M,
      N, LayoutC, sycl::sub_group>
  mad(sycl::ext::oneapi::experimental::matrix::joint_matrix<
          T1, sycl::ext::oneapi::experimental::matrix::matrix_use::a, M, K,
          LayoutA, sycl::sub_group>
          A,
      sycl::ext::oneapi::experimental::matrix::joint_matrix<
          T1, sycl::ext::oneapi::experimental::matrix::matrix_use::b, K, N,
          LayoutB, sycl::sub_group>
          B,
      sycl::ext::oneapi::experimental::matrix::joint_matrix<
          T2, sycl::ext::oneapi::experimental::matrix::matrix_use::accumulator,
          M, N, LayoutC, sycl::sub_group>
          C) {
    sycl::ext::oneapi::experimental::matrix::joint_matrix<
        T2, sycl::ext::oneapi::experimental::matrix::matrix_use::accumulator, M,
        N, LayoutC, sycl::sub_group>
        D;
    if constexpr (M == 16 && N == 16 && K == 16) {
      if constexpr (std::is_same_v<T1, int8_t>) {
        __imma_m16n16k16_mma_s8(D.data, A.data, B.data, C.data,
                                get_layout_pair_id<LayoutA, LayoutB>(), 0);
      } else if constexpr (std::is_same_v<T1, uint8_t>) {
        __imma_m16n16k16_mma_u8(D.data, A.data, B.data, C.data,
                                get_layout_pair_id<LayoutA, LayoutB>(), 0);
      } else if constexpr (std::is_same_v<T1, half>) {
        auto ptrA = reinterpret_cast<int32_t const *>(&A.data);
        auto ptrB = reinterpret_cast<int32_t const *>(&B.data);
        if constexpr (std::is_same<T2, float>::value) {
          __hmma_m16n16k16_mma_f32f32(D.data, ptrA, ptrB, C.data,
                                      get_layout_pair_id<LayoutA, LayoutB>(),
                                      0);
        } else if constexpr (std::is_same_v<T2, half>) {
        auto ptrC = reinterpret_cast<int32_t const *>(&C.data);
        auto ptrD = reinterpret_cast<int32_t *>(&D.data);
          __hmma_m16n16k16_mma_f16f16(ptrD, ptrA, ptrB, ptrC,
                                      get_layout_pair_id<LayoutA, LayoutB>(),
                                      0);
        }
      } else if constexpr (std::is_same_v<T1, uint16_t> ||
                           std::is_same_v<T1, sycl::ext::oneapi::experimental::
                                                bfloat16>) {
        auto ptrA = reinterpret_cast<int32_t const *>(&A.data);
        auto ptrB = reinterpret_cast<int32_t const *>(&B.data);
        __mma_bf16_m16n16k16_mma_f32(D.data, ptrA, ptrB, C.data,
                                     get_layout_pair_id<LayoutA, LayoutB>(), 0);
      }
    } else if constexpr (M == 8 && N == 32 && K == 16) {
      if constexpr (std::is_same_v<T1, int8_t>) {
        __imma_m8n32k16_mma_s8(D.data, A.data, B.data, C.data,
                               get_layout_pair_id<LayoutA, LayoutB>(), 0);
      } else if constexpr (std::is_same_v<T1, uint8_t>) {
        __imma_m8n32k16_mma_u8(D.data, A.data, B.data, C.data,
                               get_layout_pair_id<LayoutA, LayoutB>(), 0);
      } else if constexpr (std::is_same_v<T1, half>) {
        auto ptrA = reinterpret_cast<int32_t const *>(&A.data);
        auto ptrB = reinterpret_cast<int32_t const *>(&B.data);
        if constexpr (std::is_same_v<T2, float>) {
          __hmma_m8n32k16_mma_f32f32(D.data, ptrA, ptrB, C.data,
                                     get_layout_pair_id<LayoutA, LayoutB>(), 0);
        } else if constexpr (std::is_same_v<T2, half>) {
        auto ptrC = reinterpret_cast<int32_t const *>(&C.data);
        auto ptrD = reinterpret_cast<int32_t *>(&D.data);
          __hmma_m8n32k16_mma_f16f16(ptrD, ptrA, ptrB, ptrC,
                                     get_layout_pair_id<LayoutA, LayoutB>(), 0);
        }
      } else if constexpr (std::is_same_v<T1, uint16_t> ||
                           std::is_same_v<T1, sycl::ext::oneapi::experimental::
                                                bfloat16>) {
        auto ptrA = reinterpret_cast<int32_t const *>(&A.data);
        auto ptrB = reinterpret_cast<int32_t const *>(&B.data);
        __mma_bf16_m8n32k16_mma_f32(D.data, ptrA, ptrB, C.data,
                                    get_layout_pair_id<LayoutA, LayoutB>(), 0);
      }
    } else if constexpr (M == 32 && N == 8 && K == 16) {
      if constexpr (std::is_same_v<T1, int8_t>) {
        __imma_m32n8k16_mma_s8(D.data, A.data, B.data, C.data,
                               get_layout_pair_id<LayoutA, LayoutB>(), 0);
      } else if constexpr (std::is_same_v<T1, uint8_t>) {
        __imma_m32n8k16_mma_u8(D.data, A.data, B.data, C.data,
                               get_layout_pair_id<LayoutA, LayoutB>(), 0);
      } else if constexpr (std::is_same_v<T1, uint16_t> ||
                           std::is_same_v<T1, sycl::ext::oneapi::experimental::
                                                bfloat16>) {
        auto ptrA = reinterpret_cast<int32_t const *>(&A.data);
        auto ptrB = reinterpret_cast<int32_t const *>(&B.data);
        __mma_bf16_m32n8k16_mma_f32(D.data, ptrA, ptrB, C.data,
                                    get_layout_pair_id<LayoutA, LayoutB>(), 0);
      } else if constexpr (std::is_same_v<T1, half>) {
        auto ptrA = reinterpret_cast<int32_t const *>(&A.data);
        auto ptrB = reinterpret_cast<int32_t const *>(&B.data);
        if constexpr (std::is_same_v<T2, float>) {
          __hmma_m32n8k16_mma_f32f32(D.data, ptrA, ptrB, C.data,
                                     get_layout_pair_id<LayoutA, LayoutB>(), 0);
        } else if constexpr (std::is_same_v<T2, half>) {
          auto ptrC = reinterpret_cast<int32_t const *>(&C.data);
        auto ptrD = reinterpret_cast<int32_t *>(&D.data);
          __hmma_m32n8k16_mma_f16f16(ptrD, ptrA, ptrB, ptrC,
                                     get_layout_pair_id<LayoutA, LayoutB>(), 0);
        }
      }
    } else if constexpr (std::is_same_v<T1, double>) {
      __dmma_m8n8k4_mma_f64(D.data, A.data, B.data, C.data,
                            get_layout_pair_id<LayoutA, LayoutB>(), 0);
    }
    return D;
  }
};

} // namespace detail

namespace experimental::matrix {

template <typename Group, typename T, matrix_use Use, size_t NumRows,
          size_t NumCols, matrix_layout Layout, access::address_space Space>
void joint_matrix_load(
    Group sg, joint_matrix<T, Use, NumRows, NumCols, Layout, Group> &res,
    multi_ptr<T, Space> src, size_t stride) {
#if defined(__SYCL_DEVICE_ONLY__) && defined(__NVPTX__)
  sycl::ext::oneapi::detail::joint_matrix_load_impl<T, Use, NumRows, NumCols,
                                                    Layout, Space>{}
      .load(res, src, stride);
#else
  (void)sg;
  (void)res;
  (void)src;
  (void)stride;
  throw runtime_error(
      "When using SYCL_EXT_ONEAPI_MATRIX=3 joint_matrix_load is "
      "only supported by CUDA devices",
      PI_INVALID_DEVICE);
#endif // defined(__SYCL_DEVICE_ONLY__) && defined(__NVPTX__)
}

template <typename Group, typename T, size_t NumRows, size_t NumCols,
          matrix_layout Layout, access::address_space Space>
void joint_matrix_store(Group sg,
                        joint_matrix<T, matrix_use::accumulator, NumRows,
                                     NumCols, Layout, Group> &src,
                        multi_ptr<T, Space> dst, size_t stride) {
#if defined(__SYCL_DEVICE_ONLY__) && defined(__NVPTX__)
  sycl::ext::oneapi::detail::joint_matrix_store_impl<T, NumRows, NumCols,
                                                     Layout, Space>{}
      .store(src, dst, stride);
#else
  (void)sg;
  (void)src;
  (void)dst;
  (void)stride;
  throw runtime_error(
      "When using SYCL_EXT_ONEAPI_MATRIX=3 joint_matrix_store is "
      "only supported by CUDA devices",
      PI_INVALID_DEVICE);
#endif // defined(__SYCL_DEVICE_ONLY__) && defined(__NVPTX__)
}

template <typename Group, typename T1, typename T2, std::size_t M,
          std::size_t K, std::size_t N, matrix_layout LayoutA,
          matrix_layout LayoutB, matrix_layout LayoutC>
joint_matrix<T2, matrix_use::accumulator, M, N, LayoutC, Group>
joint_matrix_mad(
    Group sg, joint_matrix<T1, matrix_use::a, M, K, LayoutA, Group> A,
    joint_matrix<T1, matrix_use::b, K, N, LayoutB, Group> B,
    joint_matrix<T2, matrix_use::accumulator, M, N, LayoutC, Group> C) {
#if defined(__SYCL_DEVICE_ONLY__) && defined(__NVPTX__)
  return sycl::ext::oneapi::detail::joint_matrix_mad_impl<
             T1, T2, M, K, N, LayoutA, LayoutB, LayoutC>{}
      .mad(A, B, C);
#else
  (void)sg;
  (void)A;
  (void)B;
  (void)C;
  throw runtime_error("When using SYCL_EXT_ONEAPI_MATRIX=3 joint_matrix_mad is "
                      "only supported by CUDA devices",
                      PI_INVALID_DEVICE);
#endif // defined(__SYCL_DEVICE_ONLY__) && defined(__NVPTX__)
}

} // namespace experimental::matrix
} // namespace oneapi
} // namespace ext
} // namespace sycl
} // __SYCL_INLINE_NAMESPACE(cl)
