/*
** Copyright (c) AImotive Kft. 2020
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

#ifndef AIWARE_RUNTIME_COMMON_C__RAW_BUFFER_ORDERING_H
#define AIWARE_RUNTIME_COMMON_C__RAW_BUFFER_ORDERING_H

#ifdef __cplusplus
extern "C"
{
#endif

	/// Defines the ordering types of raw tensor buffers.
	typedef enum aiwRawTensorBufferOrdering
	{
		AIW_RTBO_INVALID = 0,
		AIW_RTBO_TILE_ORDERED = 1,
		AIW_RTBO_CELLROW_ORDERED = 2
	} aiwRawTensorBufferOrdering;

#ifdef __cplusplus
}
#endif

#endif //AIWARE_RUNTIME_COMMON_C__RAW_BUFFER_ORDERING_H
