#define BOUND_ADD(x, y, underflow, overflow) ((x) <= ((y) < 0 ? (underflow) : (overflow)) - (y) ? (x) + (y) : (x))
#define BOUND_DEREF(v, i, lowerbound, upperbound) (v)[(i) >= (lowerbound) && (i) <= (upperbound) ? (i) : 0]
#define BOUND_MULT(x, y, underflow, overflow) ((x) <= ((y) < 0 ? (underflow) : (overflow)) / (y) ? (x) * (y) : (x))
#define BYTES(bits) (((bits) + CHAR_BIT - 1) / CHAR_BIT)
#define EXPAND_AND_STRINGIFY(macro) STRINGIFY(macro)
#ifdef __has_builtin
# if __has_builtin(__builtin_expect)
#  define likely(condition) __builtin_expect(!!(condition), 1)
#  define unlikely(condition) __builtin_expect((condition), 0)
# endif
#endif
#ifndef likely
# define likely(condition) (condition)
#endif
#ifndef unlikely
# define unlikely(condition) (condition)
#endif
#define LOG10(n) ((n) < 10 ? 0 : (n) < 100 ? 1 : (n) < 1000 ? 2 : (n) < 10000 ? 3 : 4)
#define NUMOF(array) (sizeof (array) / sizeof *(array))
#define OCTETS(bytes) (((bytes) * CHAR_BIT + 7) / 8)
#define PERROR_AND_EXIT(string) { perror(string); exit(EXIT_FAILURE); }
#define STRINGIFY(macro) #macro
