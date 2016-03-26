#ifndef PUBLIC_DEFINE_H
#define PUBLIC_DEFINE_H

#define S_UNLIKELY(x) __builtin_expect((x), 0)
#define S_LIKELY(x) __builtin_expect((x), 1)

#endif /*PUBLIC_DEFINE_H*/
