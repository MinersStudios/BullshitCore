#ifndef BULLSHITCORE_GLOBAL_MACROS
#define BULLSHITCORE_GLOBAL_MACROS

#ifdef __has_builtin
# if __has_builtin(__builtin_expect)
#  define likely(x) __builtin_expect(!!(x), 1)
#  define unlikely(x) __builtin_expect((x), 0)
# endif
#endif
#ifndef likely
# define likely(x) (x)
#endif
#ifndef unlikely
# define unlikely(x) (x)
#endif

#define PERROR_AND_EXIT(s) { perror(s); exit(EXIT_FAILURE); }

#endif
