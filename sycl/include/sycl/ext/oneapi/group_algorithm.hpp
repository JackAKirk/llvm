//==----------- group_algorithm.hpp --- SYCL group algorithm----------------==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once
#include <CL/__spirv/spirv_ops.hpp>
#include <CL/__spirv/spirv_types.hpp>
#include <CL/__spirv/spirv_vars.hpp>
#include <CL/sycl/detail/spirv.hpp>
#include <CL/sycl/detail/type_traits.hpp>
#include <CL/sycl/group.hpp>
#include <CL/sycl/group_algorithm.hpp>
#include <CL/sycl/nd_item.hpp>
#include <sycl/ext/oneapi/atomic.hpp>
#include <sycl/ext/oneapi/functional.hpp>
#include <sycl/ext/oneapi/sub_group.hpp>
#include <sycl/ext/oneapi/sub_group_mask.hpp>

__SYCL_INLINE_NAMESPACE(cl) {
namespace sycl {
namespace ext {
namespace oneapi {

// EnableIf shorthands for algorithms that depend only on type
template <typename T>
using EnableIfIsScalarArithmetic = cl::sycl::detail::enable_if_t<
    cl::sycl::detail::is_scalar_arithmetic<T>::value, T>;

template <typename T>
using EnableIfIsVectorArithmetic = cl::sycl::detail::enable_if_t<
    cl::sycl::detail::is_vector_arithmetic<T>::value, T>;

template <typename Ptr, typename T>
using EnableIfIsPointer =
    cl::sycl::detail::enable_if_t<cl::sycl::detail::is_pointer<Ptr>::value, T>;

template <typename T>
using EnableIfIsTriviallyCopyable = cl::sycl::detail::enable_if_t<
    std::is_trivially_copyable<T>::value &&
        !cl::sycl::detail::is_vector_arithmetic<T>::value,
    T>;

// EnableIf shorthands for algorithms that depend on type and an operator
template <typename T, typename BinaryOperation>
using EnableIfIsScalarArithmeticNativeOp = cl::sycl::detail::enable_if_t<
    cl::sycl::detail::is_scalar_arithmetic<T>::value &&
        cl::sycl::detail::is_native_op<T, BinaryOperation>::value,
    T>;

template <typename T, typename BinaryOperation>
using EnableIfIsVectorArithmeticNativeOp = cl::sycl::detail::enable_if_t<
    cl::sycl::detail::is_vector_arithmetic<T>::value &&
        cl::sycl::detail::is_native_op<T, BinaryOperation>::value,
    T>;

// TODO: Lift TriviallyCopyable restriction eventually
template <typename T, typename BinaryOperation>
using EnableIfIsNonNativeOp = cl::sycl::detail::enable_if_t<
    (!cl::sycl::detail::is_scalar_arithmetic<T>::value &&
     !cl::sycl::detail::is_vector_arithmetic<T>::value &&
     std::is_trivially_copyable<T>::value) ||
        !cl::sycl::detail::is_native_op<T, BinaryOperation>::value,
    T>;

namespace detail {
template <typename Group> constexpr auto group_to_scope() {
  if constexpr (std::is_same<Group, sycl::ext::oneapi::sub_group>::value) {
    return __spv::Scope::Subgroup;
  } else {
    return __spv::Scope::Workgroup;
  }
}
} // namespace detail

/// Asynchronously copies a number of elements specified by \p numElements
/// from the source pointed by \p src to destination pointed by \p dest
/// with a source stride specified by \p srcStride, and returns a SYCL
/// device_event which can be used to wait on the completion of the copy.
/// Permitted types for dataT are all scalar and vector types, except boolean.
template <typename Group, typename dataT>
detail::enable_if_t<is_group_v<Group> && !detail::is_bool<dataT>::value,
                    device_event>
async_group_copy(Group, global_ptr<dataT> src, local_ptr<dataT> dest,
                 size_t numElements, size_t srcStride) {
  using DestT = detail::ConvertToOpenCLType_t<decltype(dest)>;
  using SrcT = detail::ConvertToOpenCLType_t<decltype(src)>;

  __ocl_event_t E = __SYCL_OpGroupAsyncCopyGlobalToLocal(
      detail::group_to_scope<Group>(), DestT(dest.get()), SrcT(src.get()),
      numElements, srcStride, 0);
  return device_event(&E);
}

/// Asynchronously copies a number of elements specified by \p numElements
/// from the source pointed by \p src to destination pointed by \p dest with
/// the destination stride specified by \p destStride, and returns a SYCL
/// device_event which can be used to wait on the completion of the copy.
/// Permitted types for dataT are all scalar and vector types, except boolean.
template <typename Group, typename dataT>
detail::enable_if_t<is_group_v<Group> && !detail::is_bool<dataT>::value,
                    device_event>
async_group_copy(Group, local_ptr<dataT> src, global_ptr<dataT> dest,
                 size_t numElements, size_t destStride) {
  using DestT = detail::ConvertToOpenCLType_t<decltype(dest)>;
  using SrcT = detail::ConvertToOpenCLType_t<decltype(src)>;

  __ocl_event_t E = __SYCL_OpGroupAsyncCopyLocalToGlobal(
      detail::group_to_scope<Group>(), DestT(dest.get()), SrcT(src.get()),
      numElements, destStride, 0);
  return device_event(&E);
}

/// Specialization for scalar bool type.
/// Asynchronously copies a number of elements specified by \p NumElements
/// from the source pointed by \p Src to destination pointed by \p Dest
/// with a stride specified by \p Stride, and returns a SYCL device_event
/// which can be used to wait on the completion of the copy.
template <typename Group, typename T, access::address_space DestS,
          access::address_space SrcS>
detail::enable_if_t<is_group_v<Group> && detail::is_scalar_bool<T>::value,
                    device_event>
async_group_copy(Group g, multi_ptr<T, SrcS> Src, multi_ptr<T, DestS> Dest,
                 size_t NumElements, size_t Stride) {
  static_assert(sizeof(bool) == sizeof(uint8_t),
                "Async copy to/from bool memory is not supported.");
  auto DestP =
      multi_ptr<uint8_t, DestS>(reinterpret_cast<uint8_t *>(Dest.get()));
  auto SrcP = multi_ptr<uint8_t, SrcS>(reinterpret_cast<uint8_t *>(Src.get()));
  return async_group_copy(g, SrcP, DestP, NumElements, Stride);
}

/// Specialization for vector bool type.
/// Asynchronously copies a number of elements specified by \p NumElements
/// from the source pointed by \p Src to destination pointed by \p Dest
/// with a stride specified by \p Stride, and returns a SYCL device_event
/// which can be used to wait on the completion of the copy.
template <typename Group, typename T, access::address_space DestS,
          access::address_space SrcS>
detail::enable_if_t<is_group_v<Group> && detail::is_vector_bool<T>::value,
                    device_event>
async_group_copy(Group g, multi_ptr<T, SrcS> Src, multi_ptr<T, DestS> Dest,
                 size_t NumElements, size_t Stride) {
  static_assert(sizeof(bool) == sizeof(uint8_t),
                "Async copy to/from bool memory is not supported.");
  using VecT = detail::change_base_type_t<T, uint8_t>;
  auto DestP = multi_ptr<VecT, DestS>(reinterpret_cast<VecT *>(Dest.get()));
  auto SrcP = multi_ptr<VecT, SrcS>(reinterpret_cast<VecT *>(Src.get()));
  return async_group_copy(g, SrcP, DestP, NumElements, Stride);
}

/// Asynchronously copies a number of elements specified by \p numElements
/// from the source pointed by \p src to destination pointed by \p dest and
/// returns a SYCL device_event which can be used to wait on the completion
/// of the copy.
/// Permitted types for dataT are all scalar and vector types.
template <typename Group, typename dataT>
detail::enable_if_t<is_group_v<Group>, device_event>
async_group_copy(Group g, global_ptr<dataT> src, local_ptr<dataT> dest,
                 size_t numElements) {
  return async_group_copy(g, src, dest, numElements, 1);
}

/// Asynchronously copies a number of elements specified by \p numElements
/// from the source pointed by \p src to destination pointed by \p dest and
/// returns a SYCL device_event which can be used to wait on the completion
/// of the copy.
/// Permitted types for dataT are all scalar and vector types.
template <typename Group, typename dataT>
device_event async_group_copy(Group g, local_ptr<dataT> src,
                              global_ptr<dataT> dest, size_t numElements) {
  return async_group_copy(g, src, dest, numElements, 1);
}

template <typename Group, typename... eventTN>
void wait_for(Group g, eventTN... Events) {
  // Events.wait() calls __spirv_GroupWaitEvents
  // __spirv_GroupWaitEvents ignores event_list and calls __spirv_ControlBarrier
  // https://github.com/intel/llvm/blob/sycl/libclc/generic/libspirv/async/wait_group_events.cl
  // __spirv_ControlBarrier calls __syncthreads or __nvvm_bar_warp_sync
  // https://github.com/intel/llvm/blob/sycl/libclc/ptx-nvidiacl/libspirv/synchronization/barrier.cl
  (Events.ext_oneapi_wait(g), ...);
}
/// Asynchronously copies a number of elements specified by \p numElements
/// from the source pointed by \p src to destination pointed by \p dest
/// with a source stride specified by \p srcStride, and returns a SYCL
/// device_event which can be used to wait on the completion of the copy.
/// Permitted types for dataT are all scalar and vector types, except boolean.
template <typename dataT>
detail::enable_if_t<!detail::is_bool<dataT>::value, device_event>
async_group_copy(sub_group, sub_group_mask mask,
                              global_ptr<dataT> src, local_ptr<dataT> dest,
                              size_t numElements, size_t srcStride) {
  using DestT = detail::ConvertToOpenCLType_t<decltype(dest)>;
  using SrcT = detail::ConvertToOpenCLType_t<decltype(src)>;

  uint32_t mask_bits;
  mask.extract_bits(mask_bits);

  __ocl_event_t E = __SYCL_OpGroupAsyncCopyGlobalToLocalMasked(
      __spv::Scope::Subgroup, DestT(dest.get()), SrcT(src.get()), numElements,
      srcStride, 0, mask_bits);
  return device_event(&E);
}

/// Asynchronously copies a number of elements specified by \p numElements
/// from the source pointed by \p src to destination pointed by \p dest with
/// the destination stride specified by \p destStride, and returns a SYCL
/// device_event which can be used to wait on the completion of the copy.
/// Permitted types for dataT are all scalar and vector types, except boolean.
template <typename dataT>
detail::enable_if_t<!detail::is_bool<dataT>::value, device_event>
async_group_copy(sub_group, sub_group_mask mask, local_ptr<dataT> src,
                 global_ptr<dataT> dest, size_t numElements,
                 size_t destStride) {
  using DestT = detail::ConvertToOpenCLType_t<decltype(dest)>;
  using SrcT = detail::ConvertToOpenCLType_t<decltype(src)>;

  uint32_t mask_bits;
  mask.extract_bits(mask_bits);

  __ocl_event_t E = __SYCL_OpGroupAsyncCopyLocalToGlobalMasked(
      __spv::Scope::Subgroup, DestT(dest.get()), SrcT(src.get()), numElements,
      destStride, 0, mask_bits);
  return device_event(&E);
}

/// Specialization for scalar bool type.
/// Asynchronously copies a number of elements specified by \p NumElements
/// from the source pointed by \p Src to destination pointed by \p Dest
/// with a stride specified by \p Stride, and returns a SYCL device_event
/// which can be used to wait on the completion of the copy.
template <typename T, access::address_space DestS, access::address_space SrcS>
detail::enable_if_t<detail::is_scalar_bool<T>::value, device_event>
async_group_copy(sub_group g, sub_group_mask mask, multi_ptr<T, SrcS> Src,
                 multi_ptr<T, DestS> Dest, size_t NumElements, size_t Stride) {
  static_assert(sizeof(bool) == sizeof(uint8_t),
                "Async copy to/from bool memory is not supported.");
  auto DestP =
      multi_ptr<uint8_t, DestS>(reinterpret_cast<uint8_t *>(Dest.get()));
  auto SrcP = multi_ptr<uint8_t, SrcS>(reinterpret_cast<uint8_t *>(Src.get()));
  return async_group_copy(g, mask, SrcP, DestP, NumElements, Stride);
}

/// Specialization for vector bool type.
/// Asynchronously copies a number of elements specified by \p NumElements
/// from the source pointed by \p Src to destination pointed by \p Dest
/// with a stride specified by \p Stride, and returns a SYCL device_event
/// which can be used to wait on the completion of the copy.
template <typename T, access::address_space DestS, access::address_space SrcS>
detail::enable_if_t<detail::is_vector_bool<T>::value, device_event>
async_group_copy(sub_group g, sub_group_mask mask, multi_ptr<T, SrcS> Src,
                 multi_ptr<T, DestS> Dest, size_t NumElements, size_t Stride) {
  static_assert(sizeof(bool) == sizeof(uint8_t),
                "Async copy to/from bool memory is not supported.");
  using VecT = detail::change_base_type_t<T, uint8_t>;
  auto DestP = multi_ptr<VecT, DestS>(reinterpret_cast<VecT *>(Dest.get()));
  auto SrcP = multi_ptr<VecT, SrcS>(reinterpret_cast<VecT *>(Src.get()));
  return async_group_copy(g, mask, SrcP, DestP, NumElements, Stride);
}

/// Asynchronously copies a number of elements specified by \p numElements
/// from the source pointed by \p src to destination pointed by \p dest and
/// returns a SYCL device_event which can be used to wait on the completion
/// of the copy.
/// Permitted types for dataT are all scalar and vector types.
template <typename dataT>
device_event async_group_copy(sub_group g, sub_group_mask mask,
                              global_ptr<dataT> src, local_ptr<dataT> dest,
                              size_t numElements) {
  return async_group_copy(g, mask, src, dest, numElements, 1);
}

/// Asynchronously copies a number of elements specified by \p numElements
/// from the source pointed by \p src to destination pointed by \p dest and
/// returns a SYCL device_event which can be used to wait on the completion
/// of the copy.
/// Permitted types for dataT are all scalar and vector types.
template <typename dataT>
device_event async_group_copy(sub_group g, sub_group_mask mask,
                              local_ptr<dataT> src, global_ptr<dataT> dest,
                              size_t numElements) {
  return async_group_copy(g, mask, src, dest, numElements, 1);
}

template <typename Group, typename... eventTN>
void wait_for(Group g, sub_group_mask mask, eventTN... Events) {
  (Events.ext_oneapi_wait(g, mask), ...);
}

template <typename Group>
__SYCL2020_DEPRECATED(
    "ext::oneapi::all_of is deprecated. Use all_of_group instead.")
detail::enable_if_t<detail::is_generic_group<Group>::value, bool> all_of(
    Group g, bool pred) {
  return all_of_group(g, pred);
}

template <typename Group, typename T, class Predicate>
__SYCL2020_DEPRECATED(
    "ext::oneapi::all_of is deprecated. Use all_of_group instead.")
detail::enable_if_t<detail::is_generic_group<Group>::value, bool> all_of(
    Group g, T x, Predicate pred) {
  return all_of_group(g, pred(x));
}

template <typename Group, typename Ptr, class Predicate>
__SYCL2020_DEPRECATED(
    "ext::oneapi::all_of is deprecated. Use joint_all_of instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_pointer<Ptr>::value),
                    bool> all_of(Group g, Ptr first, Ptr last, Predicate pred) {
  return joint_all_of(g, first, last, pred);
}

template <typename Group>
__SYCL2020_DEPRECATED(
    "ext::oneapi::any_of is deprecated. Use any_of_group instead.")
detail::enable_if_t<detail::is_generic_group<Group>::value, bool> any_of(
    Group g, bool pred) {
  return any_of_group(g, pred);
}

template <typename Group, typename T, class Predicate>
__SYCL2020_DEPRECATED(
    "ext::oneapi::any_of is deprecated. Use any_of_group instead.")
detail::enable_if_t<detail::is_generic_group<Group>::value, bool> any_of(
    Group g, T x, Predicate pred) {
  return any_of_group(g, pred(x));
}

template <typename Group, typename Ptr, class Predicate>
__SYCL2020_DEPRECATED(
    "ext::oneapi::any_of is deprecated. Use joint_any_of instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_pointer<Ptr>::value),
                    bool> any_of(Group g, Ptr first, Ptr last, Predicate pred) {
  return joint_any_of(g, first, last, pred);
}

template <typename Group>
__SYCL2020_DEPRECATED(
    "ext::oneapi::none_of is deprecated. Use none_of_group instead.")
detail::enable_if_t<detail::is_generic_group<Group>::value, bool> none_of(
    Group g, bool pred) {
  return none_of_group(g, pred);
}

template <typename Group, typename T, class Predicate>
__SYCL2020_DEPRECATED(
    "ext::oneapi::none_of is deprecated. Use none_of_group instead.")
detail::enable_if_t<detail::is_generic_group<Group>::value, bool> none_of(
    Group g, T x, Predicate pred) {
  return none_of_group(g, pred(x));
}

template <typename Group, typename Ptr, class Predicate>
__SYCL2020_DEPRECATED(
    "ext::oneapi::none_of is deprecated. Use joint_none_of instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_pointer<Ptr>::value),
                    bool> none_of(Group g, Ptr first, Ptr last,
                                  Predicate pred) {
  return joint_none_of(g, first, last, pred);
}

template <typename Group, typename T>
__SYCL2020_DEPRECATED(
    "ext::oneapi::broadcast is deprecated. Use group_broadcast instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     std::is_trivially_copyable<T>::value &&
                     !detail::is_vector_arithmetic<T>::value),
                    T> broadcast(Group, T x, typename Group::id_type local_id) {
#ifdef __SYCL_DEVICE_ONLY__
  return sycl::detail::spirv::GroupBroadcast<Group>(x, local_id);
#else
  (void)x;
  (void)local_id;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
__SYCL2020_DEPRECATED(
    "ext::oneapi::broadcast is deprecated. Use group_broadcast instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_vector_arithmetic<T>::value),
                    T> broadcast(Group g, T x,
                                 typename Group::id_type local_id) {
#ifdef __SYCL_DEVICE_ONLY__
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = broadcast(g, x[s], local_id);
  }
  return result;
#else
  (void)g;
  (void)x;
  (void)local_id;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
__SYCL2020_DEPRECATED(
    "ext::oneapi::broadcast is deprecated. Use group_broadcast instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     std::is_trivially_copyable<T>::value &&
                     !detail::is_vector_arithmetic<T>::value),
                    T> broadcast(Group g, T x,
                                 typename Group::linear_id_type
                                     linear_local_id) {
#ifdef __SYCL_DEVICE_ONLY__
  return broadcast(
      g, x,
      sycl::detail::linear_id_to_id(g.get_local_range(), linear_local_id));
#else
  (void)g;
  (void)x;
  (void)linear_local_id;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
__SYCL2020_DEPRECATED(
    "ext::oneapi::broadcast is deprecated. Use group_broadcast instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_vector_arithmetic<T>::value),
                    T> broadcast(Group g, T x,
                                 typename Group::linear_id_type
                                     linear_local_id) {
#ifdef __SYCL_DEVICE_ONLY__
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = broadcast(g, x[s], linear_local_id);
  }
  return result;
#else
  (void)g;
  (void)x;
  (void)linear_local_id;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
__SYCL2020_DEPRECATED(
    "ext::oneapi::broadcast is deprecated. Use group_broadcast instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     std::is_trivially_copyable<T>::value &&
                     !detail::is_vector_arithmetic<T>::value),
                    T> broadcast(Group g, T x) {
#ifdef __SYCL_DEVICE_ONLY__
  return broadcast(g, x, 0);
#else
  (void)g;
  (void)x;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T>
__SYCL2020_DEPRECATED(
    "ext::oneapi::broadcast is deprecated. Use group_broadcast instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_vector_arithmetic<T>::value),
                    T> broadcast(Group g, T x) {
#ifdef __SYCL_DEVICE_ONLY__
  T result;
  for (int s = 0; s < x.get_size(); ++s) {
    result[s] = broadcast(g, x[s]);
  }
  return result;
#else
  (void)g;
  (void)x;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Group, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED(
    "ext::oneapi::reduce is deprecated. Use reduce_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_scalar_arithmetic<T>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> reduce(Group g, T x, BinaryOperation binary_op) {
  return reduce_over_group(g, x, binary_op);
}

template <typename Group, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED(
    "ext::oneapi::reduce is deprecated. Use reduce_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_vector_arithmetic<T>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> reduce(Group g, T x, BinaryOperation binary_op) {
  return reduce_over_group(g, x, binary_op);
}

template <typename Group, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED(
    "ext::oneapi::reduce is deprecated. Use reduce_over_group instead.")
detail::enable_if_t<(detail::is_sub_group<Group>::value &&
                     std::is_trivially_copyable<T>::value &&
                     (!detail::is_arithmetic<T>::value ||
                      !detail::is_native_op<T, BinaryOperation>::value)),
                    T> reduce(Group g, T x, BinaryOperation op) {
  T result = x;
  for (int mask = 1; mask < g.get_max_local_range()[0]; mask *= 2) {
    T tmp = g.shuffle_xor(result, id<1>(mask));
    if ((g.get_local_id()[0] ^ mask) < g.get_local_range()[0]) {
      result = op(result, tmp);
    }
  }
  return g.shuffle(result, 0);
}

template <typename Group, typename V, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED(
    "ext::oneapi::reduce is deprecated. Use reduce_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_scalar_arithmetic<V>::value &&
                     detail::is_scalar_arithmetic<T>::value &&
                     detail::is_native_op<V, BinaryOperation>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> reduce(Group g, V x, T init, BinaryOperation binary_op) {
  return reduce_over_group(g, x, init, binary_op);
}

template <typename Group, typename V, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED(
    "ext::oneapi::reduce is deprecated. Use reduce_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_vector_arithmetic<V>::value &&
                     detail::is_vector_arithmetic<T>::value &&
                     detail::is_native_op<V, BinaryOperation>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> reduce(Group g, V x, T init, BinaryOperation binary_op) {
  return reduce_over_group(g, x, init, binary_op);
}

template <typename Group, typename V, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED(
    "ext::oneapi::reduce is deprecated. Use reduce_over_group instead.")
detail::enable_if_t<(detail::is_sub_group<Group>::value &&
                     std::is_trivially_copyable<T>::value &&
                     std::is_trivially_copyable<V>::value &&
                     (!detail::is_arithmetic<T>::value ||
                      !detail::is_arithmetic<V>::value ||
                      !detail::is_native_op<T, BinaryOperation>::value)),
                    T> reduce(Group g, V x, T init, BinaryOperation op) {
  T result = x;
  for (int mask = 1; mask < g.get_max_local_range()[0]; mask *= 2) {
    T tmp = g.shuffle_xor(result, id<1>(mask));
    if ((g.get_local_id()[0] ^ mask) < g.get_local_range()[0]) {
      result = op(result, tmp);
    }
  }
  return g.shuffle(op(init, result), 0);
}

template <typename Group, typename Ptr, class BinaryOperation>
__SYCL2020_DEPRECATED(
    "ext::oneapi::reduce is deprecated. Use joint_reduce instead.")
detail::enable_if_t<
    (detail::is_generic_group<Group>::value && detail::is_pointer<Ptr>::value &&
     detail::is_arithmetic<typename detail::remove_pointer<Ptr>::type>::value),
    typename detail::remove_pointer<Ptr>::type> reduce(Group g, Ptr first,
                                                       Ptr last,
                                                       BinaryOperation
                                                           binary_op) {
  return joint_reduce(g, first, last, binary_op);
}

template <typename Group, typename Ptr, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED(
    "ext::oneapi::reduce is deprecated. Use joint_reduce instead.")
detail::enable_if_t<
    (detail::is_generic_group<Group>::value && detail::is_pointer<Ptr>::value &&
     detail::is_arithmetic<typename detail::remove_pointer<Ptr>::type>::value &&
     detail::is_arithmetic<T>::value &&
     detail::is_native_op<typename detail::remove_pointer<Ptr>::type,
                          BinaryOperation>::value &&
     detail::is_native_op<T, BinaryOperation>::value),
    T> reduce(Group g, Ptr first, Ptr last, T init, BinaryOperation binary_op) {
  return joint_reduce(g, first, last, init, binary_op);
}

template <typename Group, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED("ext::oneapi::exclusive_scan is deprecated. Use "
                      "exclusive_scan_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_scalar_arithmetic<T>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> exclusive_scan(Group g, T x, BinaryOperation binary_op) {
  return exclusive_scan_over_group(g, x, binary_op);
}

template <typename Group, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED("ext::oneapi::exclusive_scan is deprecated. Use "
                      "exclusive_scan_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_vector_arithmetic<T>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> exclusive_scan(Group g, T x, BinaryOperation binary_op) {
  return exclusive_scan_over_group(g, x, binary_op);
}

template <typename Group, typename V, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED("ext::oneapi::exclusive_scan is deprecated. Use "
                      "exclusive_scan_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_vector_arithmetic<V>::value &&
                     detail::is_vector_arithmetic<T>::value &&
                     detail::is_native_op<V, BinaryOperation>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> exclusive_scan(Group g, V x, T init,
                                      BinaryOperation binary_op) {
  return exclusive_scan_over_group(g, x, init, binary_op);
}

template <typename Group, typename V, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED("ext::oneapi::exclusive_scan is deprecated. Use "
                      "exclusive_scan_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_scalar_arithmetic<V>::value &&
                     detail::is_scalar_arithmetic<T>::value &&
                     detail::is_native_op<V, BinaryOperation>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> exclusive_scan(Group g, V x, T init,
                                      BinaryOperation binary_op) {
  return exclusive_scan_over_group(g, x, init, binary_op);
}

template <typename Group, typename InPtr, typename OutPtr, typename T,
          class BinaryOperation>
__SYCL2020_DEPRECATED("ext::oneapi::exclusive_scan is deprecated. Use "
                      "joint_exclusive_scan instead.")
detail::enable_if_t<
    (detail::is_generic_group<Group>::value &&
     detail::is_pointer<InPtr>::value && detail::is_pointer<OutPtr>::value &&
     detail::is_arithmetic<
         typename detail::remove_pointer<InPtr>::type>::value &&
     detail::is_arithmetic<T>::value &&
     detail::is_native_op<typename detail::remove_pointer<InPtr>::type,
                          BinaryOperation>::value &&
     detail::is_native_op<T, BinaryOperation>::value),
    OutPtr> exclusive_scan(Group g, InPtr first, InPtr last, OutPtr result,
                           T init, BinaryOperation binary_op) {
  return joint_exclusive_scan(g, first, last, result, init, binary_op);
}

template <typename Group, typename InPtr, typename OutPtr,
          class BinaryOperation>
__SYCL2020_DEPRECATED("ext::oneapi::exclusive_scan is deprecated. Use "
                      "joint_exclusive_scan instead.")
detail::enable_if_t<
    (detail::is_generic_group<Group>::value &&
     detail::is_pointer<InPtr>::value && detail::is_pointer<OutPtr>::value &&
     detail::is_arithmetic<
         typename detail::remove_pointer<InPtr>::type>::value &&
     detail::is_native_op<typename detail::remove_pointer<InPtr>::type,
                          BinaryOperation>::value),
    OutPtr> exclusive_scan(Group g, InPtr first, InPtr last, OutPtr result,
                           BinaryOperation binary_op) {
  return joint_exclusive_scan(g, first, last, result, binary_op);
}

template <typename Group, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED("ext::oneapi::inclusive_scan is deprecated. Use "
                      "inclusive_scan_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_vector_arithmetic<T>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> inclusive_scan(Group g, T x, BinaryOperation binary_op) {
  return inclusive_scan_over_group(g, x, binary_op);
}

template <typename Group, typename T, class BinaryOperation>
__SYCL2020_DEPRECATED("ext::oneapi::inclusive_scan is deprecated. Use "
                      "inclusive_scan_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_scalar_arithmetic<T>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> inclusive_scan(Group g, T x, BinaryOperation binary_op) {
  return inclusive_scan_over_group(g, x, binary_op);
}

template <typename Group, typename V, class BinaryOperation, typename T>
__SYCL2020_DEPRECATED("ext::oneapi::inclusive_scan is deprecated. Use "
                      "inclusive_scan_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_scalar_arithmetic<V>::value &&
                     detail::is_scalar_arithmetic<T>::value &&
                     detail::is_native_op<V, BinaryOperation>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> inclusive_scan(Group g, V x, BinaryOperation binary_op,
                                      T init) {
  return inclusive_scan_over_group(g, x, binary_op, init);
}

template <typename Group, typename V, class BinaryOperation, typename T>
__SYCL2020_DEPRECATED("ext::oneapi::inclusive_scan is deprecated. Use "
                      "inclusive_scan_over_group instead.")
detail::enable_if_t<(detail::is_generic_group<Group>::value &&
                     detail::is_vector_arithmetic<V>::value &&
                     detail::is_vector_arithmetic<T>::value &&
                     detail::is_native_op<V, BinaryOperation>::value &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T> inclusive_scan(Group g, V x, BinaryOperation binary_op,
                                      T init) {
  return inclusive_scan_over_group(g, x, binary_op, init);
}

template <typename Group, typename InPtr, typename OutPtr,
          class BinaryOperation, typename T>
__SYCL2020_DEPRECATED("ext::oneapi::inclusive_scan is deprecated. Use "
                      "joint_inclusive_scan instead.")
detail::enable_if_t<
    (detail::is_generic_group<Group>::value &&
     detail::is_pointer<InPtr>::value && detail::is_pointer<OutPtr>::value &&
     detail::is_arithmetic<
         typename detail::remove_pointer<InPtr>::type>::value &&
     detail::is_arithmetic<T>::value &&
     detail::is_native_op<typename detail::remove_pointer<InPtr>::type,
                          BinaryOperation>::value &&
     detail::is_native_op<T, BinaryOperation>::value),
    OutPtr> inclusive_scan(Group g, InPtr first, InPtr last, OutPtr result,
                           BinaryOperation binary_op, T init) {
  return joint_inclusive_scan(g, first, last, result, binary_op, init);
}

template <typename Group, typename InPtr, typename OutPtr,
          class BinaryOperation>
__SYCL2020_DEPRECATED("ext::oneapi::inclusive_scan is deprecated. Use "
                      "joint_inclusive_scan instead.")
detail::enable_if_t<
    (detail::is_generic_group<Group>::value &&
     detail::is_pointer<InPtr>::value && detail::is_pointer<OutPtr>::value &&
     detail::is_arithmetic<
         typename detail::remove_pointer<InPtr>::type>::value &&
     detail::is_native_op<typename detail::remove_pointer<InPtr>::type,
                          BinaryOperation>::value),
    OutPtr> inclusive_scan(Group g, InPtr first, InPtr last, OutPtr result,
                           BinaryOperation binary_op) {
  return joint_inclusive_scan(g, first, last, result, binary_op);
}

template <typename Group>
detail::enable_if_t<detail::is_generic_group<Group>::value, bool>
leader(Group g) {
#ifdef __SYCL_DEVICE_ONLY__
  typename Group::linear_id_type linear_id =
      sycl::detail::get_local_linear_id(g);
  return (linear_id == 0);
#else
  (void)g;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

inline void
group_barrier(sycl::sub_group, sub_group_mask mask,
              memory_scope FenceScope = sycl::sub_group::fence_scope) {
#ifdef __SYCL_DEVICE_ONLY__
  // Per SYCL spec, group_barrier must perform both control barrier and memory
  // fence operations. All work-items execute a release fence prior to
  // barrier and acquire fence afterwards. The rest of semantics flags specify
  // which type of memory this behavior is applied to.
  uint32_t mask_bits;
  mask.extract_bits(mask_bits);
  __spirv_ControlBarrierMasked(
      __spv::Scope::Subgroup, sycl::detail::spirv::getScope(FenceScope),
      __spv::MemorySemanticsMask::SequentiallyConsistent |
          __spv::MemorySemanticsMask::SubgroupMemory |
          __spv::MemorySemanticsMask::WorkgroupMemory |
          __spv::MemorySemanticsMask::CrossWorkgroupMemory,
      mask_bits);
#else
  (void)FenceScope;
  throw sycl::runtime_error("Barriers are not supported on host device",
                            PI_INVALID_DEVICE);
#endif
}

// ---- reduce_over_group
template <typename T, class BinaryOperation>
detail::enable_if_t<(std::is_scalar<T>::value &&
(detail::IsPlus<V, BinaryOperation>::value ||
                   detail::IsMinimum<V, BinaryOperation>::value ||
                   detail::IsMaximum<V, BinaryOperation>::value)) &&
                     detail::is_native_op<T, BinaryOperation>::value),
                    T>
reduce_over_group(sub_group, sub_group_mask mask, T x,
                  BinaryOperation binary_op) {
  static_assert(
      std::is_same<decltype(binary_op(x, x)), T>::value,
      "Result type of binary_op must match reduction accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  uint32_t mask_bits;
  mask.extract_bits(mask_bits);
  return sycl::detail::calc<T, __spv::GroupOperation::Reduce,
                            __spv::Scope::Subgroup>(
      typename sycl::detail::GroupOpTag<T>::type(), x, binary_op, mask_bits);
#else
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename V, typename T, class BinaryOperation>
detail::enable_if_t<(std::is_scalar<V>::value && std::is_scalar<T>::value &&
                     (detail::IsPlus<V, BinaryOperation>::value ||
                   detail::IsMinimum<V, BinaryOperation>::value ||
                   detail::IsMaximum<V, BinaryOperation>::value)) &&
                   detail::is_native_op<T, BinaryOperation>::value),
                    T>
reduce_over_group(sub_group g, sub_group_mask mask, V x, T init,
                  BinaryOperation binary_op) {
  static_assert(
      std::is_same<decltype(binary_op(init, x)), T>::value,
      "Result type of binary_op must match reduction accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  return binary_op(init, reduce_over_group(g, mask, x, binary_op));
#else
  (void)g;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

// ---- joint_reduce
template <typename Ptr, class BinaryOperation>
detail::enable_if_t<
    (detail::is_pointer<Ptr>::value &&
     detail::is_integral<typename detail::remove_pointer<Ptr>::type>::value),
    typename detail::remove_pointer<Ptr>::type>
joint_reduce(sub_group g, sub_group_mask mask, Ptr first, Ptr last,
             BinaryOperation binary_op) {
  using T = typename detail::remove_pointer<Ptr>::type;
  static_assert(
      std::is_same<decltype(binary_op(*first, *first)), T>::value,
      "Result type of binary_op must match reduction accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  T partial = sycl::known_identity_v<BinaryOperation, T>;
  uint32_t mask_bits;
  mask.extract_bits(mask_bits);
  ptrdiff_t offset = popcount(mask_bits & ((1 << g.get_local_linear_id()) - 1));
  ptrdiff_t stride = popcount(mask_bits);
  for (Ptr p = first + offset; p < last; p += stride) {
    partial = binary_op(partial, *p);
  }
  return reduce_over_group(g, mask, partial, binary_op);
#else
  (void)g;
  (void)last;
  (void)binary_op;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

template <typename Ptr, typename T, class BinaryOperation>
detail::enable_if_t<
    (detail::is_pointer<Ptr>::value &&
     detail::is_integral<typename detail::remove_pointer<Ptr>::type>::value &&
     detail::is_integral<T>::value &&
     detail::is_native_op<typename detail::remove_pointer<Ptr>::type,
                          BinaryOperation>::value &&
     detail::is_native_op<T, BinaryOperation>::value),
    T>
joint_reduce(sub_group g, sub_group_mask mask, Ptr first, Ptr last, T init,
             BinaryOperation binary_op) {
  static_assert(
      std::is_same<decltype(binary_op(init, *first)), T>::value,
      "Result type of binary_op must match reduction accumulation type.");
#ifdef __SYCL_DEVICE_ONLY__
  T partial = sycl::known_identity_v<BinaryOperation, T>;
  uint32_t mask_bits;
  mask.extract_bits(mask_bits);
  ptrdiff_t offset = popcount(mask_bits & ((1 << g.get_local_linear_id()) - 1));
  ptrdiff_t stride = popcount(mask_bits);
  for (Ptr p = first + offset; p < last; p += stride) {
    partial = binary_op(partial, *p);
  }
  return reduce_over_group(g, mask, partial, init, binary_op);
#else
  (void)g;
  (void)last;
  throw runtime_error("Group algorithms are not supported on host device.",
                      PI_INVALID_DEVICE);
#endif
}

} // namespace oneapi
} // namespace ext

} // namespace sycl
} // __SYCL_INLINE_NAMESPACE(cl)
