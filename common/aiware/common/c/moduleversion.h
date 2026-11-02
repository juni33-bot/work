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

#ifndef AIW_COM_C__MODULE_VERSION_H
#define AIW_COM_C__MODULE_VERSION_H

#include "aiware/common/c/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// Describes the version of a module.
///
/// The string members should not be deleted.
#ifndef __cplusplus
	typedef
#endif
		struct
#ifdef __cplusplus
		aiwModuleVersion
#endif
	{
		/// The full version (major.minor.path [postfix]) represented as string.
		const aiw_char_t* version;

		/// The major version number.
		uint32_t major_;
		/// The minor version number.
		uint32_t minor_;
		/// The patch version number.
		uint32_t patch_;

		/// The major version number as string.
		const aiw_char_t* majorStr;
		/// The minor version number as string.
		const aiw_char_t* minorStr;
		/// The patch version number as string.
		const aiw_char_t* patchStr;

		/// Optional version postfix.
		const aiw_char_t* postfix;

		/// The full git commit id of the library.
		const aiw_char_t* gitRev;
		/// The short, 10 char long git commit if of the library.
		const aiw_char_t* gitRevShort;

#ifndef __cplusplus
	} aiwModuleVersion;
#else
	aiwModuleVersion()
		: version(nullptr)
		, major_(0)
		, minor_(0)
		, patch_(0)
		, majorStr(nullptr)
		, minorStr(nullptr)
		, patchStr(nullptr)
		, postfix(nullptr)
		, gitRev(nullptr)
		, gitRevShort(nullptr)
	{
	}
};
#endif

#ifdef __cplusplus
}
#endif

#endif //AIW_COM_C__MODULE_VERSION_H
