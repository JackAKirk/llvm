// DEFINE: %{mathflags} = %if cl_options %{/clang:-fno-fast-math%} %else %{-fno-fast-math%}

// TODO fix windows failures
// UNSUPPORTED: windows && (level_zero || opencl)
// RUN: %{build} %{mathflags} -o %t.out
// RUN: %{run} %t.out

// tests sycl floating point math functions for sycl::vec and sycl::marray float
// and double cases.
//#include <hip/hip_runtime.h>
#include "math_test_marray_vec_common.hpp"
#include <sycl/ext/oneapi/backend/hip.hpp>

int main() {
    //sycl::device sdev;// sdev(sycl::default_selector{});
  //auto ocl_dev = sycl::get_native<sycl::backend::ext_oneapi_hip>(sdev);
  hipDevice_t devhip;
  sycl::device dev(sycl::make_device<sycl::backend::ext_oneapi_hip>(devhip));
  queue deviceQueue(dev);
  math_tests_4<float4>(deviceQueue);
  math_tests_4<marray<float, 4>>(deviceQueue);

  math_tests_3<float3>(deviceQueue);
  math_tests_3<marray<float, 3>>(deviceQueue);

  if (deviceQueue.get_device().has(sycl::aspect::fp64)) {
    math_tests_4<double4>(deviceQueue);
    math_tests_4<marray<double, 4>>(deviceQueue);

    math_tests_3<double3>(deviceQueue);
    math_tests_3<marray<double, 3>>(deviceQueue);
  }

  std::cout << "Pass" << std::endl;
  return 0;
}
