/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Johann Zelger <jz@appserver.io>                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_FHREADS_H
#define PHP_FHREADS_H

extern zend_module_entry fhreads_module_entry;
#define phpext_fhreads_ptr &fhreads_module_entry

#define PHP_FHREADS_VERSION "0.2.0"

#ifdef PHP_WIN32
#	define PHP_FHREADS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_FHREADS_API __attribute__ ((visibility("default")))
#else
#	define PHP_FHREADS_API
#endif

#include <stdio.h>

#ifndef _WIN32
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#else
#include <pthread.h>
#include <signal.h>
#endif

#include <main/SAPI.h>
#include <Zend/zend.h>
#include <Zend/zend_API.h>
#include <Zend/zend_alloc.h>
#include <Zend/zend_operators.h>
#include <Zend/zend_closures.h>
#include <Zend/zend_compile.h>
#include <Zend/zend_execute.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_extensions.h>
#include <Zend/zend_globals.h>
#include <Zend/zend_hash.h>
#include <Zend/zend_ts_hash.h>
#include <Zend/zend_interfaces.h>
#include <Zend/zend_list.h>
#include <Zend/zend_object_handlers.h>
#include <Zend/zend_variables.h>
#include <Zend/zend_ptr_stack.h>
#include <Zend/zend_constants.h>
#include <Zend/zend_closures.h>
#include <Zend/zend_generators.h>
#include <Zend/zend_vm.h>
#include <TSRM/TSRM.h>

#include "ext/standard/basic_functions.h"
#include "ext/spl/php_spl.h"

typedef struct _fhread_object {
	pthread_t thread_id;
	pthread_mutex_t syncMutex;
	pthread_mutex_t execMutex;
	pthread_cond_t notify;
	void ***c_tsrm_ls;
	void ***tsrm_ls;
	uint32_t handle;
	uint32_t runnable_handle;
	int is_initialized;
	int is_joined;
	int rv;
	zval *runnable;
} fhread_object;

typedef struct _fhread_objects_store {
	fhread_object **object_buckets;
	uint32_t top;
	uint32_t size;
	int free_list_head;
	pthread_mutex_t mutex;
} fhread_objects_store;

#ifdef HAVE_SIGNAL_H
#ifdef _WIN32
#	define FHREAD_KILL_SIGNAL SIGBREAK
#else
#	define FHREAD_KILL_SIGNAL SIGUSR1
#endif
#endif

/* {{{ TSRM manipulation */
#define FHREADS_FETCH_ALL(ls, id, type) ((type) (*((void ***) ls))[TSRM_UNSHUFFLE_RSRC_ID(id)])
#define FHREADS_FETCH_CTX(ls, id, type, element) (((type) (*((void ***) ls))[TSRM_UNSHUFFLE_RSRC_ID(id)])->element)
#define FHREADS_CG(ls, v) FHREADS_FETCH_CTX(ls, compiler_globals_id, zend_compiler_globals*, v)
#define FHREADS_CG_ALL(ls) FHREADS_FETCH_ALL(ls, compiler_globals_id, zend_compiler_globals*)
#define FHREADS_EG(ls, v) FHREADS_FETCH_CTX(ls, executor_globals_id, zend_executor_globals*, v)
#define FHREADS_SG(ls, v) FHREADS_FETCH_CTX(ls, sapi_globals_id, sapi_globals_struct*, v)
#define FHREADS_PG(ls, v) FHREADS_FETCH_CTX(ls, core_globals_id, php_core_globals*, v)
#define FHREADS_SPL_G(ls, v) FHREADS_FETCH_CTX(ls, spl_globals_id, zend_spl_globals*, v)
#define FHREADS_EG_ALL(ls) FHREADS_FETCH_ALL(ls, executor_globals_id, zend_executor_globals*)

/* }}} */

#ifdef ZTS
#define FHREADS_G(v) TSRMG(fhreads_globals_id, zend_fhreads_globals *, v)
#else
#define FHREADS_G(v) (fhreads_globals.v)
#endif

#endif	/* PHP_FHREADS_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
