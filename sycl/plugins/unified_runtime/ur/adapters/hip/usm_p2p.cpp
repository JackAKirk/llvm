//===--------- usm_p2p.cpp - HIP Adapter-----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "common.hpp"
#include "context.hpp"

UR_APIEXPORT ur_result_t UR_APICALL urUsmP2PEnablePeerAccessExp(

    ur_device_handle_t commandDevice, ur_device_handle_t peerDevice) {

 

  ur_result_t result = UR_RESULT_SUCCESS;

  try {

    ScopedContext active(commandDevice);

    UR_CHECK_ERROR(hipCtxEnablePeerAccess(peerDevice->getNativeContext(), 0));

  } catch (ur_result_t err) {

    result = err;

  }

  return result;

}

 

UR_APIEXPORT ur_result_t UR_APICALL urUsmP2PDisablePeerAccessExp(

    ur_device_handle_t commandDevice, ur_device_handle_t peerDevice) {

 

  ur_result_t result = UR_RESULT_SUCCESS;

  try {

    ScopedContext active(commandDevice);

    UR_CHECK_ERROR(hipCtxDisablePeerAccess(peerDevice->getNativeContext()));

  } catch (ur_result_t err) {

    result = err;

  }

  return result;

}

 

UR_APIEXPORT ur_result_t UR_APICALL urUsmP2PPeerAccessGetInfoExp(

    ur_device_handle_t commandDevice, ur_device_handle_t peerDevice,

    ur_exp_peer_info_t propName, size_t propSize, void *pPropValue,

    size_t *pPropSizeRet) {

 

  UrReturnHelper ReturnValue(propSize, pPropValue, pPropSizeRet);

 

  int value;

  hipDeviceP2PAttr hip_attr;

  try {

    ScopedContext active(commandDevice);

    switch (propName) {

    case UR_EXP_PEER_INFO_UR_PEER_ACCESS_SUPPORTED: {

      hip_attr = hipDevP2PAttrAccessSupported;

      break;

    }

    case UR_EXP_PEER_INFO_UR_PEER_ATOMICS_SUPPORTED: {

      hip_attr = hipDevP2PAttrNativeAtomicSupported;

      break;

    }

    default: {

      return UR_RESULT_ERROR_INVALID_ENUMERATION;

    }

    }

 

    UR_CHECK_ERROR(hipDeviceGetP2PAttribute(

        &value, hip_attr, commandDevice->get(), peerDevice->get()));

  } catch (ur_result_t err) {

    return err;

  }

  return ReturnValue(value);

}