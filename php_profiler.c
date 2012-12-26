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
#include "php_streams.h"
#include "ext/standard/info.h"
#include "php_profiler.h"

ZEND_DECLARE_MODULE_GLOBALS(profiler)

#define PROFILE_NORMAL 	1
#define PROFILE_INTERN	2

#ifndef _WIN32
static __inline__ ticks_t ticks(void) {
	unsigned x, y;
	asm volatile ("rdtsc" : "=a" (x), "=d" (y));
	return ((((ticks_t)x) | ((ticks_t)y) << 32));
}
#else
#	include <intrin.h>
static inline ticks_t ticks(void) {
	return __rdtsc();
}
#endif

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
	PHP_FE(profiler_callgrind, NULL)
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
	"0.3",
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PROFILER
ZEND_GET_MODULE(profiler)
#endif

PHP_INI_BEGIN()
PHP_INI_ENTRY("profiler.enabled", "0", PHP_INI_SYSTEM, NULL)
PHP_INI_ENTRY("profiler.timing", "0", PHP_INI_SYSTEM, NULL)
PHP_INI_ENTRY("profiler.memory", "1", PHP_INI_SYSTEM, NULL)
PHP_INI_END()

static inline void profiler_globals_ctor(zend_profiler_globals *pg TSRMLS_DC) {
	pg->enabled = 0;
	pg->timing = 0;
	pg->memory = 1;
}
static inline void profiler_globals_dtor(zend_profiler_globals *pg TSRMLS_DC) {}

PHP_MINIT_FUNCTION(profiler)
{
	zend_execute = profiler_execute;
	zend_execute_internal = profiler_execute_internal;
	
	REGISTER_LONG_CONSTANT("PROFILE_NORMAL", PROFILE_NORMAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PROFILE_INTERN", PROFILE_NORMAL, CONST_CS | CONST_PERSISTENT);
	
	REGISTER_INI_ENTRIES();

	ZEND_INIT_MODULE_GLOBALS(profiler, profiler_globals_ctor, profiler_globals_dtor);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(profiler)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}

PHP_RINIT_FUNCTION(profiler)
{	
	if (INI_BOOL("profiler.enabled")) {
		PROF_G(enabled)=1;
	} else PROF_G(enabled)=0;
	
	if (INI_BOOL("profiler.timing")) {
		PROF_G(timing)=1;
	} else PROF_G(timing)=0;

	if (!INI_BOOL("profiler.memory")) {
		PROF_G(memory)=0;
	} else PROF_G(memory)=1;
	
	zend_llist_init(
		&PROF_G(profile), sizeof(profile_t*), (llist_dtor_func_t) profiler_destroy, 0
	);	

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
	} else zend_error(E_WARNING, "the profiler is already enabled");
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
						if (PROF_G(timing)) {
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
						}
							
						/** add file information **/
						ALLOC_INIT_ZVAL(location);
						{
							array_init(location);
							add_assoc_string(location, "file", (char*) profile->location.file, 1);
							add_assoc_long(location, "line", profile->location.line);								
						}
						add_assoc_zval(profiled, "location", location);
						
						/** add call information **/
						if (profile->call.function) {
							ALLOC_INIT_ZVAL(call);
							{
								array_init(call);
								add_assoc_string(call, "function", (char*) profile->call.function, 1);
								if (profile->call.scope) {
									add_assoc_string(call, "scope", (char*) profile->call.scope, 1);
								}
								if (PROF_G(memory))
									add_assoc_long(call, "memory", profile->call.memory);
								add_assoc_long(call, "cpu", profile->call.cpu);
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

PHP_FUNCTION(profiler_callgrind) 
{
	php_stream *stream;
	zval *resource;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &resource)==SUCCESS) {
		php_stream_from_zval(stream, &resource);
		
		php_stream_printf(stream TSRMLS_CC, "version: 1\n");
		php_stream_printf(stream TSRMLS_CC, "creator: profiler\n");
		php_stream_printf(stream TSRMLS_CC, "pid: %d\n", getpid());	
		if (PROF_G(memory)) {
			php_stream_printf(stream TSRMLS_CC, "events: memory cpu\n");
		} else php_stream_printf(stream TSRMLS_CC, "events: cpu\n");
		
		profile_t *pprofile = zend_llist_get_first(&PROF_G(profile));
		if (pprofile) do {
			profile_t profile = *pprofile;
			if (profile) {
				php_stream_printf(stream TSRMLS_CC, "fl=%s\n", profile->location.file);
				if (profile->call.scope && strlen(profile->call.scope)) {
					php_stream_printf(stream TSRMLS_CC, "fn=%s::%s\n", profile->call.scope, profile->call.function);
				} else php_stream_printf(stream TSRMLS_CC, "fn=%s\n", profile->call.function);
					
				if (PROF_G(memory)) {
					php_stream_printf(
						stream TSRMLS_CC, 
						"%d %ld %lld\n", 
						profile->location.line, (profile->call.memory > 0) ? profile->call.memory : 0, profile->call.cpu
					);
				} else php_stream_printf(
					stream TSRMLS_CC, 
					"%d %lld\n", 
					profile->location.line, profile->call.cpu
				);
				php_stream_printf(stream TSRMLS_CC, "\n");
			}
		} while((pprofile = zend_llist_get_next(&PROF_G(profile))));
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
	} else zend_error(E_WARNING, "the profiler is already disabled");
}

static void profiler_destroy(profile_t *profile) {
	if (profile) {
		if ((*profile)) {
			efree((*profile));
		}
	}
}

static inline void _profile(void *_input, uint type, int uret TSRMLS_DC) {
	if (PROF_G(enabled)) {
		profile_t profile = emalloc(sizeof(*profile));
		profile->type = type;
		profile->location.file = zend_get_executed_filename(TSRMLS_C);
		profile->location.line = zend_get_executed_lineno(TSRMLS_C);
		profile->call.function = get_active_function_name(TSRMLS_C);
		profile->call.scope = (EG(called_scope)) ? EG(called_scope)->name : "";
	
		if (PROF_G(timing))
			gettimeofday(&(profile->timing.entered), NULL);
		
		if (PROF_G(memory))
			profile->call.memory = zend_memory_usage(0 TSRMLS_CC);
	
		profile->call.cpu = ticks();
		switch(type) {
			case PROFILE_INTERN: {
				execute_internal((zend_execute_data *) _input, uret TSRMLS_CC);	
			} break;
	
			case PROFILE_NORMAL: {
				execute((zend_op_array*) _input TSRMLS_CC);
			} break;
		}
		profile->call.cpu = ticks() - profile->call.cpu;

		if (PROF_G(memory))
			profile->call.memory = (zend_memory_usage(0 TSRMLS_CC) - profile->call.memory);
		if (PROF_G(timing))
			gettimeofday(&(profile->timing.left), NULL);					

		zend_llist_add_element(&PROF_G(profile), &profile);
	} else switch(type) {
		case PROFILE_INTERN: {
			execute_internal((zend_execute_data *) _input, uret TSRMLS_CC);	
		} break;
	
		case PROFILE_NORMAL: {
			execute((zend_op_array*) _input TSRMLS_CC);
		} break;
	}
}

static void profiler_execute(zend_op_array *ops TSRMLS_DC) {
	_profile(ops, PROFILE_NORMAL, 0 TSRMLS_CC);
}

static void profiler_execute_internal(zend_execute_data *data, int return_value_used TSRMLS_DC) {
	_profile(data, PROFILE_INTERN, return_value_used TSRMLS_CC);
}
