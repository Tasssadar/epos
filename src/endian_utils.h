/*
 *	epos/src/endian_utils.h
 *	2005, Adam Nohejl - adam@nohejl.name
 *	2005, Jirka Hanika
 *
 *  Public Domain
 *  Feel free to modify, redistribute and use this file in any project,
 *  no matter which licence it has.  If you want to make me happy, don't
 *  change these lines.
 */

/*
 * This file provides byte order conversions for 16-bit and 32-bit integers.
 * 
 * The only exported names are the lowercase inlines at the end of the file.
 * If you intend to use the uppercase macros as well, simply remove the #undef
 * block.
 *
 * We do not rely on determining endianness using autoconf,
 * as this would probably make cross-compilation dependent
 * on assumptions concerning binary object files.
 * We try to use Darwin macros or some macros defined on most systems in
 * (headers included by) <sys/types.h>. Feel free to add conditions for your system.
 *
 * (Architectures whose byte order is different from both big-endian
 * and little-endian (such as PDP) are not supported.  Modifying this
 * file only has a chance to do the job.)
 *
 */


#ifndef ENDIAN_H
#define ENDIAN_H

#include "common.h" /* Epos header: uint16_t, uint32_t */
/* Replace with <stdint.h> or your own definitions in other projects. */


/* Basic definitions that always work: */

#define ON_BIG_ENDIAN(code)	if (scfg->_big_endian) { code }
#define ON_LITTLE_ENDIAN(code)	if (!scfg->_big_endian) { code }
/* Replace with appropriate runtime endiannes test in other projects. */

#define ENDIAN_16SWAP(value)                 \
		(((((uint16_t)value)<<8) & 0xFF00)   | \
		 ((((uint16_t)value)>>8) & 0x00FF))
#define ENDIAN_32SWAP(value)                     \
		(((((uint32_t)value)<<24) & 0xFF000000)  | \
		 ((((uint32_t)value)<< 8) & 0x00FF0000)  | \
		 ((((uint32_t)value)>> 8) & 0x0000FF00)  | \
		 ((((uint32_t)value)>>24) & 0x000000FF))


/* Optimizations if possible: */

#if defined(__GNUC__) && defined(__APPLE__) && defined(__MACH__)	/* Darwin gcc */
	#include <libkern/OSByteOrder.h>
	#undef ENDIAN_16SWAP
	#undef ENDIAN_32SWAP
	#define  ENDIAN_16SWAP(value)		(uint16_t) (__builtin_constant_p(value) ? OSSwapConstInt16(value) : OSSwapInt16(value))
	#define  ENDIAN_32SWAP(value)		(uint32_t) (__builtin_constant_p(value) ? OSSwapConstInt32(value) : OSSwapInt32(value))
	#if defined (__BIG_ENDIAN__)
		#undef ON_BIG_ENDIAN
		#undef ON_LITTLE_ENDIAN
		#define ON_BIG_ENDIAN(code)   do { code } while (0);
		#define ON_LITTLE_ENDIAN(code)	;
	#elif defined (__LITTLE_ENDIAN__)
		#undef ON_BIG_ENDIAN
		#undef ON_LITTLE_ENDIAN
		#define ON_LITTLE_ENDIAN(code)   do { code } while (0);
		#define ON_BIG_ENDIAN(code)	;
	#endif
#elif defined(HAVE_SYS_TYPES_H)										/* Other reasonable systems */
	#include <sys/types.h>
/*
 * Most systems:	BYTE_ORDER == (BIG|LITTLE)_ENDIAN
 * Solaris:         _(BIG|LITTLE)_ENDIAN
 */
	#if (defined(BYTE_ORDER) && defined(BIG_ENDIAN) && (BYTE_ORDER == BIG_ENDIAN)) || defined(_BIG_ENDIAN)
		#undef ON_BIG_ENDIAN
		#undef ON_LITTLE_ENDIAN
		#define ON_BIG_ENDIAN(code)   do { code } while (0);
		#define ON_LITTLE_ENDIAN(code)	; 
	#elif (defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && (BYTE_ORDER == LITTLE_ENDIAN)) || defined(_LITTLE_ENDIAN)
		#undef ON_BIG_ENDIAN
		#undef ON_LITTLE_ENDIAN
		#define ON_LITTLE_ENDIAN(code)   do { code } while (0);
		#define ON_BIG_ENDIAN(code)	;
	#endif
#endif

/* some more macros atop the preceding ones: */

#define LE_16SWAP { ON_BIG_ENDIAN( return ENDIAN_16SWAP(arg); ) \
                    ON_LITTLE_ENDIAN( return arg; ) }
#define LE_32SWAP { ON_BIG_ENDIAN( return ENDIAN_32SWAP(arg); ) \
                    ON_LITTLE_ENDIAN( return arg; ) }

/* that's it: */


inline int16_t from_le16s(int16_t arg) { LE_16SWAP(arg); }
inline int32_t from_le32s(int32_t arg) { LE_32SWAP(arg); }
inline uint16_t from_le16u(uint16_t arg) { LE_16SWAP(arg); }
inline uint32_t from_le32u(uint32_t arg) { LE_32SWAP(arg); }

inline int16_t to_le16s(int16_t arg) { LE_16SWAP(arg); }
inline int32_t to_le32s(int32_t arg) { LE_32SWAP(arg); }
inline uint16_t to_le16u(uint16_t arg) { LE_16SWAP(arg); }
inline uint32_t to_le32u(uint32_t arg) { LE_32SWAP(arg); }


#undef LE_32SWAP
#undef LE_16SWAP
#undef ON_BIG_ENDIAN
#undef ON_LITTLE_ENDIAN
#undef ENDIAN_16SWAP
#undef ENDIAN_32SWAP


#endif		/* ENDIAN_H */
