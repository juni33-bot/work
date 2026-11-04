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

#ifndef AIWARE_RUNTIME_COMMON_C__TYPES_H
#define AIWARE_RUNTIME_COMMON_C__TYPES_H
/// Represents an aiWare device
typedef struct aiwDeviceImpl aiwDevice;
/// Represents an aiWare program
typedef struct aiwProgramImpl aiwProgram;
typedef struct aiwProgramSetImpl aiwProgramSet;
typedef struct aiwProgramSetBuilderImpl aiwProgramSetBuilder;
typedef struct aiwProgramImplTensorInfo aiwTensor;
/// Opaque type representing a memory area by which the tensor is backed.
typedef void aiwExternalMemoryDescriptor;

#endif //AIWARE_RUNTIME_COMMON_C__TYPES_H
