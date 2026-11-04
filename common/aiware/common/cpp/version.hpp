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

#ifndef AIWARE_COMMON_CPP__VERSION_HPP
#define AIWARE_COMMON_CPP__VERSION_HPP

#include "aiware/common/cpp/aiware-common-lib-cpp_export.h"
#include "aiware/common/cpp/moduleversion.hpp"

namespace aiware
{
namespace common
{

/// Returns the version info of the library.
AIWARE_COMMON_LIB_CPP_EXPORT ModuleVersion version();

} // namespace common
} // namespace aiware

#endif //AIWARE_COMMON_CPP__VERSION_HPP
