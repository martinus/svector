#pragma once

#if __has_include(<absl/container/inlined_vector.h>)
#    define ANKERL_SVECTOR_HAS_ABSL() 1
#    include <absl/container/inlined_vector.h>
#else
#    define ANKERL_SVECTOR_HAS_ABSL() 0
#endif

#if __has_include(<boost/container/small_vector.hpp>)
#    define ANKERL_SVECTOR_HAS_BOOST() 1
#    include <boost/container/small_vector.hpp>
#else
#    define ANKERL_SVECTOR_HAS_BOOST() 0
#endif
