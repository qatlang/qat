#ifndef QAT_UTILS_MACROS_HPP
#define QAT_UTILS_MACROS_HPP

#define useit  [[nodiscard]]
#define exitFn [[noreturn]]

#if defined __MINGW32__ || defined __MINGW64__
#define RUNTIME_IS_MINGW 1
#define RUNTIME_IS_MSVC	 0
#elif defined _WIN32 || defined WIN32 || defined WIN64 || defined _WIN64
#define RUNTIME_IS_MSVC	 1
#define RUNTIME_IS_MINGW 0
#endif

#endif
