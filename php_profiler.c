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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_profiler.h"

ZEND_DECLARE_MODULE_GLOBALS(profiler)

#define PROFILE_NORMAL 	1
#define PROFILE_INTERN	2

typedef void (*zend_execute_handler_t)(zend_op_array *, void ***);
typedef void (*zend_execute_internal_handler_t)(zend_execute_data *, int, void***);

zend_execute_handler_t zend_execute_original;
zend_execute_internal_handler_t zend_execute_internal_original;

static void profiler_destroy(profile_t *profile);
static void profiler_execute(zend_op_array *ops TSRMLS_DC);
static void profiler_execute_internal(zend_execute_data *data, int return_value_used TSRMLS_DC);

const zend_function_entry profiler_functions[] = {
	PHP_FE(profiler_enable, NULL)
	PHP_FE(profiler_fetch, NULL)
	PHP_FE(profiler_clear, NULL)
	PHP_FE(profiler_disable, NULL)
	PHP_FE_END
};

zend_module_entry profiler_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"profiler",
	profiler_functions,
	PHP_MINIT(profiler),
	PHP_MSHUTDOWN(profiler),
	PHP_RINIT(profiler),	
	PHP_RSHUTDOWN(profiler),
	PHP_MINFO(profiler),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1",
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PROFILER
ZEND_GET_MODULE(profiler)
#endif

PHP_MINIT_FUNCTION(profiler)
{
	zend_execute_original = zend_execute;
	zend_execute_internal_original = zend_execute_internal;
	
	zend_execute = profiler_execute;
	zend_execute_internal = profiler_execute_internal;
	
	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(profiler)
{
	zend_execute = zend_execute_original;
	zend_execute_internal = zend_execute_internal_original;
	return SUCCESS;
}

PHP_RINIT_FUNCTION(profiler)
{	
	ZEND_INIT_MODULE_GLOBALS(profiler, NULL, NULL);

	zend_llist_init(
		&PROF_G(profile), sizeof(profile_t*), (llist_dtor_func_t) profiler_destroy, 0
	);

	PROF_G(enabled)=0;
	
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(profiler)
{
	zend_llist_destroy(
		&PROF_G(profile)
	);

	return SUCCESS;
}

PHP_MINFO_FUNCTION(profiler)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "profiler support", "enabled");
	php_info_print_table_end();
}

PHP_FUNCTION(profiler_enable) 
{
	if (!PROF_G(enabled)) {
		PROF_G(enabled)=1;	
	} else zend_error(E_WARNING, "the profiler is currently running enabled");
}

PHP_FUNCTION(profiler_fetch) 
{
	array_init(return_value);
	{
		profile_t *pprofile = zend_llist_get_first(&PROF_G(profile));
		if (pprofile) do {
			profile_t profile = *pprofile;
			if (profile) {
				zval *profiled, *timing, *location, *call;
				
				ALLOC_INIT_ZVAL(profiled);
				{
					array_init(profiled);
					{
						/** set type of profile data */
						add_assoc_long(profiled, "type", profile->type);
						
						/** insert timing information **/
						ALLOC_INIT_ZVAL(timing);
						{
							zval *entered, *left;
							array_init(timing);
							{
								ALLOC_INIT_ZVAL(entered);
								{
									array_init(entered);
									add_assoc_long(entered, "tv_sec", profile->timing.entered.tv_sec);
									add_assoc_long(entered, "tv_usec", profile->timing.entered.tv_usec);
								}
								add_assoc_zval(timing, "entered", entered);

								ALLOC_INIT_ZVAL(left);
								{
									array_init(left);
									add_assoc_long(left, "tv_sec", profile->timing.left.tv_sec);
									add_assoc_long(left, "tv_usec", profile->timing.left.tv_usec);
								}
								add_assoc_zval(timing, "left", left);
							}
							add_assoc_zval(profiled, "timing", timing);
						}
						
						/** add file information **/
						ALLOC_INIT_ZVAL(location);
						{
							array_init(location);
							add_assoc_string(location, "file", profile->location.file, strlen(profile->location.file));
							add_assoc_long(location, "line", profile->location.line);								
						}
						add_assoc_zval(profiled, "location", location);
						
						/** add call information **/
						if (profile->call.function) {
							ALLOC_INIT_ZVAL(call);
							{
								array_init(call);
								add_assoc_string(call, "function", profile->call.function, strlen(profile->call.function));
								if (profile->call.scope) {
									add_assoc_string(call, "scope", profile->call.scope, strlen(profile->call.scope));
								}
								add_assoc_long(call, "overhead", profile->call.overhead);
							}
							add_assoc_zval(profiled, "call", call);
						}
					}
				}
				add_next_index_zval(return_value, profiled);
			}
		} while ((pprofile = zend_llist_get_next(&PROF_G(profile))) != NULL);
	}
}

PHP_FUNCTION(profiler_clear)
{
	zend_llist_clean(&PROF_G(profile));
}

PHP_FUNCTION(profiler_disable)
{
	if (PROF_G(enabled)) {
		PROF_G(enabled)=0;
	} else zend_error(E_WARNING, "the profiler has not yet been enabled");
}

static void profiler_destroy(profile_t *profile) {
	if (profile) {
		profile_t pointer = (*profile);
		if (pointer) {
			efree(pointer->location.file);	
			if (pointer->call.function)
				efree(pointer->call.function);
			if (pointer->call.scope)
				efree(pointer->call.scope);
			efree(pointer);
		}
	}
}

static inline void _profile(void *_input, uint type, int uret TSRMLS_DC) {
	profile_t profile = NULL;
	zend_bool enabled = PROF_G(enabled);
	size_t memory;
	if (enabled) {
		profile = emalloc(sizeof(*profile));
		if (profile) {
			profile->type = type;
			profile->location.file = estrdup(zend_get_executed_filename(TSRMLS_C));
			profile->location.line = zend_get_executed_lineno(TSRMLS_C);
			profile->call.function = estrdup(get_active_function_name(TSRMLS_C));
			if (!EG(scope)) {
				profile->call.spacing = NULL;
				profile->call.scope = NULL;
			} else {
				profile->call.scope = estrdup(
					get_active_class_name(&(profile->call.spacing) TSRMLS_CC)
				);
			}	
			gettimeofday(&(profile->timing.entered), NULL);

			memory = zend_memory_usage(0 TSRMLS_CC);
		}
	}

	switch(type) {
		case PROFILE_INTERN: {
			execute_internal((zend_execute_data *) _input, uret TSRMLS_CC);	
		} break;
		
		case PROFILE_NORMAL: {
			execute((zend_op_array*) _input TSRMLS_CC);
		} break;
	}

	if (enabled) {
		profile->call.overhead = (zend_memory_usage(0 TSRMLS_CC) - memory);
		gettimeofday(
			&(profile->timing.left), NULL
		);			
		zend_llist_add_element(&PROF_G(profile), &profile);
	}
}

static void profiler_execute(zend_op_array *ops TSRMLS_DC) {
	_profile(ops, PROFILE_NORMAL, 0 TSRMLS_CC);
}

static void profiler_execute_internal(zend_execute_data *data, int return_value_used TSRMLS_DC) {
	_profile(data, PROFILE_INTERN, return_value_used TSRMLS_CC);
}
