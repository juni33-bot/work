/*
** Copyright (c) AImotive Kft. 2024
**
** The intellectual and technical concepts and implementations contained herein (including
** data structures, algorithms and essential business logic developed by AImotive Kft.) are
** proprietary to AImotive Kft., and may be covered by patents, and/or copyright law. This
** hardware or software is protected by trade secret, confidential business secret and as a
** general principle must be treated as confidential information.
**
** You may not use this hardware or software without specific prior written permission
** obtained from AImotive Kft.
**
** Access to this hardware or software is hereby forbidden to anyone except for Contracted
** Partners who have prior signed License Agreement, or Confidentiality, Non-Disclosure
** Agreements or any other equivalent Agreements explicitly covering such access and use.
**
** UNLESS OTHERWISE AGREED, THIS HARDWARE OR SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS
** OR IMPLIED WARRANTIES, INCLUDING - BUT NOT LIMITED TO - THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
*/

#ifndef AIWARE_RUNTIME_PRIVATE__SOCKET_H
#define AIWARE_RUNTIME_PRIVATE__SOCKET_H

#include "aiware/common/c/status.h"
#include "aiware/common/c/types.h"
#include "aiware/runtime/c/aiware-runtime-device-lib-c_export.h"
#include "aiware/runtime/c/platform/defines.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwPlatformSocketContextCreate(aiwSocketContext* context); /* Also defined in platform specific implementation */

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwPlatformSocketContextFree(aiwSocketContext* context); /* Also defined in platform specific implementation */

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwPlatformGetAddrInfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformFreeAddrInfo(struct addrinfo* res);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwPlatformSocket(int domain, int type, int protocol, aiwSocket* socket);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwPlatformConnect(aiwSocket socket, const aiwSockAddr* address, aiwSockLen addressLength);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwPlatformBind(aiwSocket socket, const aiwSockAddr* address, aiwSockLen addressLength);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformListen(aiwSocket socket, int backlog);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT
	aiw_status aiwPlatformAccept(
		aiwSocket socket,
		aiwSockAddr* address,
		aiwSockLen* addressLength,
		aiwSocket* retSocket);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT void aiwPlatformCloseSocket(
		aiwSocket socket); /* Also defined in platform specific implementation */

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT
	aiw_status aiwPlatformSend(aiwSocket socket, const void* buffer, size_t length, int flags, size_t* sent);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT
	aiw_status aiwPlatformRecv(aiwSocket socket, void* buffer, size_t length, int flags, size_t* recvd);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformRecvExact(aiwSocket socket, uint8_t* data, size_t length);

#ifdef __cplusplus
}
#endif

#endif // AIWARE_RUNTIME_PRIVATE__SOCKET_H
