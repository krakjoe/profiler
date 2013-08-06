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

#if PHP_VERSION_ID >= 50500
void (*zend_execute_old)(zend_execute_data *execute_data TSRMLS_DC);
void profiler_execute(zend_execute_data *execute_data TSRMLS_DC);
#else
void (*zend_execute_old)(zend_op_array *op_array TSRMLS_DC);
void profiler_execute(zend_op_array *ops TSRMLS_DC);
#endif

const zend_function_entry profiler_functions[] = {
	PHP_FE(profiler_enable, NULL)
	PHP_FE(profiler_output, NULL)
	PHP_FE(profiler_disable, NULL)
	PHP_FE_END
};

zend_module_entry profiler_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	PROFILER_NAME,
	profiler_functions,
	PHP_MINIT(profiler),
	PHP_MSHUTDOWN(profiler),
	PHP_RINIT(profiler),	
	PHP_RSHUTDOWN(profiler),
	PHP_MINFO(profiler),
#if ZEND_MODULE_API_NO >= 20010901
	PROFILER_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PROFILER
ZEND_GET_MODULE(profiler)
#endif

PHP_INI_BEGIN()
PHP_INI_ENTRY("profiler.enabled", "0", PHP_INI_SYSTEM, NULL)
PHP_INI_ENTRY("profiler.memory", "1", PHP_INI_SYSTEM, NULL)
PHP_INI_ENTRY("profiler.output", "/tmp/profile.callgrind", PHP_INI_SYSTEM, NULL)
PHP_INI_END()

static inline void profiler_globals_ctor(zend_profiler_globals *pg TSRMLS_DC) {
	pg->enabled = 0;
	pg->memory = 1;
	pg->output = NULL;
	pg->reset = 0;
	pg->frame = &pg->frames[0];
	pg->limit = &pg->frames[PROFILER_MAX_FRAMES];
}
static inline void profiler_globals_dtor(zend_profiler_globals *pg TSRMLS_DC) {}

PHP_MINIT_FUNCTION(profiler)
{
#if PHP_VERSION_ID >= 50500
    zend_execute_old = zend_execute_ex;
    zend_execute_ex = profiler_execute;
#else
    zend_execute_old = zend_execute;
    zend_execute = profiler_execute;
#endif
	
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
	PROF_G(enabled) = INI_BOOL("profiler.enabled");
	PROF_G(memory) = INI_BOOL("profiler.memory");
	PROF_G(output) = INI_STR("profiler.output");
	
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(profiler)
{
	if (PROF_G(output)) {
		FILE *stream = fopen(PROF_G(output), "w");
		if (stream) {
			profile_t profile, end;

			fprintf(stream, "version: 1\n");
			fprintf(stream, "creator: profiler\n");
			fprintf(stream, "pid: %d\n", getpid());	
			if (PROF_G(memory)) {
				fprintf(stream, "events: memory cpu\n");
			} else fprintf(stream, "events: cpu\n");
			
			profile = &PROF_G(frames)[0];		
			end = PROF_G(frame);			
			do {
				if (profile) {
					fprintf(stream, "fl=%s\n", profile->location.file);
					if (profile->call.scope && strlen(profile->call.scope)) {
						fprintf(stream, "fn=%s::%s\n", profile->call.scope, profile->call.function);
					} else fprintf(stream, "fn=%s\n", profile->call.function);
					
					if (PROF_G(memory)) {
						fprintf(
							stream, 
							"%d %ld %lld\n", 
							profile->location.line, (profile->call.memory > 0L) ? profile->call.memory : 0L, profile->call.cpu
						);
					} else fprintf(
						stream, 
						"%d %lld\n", 
						profile->location.line, profile->call.cpu
					);
					fprintf(stream, "\n");
				} else break;
			} while (++profile < end);
			fclose(stream);
		} else zend_error(E_WARNING, "the profiler has failed to open %s for writing", PROF_G(output));
		if (PROF_G(reset))
			free(PROF_G(output));
	}
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

PHP_FUNCTION(profiler_output)
{
	uint flength;
	char *fpath;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &fpath, &flength)==SUCCESS) {
		if (PROF_G(reset))
			free(PROF_G(output));
		
		PROF_G(output) = malloc(flength+1);
		if (PROF_G(output)) {
			if (strncpy(PROF_G(output), fpath, flength) != PROF_G(output)) {
				PROF_G(output)=INI_STR("profiler.output");
				PROF_G(reset)=0;
				free(PROF_G(output));
				RETURN_NULL();
			}
		}
		PROF_G(reset)=1;
	}
}

PHP_FUNCTION(profiler_disable)
{
	if (PROF_G(enabled)) {
		PROF_G(enabled)=0;
	} else zend_error(E_WARNING, "the profiler is already disabled");
}

#if PHP_VERSION_ID >= 50500
void profiler_execute(zend_execute_data *input TSRMLS_DC) {
#else
void profiler_execute(zend_op_array *input TSRMLS_DC) {
#endif
	ulong line = 0L;
	
	if (PROF_G(enabled) && 
		(PROF_G(frame) < PROF_G(limit)) && 
		(line = zend_get_executed_lineno(TSRMLS_C))) {
	    profile_t profile = PROF_G(frame)++;
		profile->location.file = zend_get_executed_filename(TSRMLS_C);
		profile->location.line = line;
		profile->call.function = get_active_function_name(TSRMLS_C);
		profile->call.scope = (EG(called_scope)) ? EG(called_scope)->name : "";

		if (PROF_G(memory))
			profile->call.memory = zend_memory_usage(0 TSRMLS_CC);
	
		profile->call.cpu = ticks();
		zend_execute_old(
		    input TSRMLS_CC);
		profile->call.cpu = ticks() - profile->call.cpu;

		if (PROF_G(memory))
			profile->call.memory = (zend_memory_usage(0 TSRMLS_CC) - profile->call.memory);
			
    } else zend_execute_old(input TSRMLS_CC);
}

