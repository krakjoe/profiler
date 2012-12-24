/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Joe Watkins <joe.watkins@live.co.uk>                         |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_PROFILER_H
#define PHP_PROFILER_H

extern zend_module_entry profiler_module_entry;
#define phpext_profiler_ptr &profiler_module_entry

#ifdef PHP_WIN32
#	define PHP_PROFILER_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_PROFILER_API __attribute__ ((visibility("default")))
#else
#	define PHP_PROFILER_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(profiler);
PHP_MSHUTDOWN_FUNCTION(profiler);
PHP_RINIT_FUNCTION(profiler);
PHP_RSHUTDOWN_FUNCTION(profiler);
PHP_MINFO_FUNCTION(profiler);

#ifdef ZTS
#define PROF_G(v) TSRMG(profiler_globals_id, zend_profiler_globals *, v)
#else
#define PROF_G(v) (profiler_globals.v)
#endif

typedef struct {
	struct timeval entered;
	struct timeval left;
} timing_t;

typedef unsigned long long ticks_t;
typedef struct {
	const char *scope;
	const char *spacing;
	const char *function;
	size_t memory;
	ticks_t cpu;
} call_t;

typedef struct {
	char *file;
	uint line;
} location_t;

typedef struct {
	uint type;
	timing_t timing;
	location_t location;
	call_t call;
} *profile_t;

ZEND_BEGIN_MODULE_GLOBALS(profiler)
	zend_bool	enabled;
	zend_bool	memory;
	zend_bool	timing;
	zend_llist	profile;
ZEND_END_MODULE_GLOBALS(profiler)

PHP_FUNCTION(profiler_enable);
PHP_FUNCTION(profiler_fetch);
PHP_FUNCTION(profiler_clear);
PHP_FUNCTION(profiler_disable);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
