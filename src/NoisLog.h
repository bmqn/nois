#ifndef H_NOIS_LOG_H
#define H_NOIS_LOG_H

#include <stdio.h>

//-------------------------------------------------------------------------------------------------
//	Utility
//-------------------------------------------------------------------------------------------------
#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define _EXPAND(x) x
#define _VARGS(_9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N

#if defined(__cplusplus)

/* Taken from https://stackoverflow.com/a/54335644 */

template <typename T, size_t S>
inline constexpr size_t GetFileNameOffset(const T(&str)[S], size_t i = S - 1)
{
	return (str[i] == '/' || str[i] == '\\') ? i + 1 : (i > 0 ? GetFileNameOffset(str, i - 1) : 0);
}
template <typename T>
inline constexpr size_t GetFileNameOffset(T(&str)[1])
{
	return 0;
}

#define __FILENAME__ (&__FILE__[GetFileNameOffset(__FILE__)])

#else

#define __FILENAME__ __FILE__

#endif // defined(__cplusplus)

#ifndef __FUNCTION_NAME__
#ifdef WIN32
#define __FUNCTION_NAME__ __FUNCTION__
#else
#define __FUNCTION_NAME__ __func__
#endif
#endif

//-------------------------------------------------------------------------------------------------
//	Debug Breaks
//-------------------------------------------------------------------------------------------------
#if NOIS_ENABLE_BREAKS
#include <debugbreak.h>
#else
inline void debug_break() {}
#endif // NOIS_ENABLE_BREAKS

//-------------------------------------------------------------------------------------------------
//	Logging
//-------------------------------------------------------------------------------------------------
#if NOIS_ENABLE_LOGGING

#define _LOG1(format)                                                                             \
do                                                                                                \
{                                                                                                 \
	fprintf(stdout, "[Info]");                                                                    \
	fprintf(stdout, "[%s:%d][%s] ", __FILENAME__, __LINE__, __FUNCTION_NAME__);                   \
	fprintf(stdout, format);                                                                      \
	fprintf(stdout, "\n");                                                                        \
} while(0)

#define _LOG2(format, ...)                                                                        \
do                                                                                                \
{                                                                                                 \
	fprintf(stdout, "[Info]");                                                                    \
	fprintf(stdout, "[%s:%d][%s] ", __FILENAME__, __LINE__, __FUNCTION_NAME__);                   \
	fprintf(stdout, format, __VA_ARGS__);                                                         \
	fprintf(stdout, "\n");                                                                        \
} while(0)

#define _LOG_CHOOSER(...) _EXPAND(                                                                \
_VARGS(__VA_ARGS__,                                                                               \
_LOG2, _LOG2, _LOG2,                                                                              \
_LOG2, _LOG2, _LOG2,                                                                              \
_LOG2, _LOG2, _LOG1)                                                                              \
)

#define LOG(...) _EXPAND(_LOG_CHOOSER(__VA_ARGS__)(__VA_ARGS__))

#define _LOG_UNIMPLEMENTED1()                                                                     \
do                                                                                                \
{                                                                                                 \
	fprintf(stdout, "[Unimplemented]");                                                           \
	fprintf(stdout, "[%s:%d][%s] ", __FILENAME__, __LINE__, __FUNCTION_NAME__);                   \
	fprintf(stdout, "\n");                                                                        \
} while(0)

#define _LOG_UNIMPLEMENTED2(format)                                                               \
do                                                                                                \
{                                                                                                 \
	fprintf(stdout, "[Unimplemented]");                                                           \
	fprintf(stdout, "[%s:%d][%s] ", __FILENAME__, __LINE__, __FUNCTION_NAME__);                   \
	fprintf(stdout, format);                                                                      \
	fprintf(stdout, "\n");                                                                        \
} while(0)

#define _LOG_UNIMPLEMENTED3(format, ...)                                                          \
do                                                                                                \
{                                                                                                 \
	fprintf(stdout, "[Unimplemented]");                                                           \
	fprintf(stdout, "[%s:%d][%s] ", __FILENAME__, __LINE__, __FUNCTION_NAME__);                   \
	fprintf(stdout, format, __VA_ARGS__);                                                         \
	fprintf(stdout, "\n");                                                                        \
} while(0)

#define _LOG_UNIMPLEMENTED_CHOOSER(...) _EXPAND(                                                  \
_VARGS(__VA_ARGS__,                                                                               \
_LOG_UNIMPLEMENTED3, _LOG_UNIMPLEMENTED3, _LOG_UNIMPLEMENTED3,                                    \
_LOG_UNIMPLEMENTED3, _LOG_UNIMPLEMENTED3, _LOG_UNIMPLEMENTED3,                                    \
_LOG_UNIMPLEMENTED3, _LOG_UNIMPLEMENTED2, _LOG_UNIMPLEMENTED1)                                    \
)

#define LOG_UNIMPLEMENTED(...) _EXPAND(_LOG_UNIMPLEMENTED_CHOOSER(__VA_ARGS__)(__VA_ARGS__))

#else

#define LOG(...)
#define LOG_UNIMPLEMENTED(...)

#endif // NOIS_ENABLE_LOGGING

//-------------------------------------------------------------------------------------------------
//	Assertions
//-------------------------------------------------------------------------------------------------
#if NOIS_ENABLE_ASSERTIONS

#define _ASSERT1(condition)                                                                       \
do                                                                                                \
{                                                                                                 \
	if (!(condition))                                                                             \
	{                                                                                             \
		fprintf(stdout, "[Assertion Failed]");                                                    \
		fprintf(stdout, "[%s:%d][%s]", __FILENAME__, __LINE__, __FUNCTION_NAME__);                \
		fprintf(stdout, "\n");                                                                    \
		debug_break();                                                                            \
	}                                                                                             \
} while(0)

#define _ASSERT2(condition, format)                                                               \
do                                                                                                \
{                                                                                                 \
	if (!(condition))                                                                             \
	{                                                                                             \
		fprintf(stdout, "[Assertion Failed]");                                                    \
		fprintf(stdout, "[%s:%d][%s] ", __FILENAME__, __LINE__, __FUNCTION_NAME__);               \
		fprintf(stdout, format);                                                                  \
		fprintf(stdout, "\n");                                                                    \
		debug_break();                                                                            \
	}                                                                                             \
} while (0)

#define _ASSERT3(condition, format, ...)                                                          \
do                                                                                                \
{                                                                                                 \
	if (!(condition))                                                                             \
	{                                                                                             \
		fprintf(stdout, "[Assertion Failed]");                                                    \
		fprintf(stdout, "[%s:%d][%s] ", __FILENAME__, __LINE__, __FUNCTION_NAME__);               \
		fprintf(stdout, format, __VA_ARGS__);                                                     \
		fprintf(stdout, "\n");                                                                    \
		debug_break();                                                                            \
	}                                                                                             \
} while (0)

#define _ASSERT_CHOOSER(...) _EXPAND(                                                             \
_VARGS(__VA_ARGS__,                                                                               \
_ASSERT3, _ASSERT3, _ASSERT3,                                                                     \
_ASSERT3, _ASSERT3, _ASSERT3,                                                                     \
_ASSERT3, _ASSERT2, _ASSERT1)                                                                     \
)

#define ASSERT(...) _EXPAND(_ASSERT_CHOOSER(__VA_ARGS__)(__VA_ARGS__))

#else

#define ASSERT(condition, ...)

#endif  // NOIS_ENABLE_ASSERTIONS

#endif // H_NOIS_LOG_H
