#if !defined(NOC_STATIC_ASSERT_H_INCLUDED)
#define NOC_STATIC_ASSERT_H_INCLUDED

#if !defined(NOC_DISABLE_STATIC_ASSERT) && !defined(NOC_STATIC_ASSERT)

    #if (defined(NOC_DETECT_LANGUAGE_CXX) && (NOC_DETECT_CXX_VERSION >= NOC_DETECT_CXX_VERSION_2011)) || \
	(defined(NOC_DETECT_LANGUAGE_C) && (NOC_DETECT_C_VERSION >= NOC_DETECT_C_VERSION_2023))

	#define NOC_STATIC_ASSERT(EXPR, MSG) static_assert(EXPR, MSG)

    #elif defined(NOC_DETECT_LANGUAGE_C) && (NOC_C_STD_VERSION >= NOC_C_STD_2011)

	#define NOC_STATIC_ASSERT(EXPR, MSG) _Static_assert(EXPR, MSG)

    #else

	#error "No static assert possible :C"

    #endif

#else
    #define NOC_STATIC_ASSERT(EXPR, MSG)
#endif

#endif // NOC_STATIC_ASSERT_H_INCLUDED
