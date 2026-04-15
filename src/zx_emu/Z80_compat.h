/* Z80_compat.h — standalone replacement for Zeta (Z) library headers.
   Provides all Z_* macros and z* types needed by Z80.h / Z80.c
   so the Zeta library does not need to be installed. */

#ifndef Z80_COMPAT_H
#define Z80_COMPAT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

/* --- Primitive type aliases -------------------------------------------- */

typedef uint8_t  zuint8;
typedef int8_t   zsint8;
typedef uint16_t zuint16;
typedef uint32_t zuint32;
typedef size_t       zusize;
typedef int          zsint;
typedef unsigned int zuint;
typedef char         zchar;
typedef bool         zbool;

/* --- Compound types (little-endian ARM assumed) ------------------------ */

typedef union {
	uint16_t uint16_value;
	struct { uint8_t at_0, at_1; } uint8_values;
} ZInt16;

typedef union {
	uint32_t uint32_value;
	uint8_t  uint8_array[4];
	uint16_t uint16_array[2];
} ZInt32;

/* --- Pointer / boolean constants --------------------------------------- */

#ifndef Z_NULL
#define Z_NULL NULL
#endif

#ifndef Z_TRUE
#define Z_TRUE  1
#endif

#ifndef Z_FALSE
#define Z_FALSE 0
#endif

/* --- Integer literal macros -------------------------------------------- */

#define Z_USIZE(value)        ((zusize)(value))
#define Z_USIZE_MAXIMUM       SIZE_MAX
#define Z_UINT16(value)       ((zuint16)(value))
#define Z_UINT32(value)       ((zuint32)(value))

/* --- Cast -------------------------------------------------------------- */

#define Z_CAST(type) (type)

/* --- Language helpers -------------------------------------------------- */

#if defined(__GNUC__) || defined(__clang__)
#	define Z_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#	define Z_ALWAYS_INLINE inline
#endif

#ifdef __cplusplus
#	define Z_EXTERN_C_BEGIN extern "C" {
#	define Z_EXTERN_C_END   }
#else
#	define Z_EXTERN_C_BEGIN
#	define Z_EXTERN_C_END
#endif

/* --- API visibility ---------------------------------------------------- */

#if defined(_WIN32)
#	define Z_API_EXPORT __declspec(dllexport)
#	define Z_API_IMPORT __declspec(dllimport)
#else
#	define Z_API_EXPORT __attribute__((visibility("default")))
#	define Z_API_IMPORT __attribute__((visibility("default")))
#endif

/* --- Bitwise helpers --------------------------------------------------- */

#define Z_UINT8_ROTATE_LEFT(value, rotation) \
	((zuint8)(((zuint8)(value) << (rotation)) | ((zuint8)(value) >> (8 - (rotation)))))

#define Z_UINT8_ROTATE_RIGHT(value, rotation) \
	((zuint8)(((zuint8)(value) >> (rotation)) | ((zuint8)(value) << (8 - (rotation)))))

/* Big-endian byte-swap for 16-bit value (target is little-endian ARM) */
#define Z_UINT16_BIG_ENDIAN(value) \
	((zuint16)(((zuint16)(value) >> 8) | ((zuint16)(value) << 8)))

/* Big-endian byte-swap for 32-bit value (target is little-endian ARM) */
#define Z_UINT32_BIG_ENDIAN(value) \
	((zuint32)( \
		(((zuint32)(value) & 0xFF000000u) >> 24) | \
		(((zuint32)(value) & 0x00FF0000u) >>  8) | \
		(((zuint32)(value) & 0x0000FF00u) <<  8) | \
		(((zuint32)(value) & 0x000000FFu) << 24)))

/* --- Structure helpers ------------------------------------------------- */

#define Z_MEMBER_OFFSET(type, member) ((zusize)offsetof(type, member))

/* --- Miscellaneous ----------------------------------------------------- */

#define Z_EMPTY
#define Z_UNUSED(variable) (void)(variable);

#ifdef _WIN32
#	define Z_MICROSOFT_STD_CALL __stdcall
#else
#	define Z_MICROSOFT_STD_CALL
#endif

#endif /* Z80_COMPAT_H */
