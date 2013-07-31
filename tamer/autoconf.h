#ifndef TAMER_AUTOCONF_H
#define TAMER_AUTOCONF_H 1
/* Copyright (c) 2012-2013, Eddie Kohler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Tamer LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Tamer LICENSE file; the license in that file is
 * legally binding.
 */

#if __GNUC__
# define TAMER_GCCVERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
# define TAMER_GCCVERSION 0
#endif
#if __clang__
# define TAMER_CLANGVERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#else
# define TAMER_CLANGVERSION 0
#endif

#if __cplusplus >= 201103L || __GXX_EXPERIMENTAL_CXX0X__
#if !defined(TAMER_HAVE_CXX_CONSTEXPR) && \
  (TAMER_CLANGVERSION >= 30100 || TAMER_GCCVERSION >= 40600)
/* Define if the C++ compiler understands constexpr. */
#define TAMER_HAVE_CXX_CONSTEXPR 1
#endif

#if !defined(TAMER_HAVE_CXX_NOEXCEPT) && \
  (TAMER_CLANGVERSION >= 30000 || TAMER_GCCVERSION >= 40600)
/* Define if the C++ compiler understands noexcept. */
#define TAMER_HAVE_CXX_NOEXCEPT 1
#endif

#if !defined(TAMER_HAVE_CXX_RVALUE_REFERENCES) && \
  (TAMER_CLANGVERSION >= 20900 || TAMER_GCCVERSION >= 40300)
/* Define if the C++ compiler understands rvalue references. */
#define TAMER_HAVE_CXX_RVALUE_REFERENCES 1
#endif

#if !defined(TAMER_HAVE_CXX_STATIC_ASSERT) && \
  (TAMER_CLANGVERSION >= 20900 || TAMER_GCCVERSION >= 40300)
/* Define if the C++ compiler understands static_assert. */
#define TAMER_HAVE_CXX_STATIC_ASSERT 1
#endif

#if !defined(TAMER_HAVE_CXX_TEMPLATE_ALIAS) && \
  (TAMER_CLANGVERSION >= 30000 || TAMER_GCCVERSION >= 40700)
/* Define if the C++ compiler understands template alias. */
#define TAMER_HAVE_CXX_TEMPLATE_ALIAS 1
#endif

#if !defined(TAMER_HAVE_CXX_VARIADIC_TEMPLATES) && \
  (TAMER_CLANGVERSION >= 20900 || TAMER_GCCVERSION >= 40300)
/* Define if the C++ compiler understands variadic templates. */
#define TAMER_HAVE_CXX_VARIADIC_TEMPLATES 1
#endif
#endif

#if TAMER_HAVE_CXX_NOEXCEPT
#define TAMER_NOEXCEPT noexcept
#else
#define TAMER_NOEXCEPT
#endif

#if TAMER_HAVE_CXX_CONSTEXPR
#define TAMER_CONSTEXPR constexpr
#else
#define TAMER_CONSTEXPR
#endif

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
#define TAMER_MOVEARG(t, ...) t, ## __VA_ARGS__ &&
#define TAMER_MOVE(v) std::move(v)
#else
#define TAMER_MOVEARG(t, ...) t, ## __VA_ARGS__
#define TAMER_MOVE(v) v
#endif

#if !defined(TAMER_HAVE_PREEVENT) && TAMER_HAVE_CXX_RVALUE_REFERENCES
#define TAMER_HAVE_PREEVENT 1
#endif

#if __GNUC__
#define TAMER_CLOSUREVARATTR __attribute__((unused))
#define TAMER_DEPRECATEDATTR __attribute__((deprecated))
#else
#define TAMER_CLOSUREVARATTR
#define TAMER_DEPRECATEDATTR
#endif

#if __GNUC__ && TAMER_DEBUG
#define TAMER_MAKE_ANNOTATED_EVENT(r, ...) make_annotated_event(__FILE__, __LINE__, r, ## __VA_ARGS__)
#define TAMER_MAKE_FN_ANNOTATED_EVENT(r, ...) make_annotated_event(__PRETTY_FUNCTION__, 0, r, ## __VA_ARGS__)
#elif TAMER_DEBUG
#define TAMER_MAKE_ANNOTATED_EVENT(r, ...) make_annotated_event(__FILE__, __LINE__, r, ## __VA_ARGS__)
#define TAMER_MAKE_FN_ANNOTATED_EVENT(r, ...) make_annotated_event(__FILE__, __LINE__, r, ## __VA_ARGS__)
#else
#define TAMER_MAKE_ANNOTATED_EVENT make_event
#define TAMER_MAKE_FN_ANNOTATED_EVENT make_event
#endif

#endif