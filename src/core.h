/*
 * core.h
 *
 * qotd - A simple QOTD daemon.
 * Copyright (c) 2015-2016 Emmie Smith
 *
 * qotd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * qotd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with qotd.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CORE_H_
#define _CORE_H_

/* Definitions */

#define PROGRAM_NAME				"qotd"
#define PROGRAM_VERSION_MAJOR			0
#define PROGRAM_VERSION_MINOR			12
#define PROGRAM_VERSION_PATCH			0

#if !defined(GIT_HASH)
# define GIT_HASH				"nogithash"
#endif /* GIT_HASH */

#if !defined(DEBUG)
# define DEBUG					0
#endif /* DEBUG */

#if DEBUG
# pragma message("Compiling with debug statements.")
#endif /* DEBUG */

#if defined(__clang__)
# define COMPILER_NAME				"Clang/LLVM"
# define COMPILER_VERSION			__clang_version__
#elif defined(__ICC) || defined(__INTEL_COMPILER)
# define COMPILER_NAME				"Intel ICC"
# define COMPILER_VERSION			__INTEL_COMPILER
#elif defined(__MINGW32__)
# define COMPILER_NAME				"Mingw"
# define COMPILER_VERSION			__VERSION__
#elif defined(__GNUC__) || defined(__GNUG__)
# define COMPILER_NAME				"GCC"
# define COMPILER_VERSION			__VERSION__
#elif defined(__HP_cc) || defined(__HP_aCC)
# define COMPILER_NAME				"Hewlett-Packard C"
# define COMPILER_VERSION			__HP_cc
#elif defined(__IBMC__) || defined(__IBMCPP__)
# define COMPILER_NAME				"IBM XL C"
# define COMPILER_VERSION			__xlc__
#elif defined(_MSC_VER)
# define COMPILER_NAME				"Microsoft Visual Studio"
# define COMPILER_VERSION			_MSC_VER
#elif defined(__PGI)
# define COMPILER_NAME				"Portland Group PGCC"
# define COMPILER_VERSION			__PGIC__
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
# define COMPILER_NAME				"Oracle Solaris Studio"
# define COMPILER_VERSION			__SUNPRO_C
#else
# define COMPILER_NAME				"Unknown"
# define COMPILER_VERSION			"compiler"
#endif

#if defined(_WIN64)
# define PLATFORM_NAME				"Windows 64-bit"
#elif defined(_WIN32)
# define PLATFORM_NAME				"Windows 32-bit"
#elif defined(__linux__)
# define PLATFORM_NAME				"Linux"
#elif defined(__APPLE__) || defined(__MACH__)
# define PLATFORM_NAME				"Apple"
#elif defined(__FreeBSD__)
# define PLATFORM_NAME				"FreeBSD"
#elif defined(__unix) || defined(__unix__)
# define PLATFORM_NAME				"Unknown UNIX"
#else
# define PLATFORM_NAME				"Unknown"
#endif

/* Macros */

#define UNUSED(x)				((void)(x))
#define STATIC_ASSERT(x)			((void)sizeof(char[2 * (!!(x)) - 1]))
#define ARRAY_SIZE(x)				(sizeof(x) / sizeof((x)[0]))
#define DEFAULT(x,a)				((x) ? (x) : (a))
#define PLURAL(x)				(((x) == 1) ? "" : "s")
#define EMPTYSTR(x)				(((x)[0]) == '\0')

#define MIN(x,y)				(((x) < (y)) ? (x) : (y))
#define MAX(x,y)				(((x) > (y)) ? (x) : (y))

#define SWAP(x,y,t)				\
	do {					\
		t __temp_var_ = (a);		\
		(a) = (b);			\
		(b) = __temp_var_;		\
	} while (0)

#if defined(RELEASE)
# define FINAL_FREE(x)				UNUSED(x)
#else
# define FINAL_FREE(x)				free(x)
#endif /* RELEASE */

/* Extensions */

#if defined(__GNUC__) || defined(__clang__)
# define likely(x)				__builtin_expect(!!(x), 1)
# define unlikely(x)				__builtin_expect(!!(x), 0)
# define NORETURN				__attribute__((noreturn))
#else
# define likely(x)				(x)
# define unlikely(x)				(x)
# define NORETURN
#endif /* __GNUC__ || __clang__ */

/* Functions */

void print_version(void);

#endif /* _CORE_H_ */
