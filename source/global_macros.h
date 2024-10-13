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
#define EXPAND_AND_STRINGIFY(x) STRINGIFY(x)
#define STRINGIFY(x) #x
#define LOG10(n) ((n) < 10 ? 0 : (n) < 100 ? 1 : (n) < 1000 ? 2 : (n) < 10000 ? 3 : 4)
#define NUMOF(array) (sizeof (array) / sizeof *(array))
#define BYTES(bits) (((bits) + CHAR_BIT - 1) / CHAR_BIT)
#define OCTETS(bytes) (((bytes) * CHAR_BIT + 7) / 8)
