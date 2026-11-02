
#ifndef AIWARE_EMULATOR_LIB_EXPORT_H
#define AIWARE_EMULATOR_LIB_EXPORT_H

#ifdef AIWARE_EMULATOR_LIB_STATIC_DEFINE
#  define AIWARE_EMULATOR_LIB_EXPORT
#  define AIWARE_EMULATOR_LIB_NO_EXPORT
#else
#  ifndef AIWARE_EMULATOR_LIB_EXPORT
#    ifdef aiware_emulator_lib_EXPORTS
        /* We are building this library */
#      define AIWARE_EMULATOR_LIB_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define AIWARE_EMULATOR_LIB_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef AIWARE_EMULATOR_LIB_NO_EXPORT
#    define AIWARE_EMULATOR_LIB_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef AIWARE_EMULATOR_LIB_DEPRECATED
#  define AIWARE_EMULATOR_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef AIWARE_EMULATOR_LIB_DEPRECATED_EXPORT
#  define AIWARE_EMULATOR_LIB_DEPRECATED_EXPORT AIWARE_EMULATOR_LIB_EXPORT AIWARE_EMULATOR_LIB_DEPRECATED
#endif

#ifndef AIWARE_EMULATOR_LIB_DEPRECATED_NO_EXPORT
#  define AIWARE_EMULATOR_LIB_DEPRECATED_NO_EXPORT AIWARE_EMULATOR_LIB_NO_EXPORT AIWARE_EMULATOR_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef AIWARE_EMULATOR_LIB_NO_DEPRECATED
#    define AIWARE_EMULATOR_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* AIWARE_EMULATOR_LIB_EXPORT_H */
