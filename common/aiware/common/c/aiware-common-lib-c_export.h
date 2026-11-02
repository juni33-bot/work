
#ifndef AIWARE_COMMON_LIB_C_EXPORT_H
#define AIWARE_COMMON_LIB_C_EXPORT_H

#ifdef AIWARE_COMMON_LIB_C_STATIC_DEFINE
#  define AIWARE_COMMON_LIB_C_EXPORT
#  define AIWARE_COMMON_LIB_C_NO_EXPORT
#else
#  ifndef AIWARE_COMMON_LIB_C_EXPORT
#    ifdef aiware_common_lib_c_EXPORTS
        /* We are building this library */
#      define AIWARE_COMMON_LIB_C_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define AIWARE_COMMON_LIB_C_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef AIWARE_COMMON_LIB_C_NO_EXPORT
#    define AIWARE_COMMON_LIB_C_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef AIWARE_COMMON_LIB_C_DEPRECATED
#  define AIWARE_COMMON_LIB_C_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef AIWARE_COMMON_LIB_C_DEPRECATED_EXPORT
#  define AIWARE_COMMON_LIB_C_DEPRECATED_EXPORT AIWARE_COMMON_LIB_C_EXPORT AIWARE_COMMON_LIB_C_DEPRECATED
#endif

#ifndef AIWARE_COMMON_LIB_C_DEPRECATED_NO_EXPORT
#  define AIWARE_COMMON_LIB_C_DEPRECATED_NO_EXPORT AIWARE_COMMON_LIB_C_NO_EXPORT AIWARE_COMMON_LIB_C_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef AIWARE_COMMON_LIB_C_NO_DEPRECATED
#    define AIWARE_COMMON_LIB_C_NO_DEPRECATED
#  endif
#endif

#endif /* AIWARE_COMMON_LIB_C_EXPORT_H */
