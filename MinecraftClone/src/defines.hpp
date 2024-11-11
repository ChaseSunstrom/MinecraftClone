
#ifndef DEFINES_HPP
#define DEFINES_HPP

#if (!defined(RELEASE) && !defined(__DIST__) && !defined(NDEBUG)) || defined(DEBUG)
#	define __DEBUG__
#endif

#ifndef __DIST__
#	ifndef NDEBUG
#		define __TRACE__
#	endif

#	define __INFO__
#	define __WARN__
#	define __ERROR__
#	define __FATAL__
#endif

#define assert_false assert(false)

#define _MC ::MC::



#endif
