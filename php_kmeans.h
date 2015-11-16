/**********************************************************\
|                                                          |
| php_kmeans.h											   |
| Author xiaofeng										   |
|                                                          |
\**********************************************************/

#ifndef PHP_KMEANS_H
#define PHP_KMEANS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern zend_module_entry kmeans_module_entry;
#define phpext_kmeans_ptr &kmeans_module_entry

#ifdef PHP_WIN32
#    define PHP_KMEANS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#    define PHP_KMEANS_API __attribute__ ((visibility("default")))
#else
#    define PHP_KMEANS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define PHP_KMEANS_VERSION		"0.1"
#define PHP_KMEANS_MODULE_NAME	"kmeans"
#define PHP_KMEANS_BUILD_DATA	__DATE__ " " __TIME__
#define PHP_KMEANS_AUTHOR		"xiaofeng"
#define PHP_KMEANS_HOMEPAGE		""

#ifndef PARAM_OUT
#define PARAM_OUT
#endif

#ifndef PREC_TYPE
#define PREC_TYPE 0
#endif

#include <float.h>

#if PREC_TYPE==0
#define prec_t double
#define PREC_MAX DBL_MAX
#define PREC_MIN DBL_MIN
#elif PREC_TYPE==1
#define prec_t float
#define PREC_MAX FLT_MAX
#define PREC_MIN FLT_MIN
#endif

#define emalloc2D(var, x, y, type)							\
do															\
{														    \
	size_t i;												\
	(var) = (type **)emalloc((x) * sizeof(type *));         \
	(var)[0] = (type *)emalloc((x) * (y) * sizeof(type));   \
	for (i = 1; i < x; i++)									\
	{														\
		(var)[i] = (var)[i-1] + y;                          \
	}														\
} while (0)													\

#define ecalloc2D(var, x, y, type)							\
do															\
{														    \
	size_t i;												\
	(var) = (type **)emalloc((x) * sizeof(type *));         \
	(var)[0] = (type *)ecalloc((x) * (y), sizeof(type));    \
	for (i = 1; i < x; i++)									\
	{														\
		(var)[i] = (var)[i-1] + y;                          \
	}														\
} while (0)													\

#define efree2D(var)										\
do															\
{															\
	efree((var)[0]);										\
	efree((var));											\
} while(0)													\


PHP_MINIT_FUNCTION(kmeans);
PHP_MSHUTDOWN_FUNCTION(kmeans);
PHP_RINIT_FUNCTION(kmeans);
PHP_RSHUTDOWN_FUNCTION(kmeans);
PHP_MINFO_FUNCTION(kmeans);

/*
extern ZEND_DECLARE_MODULE_GLOBALS(kmeans);
*/

ZEND_FUNCTION(kmeans);

#endif /* ifndef PHP_KMEANS_H */