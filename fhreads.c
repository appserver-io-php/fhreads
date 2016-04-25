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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_fhreads.h"

ZEND_DECLARE_MODULE_GLOBALS(fhreads)

ZEND_BEGIN_MODULE_GLOBALS(fhreads)
	HashTable global_value;
ZEND_END_MODULE_GLOBALS(fhreads)

/* True global resources - no need for thread safety here */
static zend_object_handlers fhreads_handlers, *fhreads_zend_handlers;
static fhread_objects_store fhread_objects;
uint32_t fhreads_zend_objects_store_top;
int fhreads_zend_objects_store_free_list_head;
pthread_mutex_t fhreads_zend_objects_store_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef void (*zend_throw_exception_hook_func)(zval * TSRMLS_DC);
zend_throw_exception_hook_func zend_throw_exception_hook_orig = NULL;

ZEND_BEGIN_ARG_INFO_EX(arginfo_fhread_create, 0, 0, 1)
	ZEND_ARG_INFO(0, runnable)
	ZEND_ARG_INFO(1, thread_id)
	ZEND_ARG_INFO(1, fhread_handle)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fhread_join, 0, 0, 1)
	ZEND_ARG_INFO(0, fhread_handle)
	ZEND_ARG_INFO(1, rv)
ZEND_END_ARG_INFO()

/* {{{ PHP_INI */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("fhreads.global_value", "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_fhreads_globals, fhreads_globals)
PHP_INI_END()
/* }}} */

/* {{{ Wrappes orig zend_objects_store_get_handle function to make it thread-safe */
ZEND_API uint32_t fhreads_zend_objects_store_get_handle() /* {{{ */
{
	uint32_t handle;
	// lock object store mutex
	pthread_mutex_lock(&fhreads_zend_objects_store_mutex);
	// set global values to objects store in current context
	EG(objects_store).top = fhreads_zend_objects_store_top;
	EG(objects_store).free_list_head = fhreads_zend_objects_store_free_list_head;
	// get next handle from orig functionality and write back to global vars for next one
	handle = zend_objects_store_get_handle();
	fhreads_zend_objects_store_top = EG(objects_store).top;
	fhreads_zend_objects_store_free_list_head = EG(objects_store).free_list_head;
	// unlock it
	pthread_mutex_unlock(&fhreads_zend_objects_store_mutex);
	// return handle
	return handle;
} /* }}} */

/* {{{ Wrappes orig fhreads_zend_objects_store_add_to_free_list function to make it thread-safe */
ZEND_API void fhreads_zend_objects_store_add_to_free_list(uint32_t handle) /* {{{ */
{
	// lock it
	pthread_mutex_lock(&fhreads_zend_objects_store_mutex);
	// set global vars to objects store
	EG(objects_store).free_list_head = fhreads_zend_objects_store_free_list_head;
	// call orig function
	zend_objects_store_add_to_free_list(handle);
	// write back to globals for next one
	fhreads_zend_objects_store_free_list_head = EG(objects_store).free_list_head;
	// unlock it
	pthread_mutex_unlock(&fhreads_zend_objects_store_mutex);
}
/* }}} */

/* {{{ Prepares zend objects store for threaded environment */
static void fhread_prepare_executor_globals()
{
	ZEND_TSRMLS_CACHE_UPDATE();

	/* prepare constants */
	// save old global consts table
	HashTable *global_constants = EG(zend_constants);
	// reinit constants
	zend_startup_constants();
	// copy constants from global constants
	zend_copy_constants(EG(zend_constants), global_constants);

	/* prepare zend objects_store */
	// recalc object store size that reallocation does not get trigged
	uint32_t new_objects_store_size = EG(objects_store).size * 1024 * 8;
	// reallocate objects store at beginning
	if (EG(objects_store).size != new_objects_store_size) {
		EG(objects_store).size = new_objects_store_size;
		EG(objects_store).object_buckets = (zend_object **) erealloc(EG(objects_store).object_buckets, EG(objects_store).size * sizeof(zend_object*));
	}

	// init global values
	fhreads_zend_objects_store_top = EG(objects_store).top;
	fhreads_zend_objects_store_free_list_head = EG(objects_store).free_list_head;

	// synchronize objects store by providing thread-safe objects_store management functions
	zend_objects_store_get_handle_ex = fhreads_zend_objects_store_get_handle;
	zend_objects_store_add_to_free_list_ex = fhreads_zend_objects_store_add_to_free_list;
} /* }}} */

/* {{{ Initialises the fhread object handlers */
static void fhread_init_object_handlers()
{
	// setup standard and fhreads object handlers
	fhreads_zend_handlers = zend_get_std_object_handlers();
	// copy handler for internal freaded objects usage
	memcpy(&fhreads_handlers, fhreads_zend_handlers, sizeof(zend_object_handlers));
	// override object handlers
	// fhreads_handlers.write_property = fhread_write_property;
	fhreads_handlers.get_gc = NULL;
} /* }}} */

/* {{{ Initialises the fhread object store */
void fhread_object_store_init()
{
	// init size
	fhread_objects.size = 1024 * 4;
	// init top
	fhread_objects.top = 1;
	// init free_list_head
	fhread_objects.free_list_head = -1;
	// init object store mutex
	pthread_mutex_init(&fhread_objects.mutex, NULL);
	// init object_buckets
	fhread_objects.object_buckets = (fhread_object **) emalloc(fhread_objects.size * sizeof(fhread_object*));
	// erase data
	memset(&fhread_objects.object_buckets[0], 0, sizeof(fhread_object*));
} /* }}} */

/* {{{ Creates and returns new fhread object */
static void fhread_object_destroy(fhread_object* fhread)
{
	// join thread if not done yet
	if (fhread->is_joined == 0) {
		fhread->is_joined = 1;
		pthread_join(fhread->thread_id, NULL);
	}
	// destroy mutex
	pthread_mutex_destroy(&fhread->syncMutex);
	pthread_mutex_destroy(&fhread->execMutex);
	// destory condition
	pthread_cond_destroy(&fhread->notify);
	// finally free object itself
	efree(fhread);
	// set pointer to null
	fhread = NULL;
} /* }}} */

/* {{{ Destroy the fhread object store */
void fhread_object_store_destroy()
{
	fhread_object **obj_ptr, **end, *obj;

	if ((fhread_objects.top <= 1)) {
		return;
	}

	// free all objects
	end = fhread_objects.object_buckets + 1;
	obj_ptr = fhread_objects.object_buckets + fhread_objects.top;

	do {
		obj_ptr--;
		obj = *obj_ptr;
		fhread_object_destroy(obj);
	} while (obj_ptr != end);
	// destroy object store mutex
	pthread_mutex_destroy(&fhread_objects.mutex);
	// free object buckets
	efree(fhread_objects.object_buckets);
	fhread_objects.object_buckets = NULL;
} /* }}} */

/* {{{ Adds a fhread object to store */
uint32_t fhread_object_store_put(fhread_object *object)
{
	// init vars
	int handle;
	// lock store access for thread safty inc stores top
	pthread_mutex_lock(&fhread_objects.mutex);
	handle = fhread_objects.top++;
	pthread_mutex_unlock(&fhread_objects.mutex);
	// add handle number to object itself
	object->handle = handle;
	// add object to store
	fhread_objects.object_buckets[handle] = object;
	// return handle
	return handle;
} /* }}} */

/* {{{ Creates and returns new fhread object */
static fhread_object* fhread_object_create()
{
	TSRMLS_CACHE_UPDATE();
	// prepare fhread
	fhread_object* fhread = emalloc(sizeof(fhread_object));
	// init return value
	fhread->rv = SUCCESS;
	// init is joined flag
	fhread->is_joined = 0;
	// init executor flag
	fhread->is_initialized = 0;
	// set thread_id
	fhread->thread_id = tsrm_thread_id();
	// set creator tsrm ls
	fhread->c_tsrm_ls = TSRMLS_CACHE;
	// create new interpreter context
	// fhread->tsrm_ls = tsrm_new_interpreter_context();

	// init internal condition
	pthread_cond_init(&fhread->notify, NULL);
	// init internal mutexe and lock it per default
	pthread_mutex_init(&fhread->syncMutex, NULL);
	pthread_mutex_init(&fhread->execMutex, NULL);
	// return inited object
	return fhread;
} /* }}} */

/* {{{ Used to register throw exception hook internal */
void fhread_throw_exception_hook(zval *ex TSRMLS_DC) {
	if (!ex)
		return;

	if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
		zend_fcall_info fci = empty_fcall_info;
		zend_fcall_info_cache fcc = empty_fcall_info_cache;
		zval retval;
		zend_string *cname;

		ZVAL_UNDEF(&retval);

		if (zend_fcall_info_init(&EG(user_exception_handler), IS_CALLABLE_CHECK_SILENT, &fci, &fcc, &cname, NULL) == SUCCESS) {
			fci.retval = &retval;

			EG(exception) = NULL;
			zend_fcall_info_argn(&fci, 1, ex);
			zend_call_function(&fci, &fcc);
			zend_fcall_info_args_clear(&fci, 1);
		}

		if (Z_TYPE(retval) != IS_UNDEF)
			zval_dtor(&retval);

		if (cname)
			zend_string_release(cname);
	}

	if (zend_throw_exception_hook_orig) {
		zend_throw_exception_hook_orig(ex);
	}

} /* }}} */

/* {{{ Initialises the compile in fhreads context */
void fhread_init_compiler(fhread_object *fhread) /* {{{ */
{
	// call original compiler
	init_compiler();
} /* }}} */

/* {{{ Initialises the executor in fhreads context */
void fhread_init_executor(fhread_object *fhread) /* {{{ */
{
	BG(locale_string) = NULL;
	BG(CurrentLStatFile) = NULL;
	BG(CurrentStatFile) = NULL;
	BG(user_shutdown_function_names) = NULL;

	// link global stuff
	EG(included_files) = FHREADS_EG(fhread->c_tsrm_ls, included_files);
	// zend_hash_init(&EG(included_files), 8, NULL, NULL, 0);

	EG(ini_directives) = FHREADS_EG(fhread->c_tsrm_ls, ini_directives);

	EG(zend_constants) = FHREADS_EG(fhread->c_tsrm_ls, zend_constants);

	CG(function_table) = FHREADS_CG(fhread->c_tsrm_ls, function_table);
	EG(function_table) = FHREADS_CG(fhread->c_tsrm_ls, function_table);

	CG(class_table) = FHREADS_CG(fhread->c_tsrm_ls, class_table);
	EG(class_table) = FHREADS_CG(fhread->c_tsrm_ls, class_table);

	//EG(regular_list) = FHREADS_EG(fhread->c_tsrm_ls, regular_list);
	//EG(persistent_list) = FHREADS_EG(fhread->c_tsrm_ls, persistent_list);

	EG(objects_store) = FHREADS_EG(fhread->c_tsrm_ls, objects_store);

	EG(symbol_table) = FHREADS_EG(fhread->c_tsrm_ls, symbol_table);
	//zend_hash_init(&EG(symbol_table), 64, NULL, ZVAL_PTR_DTOR, 0);

	EG(user_exception_handler) = FHREADS_EG(fhread->c_tsrm_ls, user_exception_handler);
	EG(user_error_handler) = FHREADS_EG(fhread->c_tsrm_ls, user_error_handler);
	EG(error_handling) = FHREADS_EG(fhread->c_tsrm_ls, error_handling);

	EG(in_autoload) = FHREADS_EG(fhread->c_tsrm_ls, in_autoload);;
	EG(autoload_func) = FHREADS_EG(fhread->c_tsrm_ls, autoload_func);

	zend_init_fpu();

	ZVAL_NULL(&EG(uninitialized_zval));
	ZVAL_NULL(&EG(error_zval));
	// destroys stack frame, therefore makes core dumps worthless
#if 0&&ZEND_DEBUG
	original_sigsegv_handler = signal(SIGSEGV, zend_handle_sigsegv);
#endif
	EG(symtable_cache_ptr) = EG(symtable_cache) - 1;
	EG(symtable_cache_limit) = EG(symtable_cache) + SYMTABLE_CACHE_SIZE - 1;
	EG(no_extensions) = 0;

	zend_vm_stack_init();

	EG(valid_symbol_table) = 1;

	EG(ticks_count) = 0;

	ZVAL_UNDEF(&EG(user_error_handler));

	EG(current_execute_data) = NULL;

	zend_stack_init(&EG(user_error_handlers_error_reporting), sizeof(int));
	zend_stack_init(&EG(user_error_handlers), sizeof(zval));
	zend_stack_init(&EG(user_exception_handlers), sizeof(zval));

	EG(full_tables_cleanup) = 0;
	#ifdef ZEND_WIN32
		EG(timed_out) = 0;
	#endif

	EG(scope) = NULL;
	EG(exception) = NULL;
	EG(prev_exception) = NULL;

	EG(ht_iterators_count) = sizeof(EG(ht_iterators_slots)) / sizeof(HashTableIterator);
	EG(ht_iterators_used) = 0;
	EG(ht_iterators) = EG(ht_iterators_slots);
	memset(EG(ht_iterators), 0, sizeof(EG(ht_iterators_slots)));

	EG(active) = 1;
} /* }}} */

/* {{{ Initialises the fhreads context */
void fhread_init(fhread_object* fhread)
{
	TSRMLS_CACHE_UPDATE();

	// check if initialization is needed
	if (fhread->is_initialized != 1) {
		// create new interpreter context
		TSRMLS_CACHE = fhread->tsrm_ls = tsrm_new_interpreter_context();
		// set prepared thread ls
		tsrm_set_interpreter_context(TSRMLS_CACHE);
		// link server context
		SG(server_context) = FHREADS_SG(fhread->c_tsrm_ls, server_context);

		// init vars
		PG(expose_php) = 0;
		PG(auto_globals_jit) = 0;
		SG(sapi_started) = 0;

#ifdef ZTS
		virtual_cwd_activate();
#endif
		gc_reset();
		// init compiler
		fhread_init_compiler(fhread);
		// init executor
		fhread_init_executor(fhread);

		// startup scanner
		startup_scanner();

		// activate output
		php_output_activate();
		// set initialized flag to be true
		fhread->is_initialized = 1;
	}
} /* }}} */

/* {{{ Run the runnable zval in given fhread context */
void *fhread_run(fhread_object* fhread)
{
	zval runnable, rv;
	zend_object* obj;
	// init fhread before run it
	fhread_init(fhread);
	// get runnable from object
	obj = EG(objects_store).object_buckets[fhread->runnable_handle];
	ZVAL_OBJ(&runnable, obj);

	// call run method
	zend_function *run;
	zend_fcall_info fci = empty_fcall_info;
	zend_fcall_info_cache fcc = empty_fcall_info_cache;
	zend_string *method = zend_string_init(ZEND_STRL("run"), 0);

	if ((run = zend_hash_find_ptr(&Z_OBJCE(runnable)->function_table, method))) {
		if (run->type == ZEND_USER_FUNCTION) {
			zend_try {
				ZVAL_STR(&fci.function_name, method);
				fci.size = sizeof(zend_fcall_info);
				fci.retval = &rv;
				fci.object = Z_OBJ(runnable);
				fci.no_separation = 1;
				fcc.initialized = 1;
				fcc.object = Z_OBJ(runnable);
				fcc.calling_scope = Z_OBJCE(runnable);
				fcc.called_scope = Z_OBJCE(runnable);
				fcc.function_handler = run;
				EG(scope) = Z_OBJCE(runnable);

				// send signal for creator logic to go ahead
				pthread_cond_broadcast(&fhread->notify);

				// call run function
				zend_call_function(&fci, &fcc);

				// destroy function return value
				zval_ptr_dtor(&rv);

				// call shutdown function to be compatible to pthreads api
				php_call_shutdown_functions();

				// set return value to success
				fhread->rv = SUCCESS;

			} zend_catch {
				// set return value to be failure
				fhread->rv = FAILURE;

			} zend_end_try();
		}
	}

	return &fhread->rv;
} /* }}} */

#ifdef FHREAD_KILL_SIGNAL
static inline void fhreads_kill_handler(int signo) /* {{{ */
{
	zend_bailout();
} /* }}} */
#endif

/* {{{ The routine to call for pthread_create */
void *fhread_routine (void* ptr)
{

#ifdef FHREAD_KILL_SIGNAL
	// register kill signal handler
	signal(FHREAD_KILL_SIGNAL, fhreads_kill_handler);
#endif
	// run & exit fhread
	pthread_exit(fhread_run(fhread_objects.object_buckets[*((uint32_t*)ptr)]));

#ifdef _WIN32
	return NULL; /* silence MSVC compiler */
#endif
} /* }}} */

/* {{{ proto fhread_tls_get_id()
	Obtain the identifier of currents ls context */
PHP_FUNCTION(fhread_tsrm_get_ls_cache)
{
	ZVAL_LONG(return_value, (long)tsrm_get_ls_cache());
} /* }}} */

/* {{{ proto fhread_mutex_destroy() */
PHP_FUNCTION(fhread_mutex_destroy)
{
	pthread_mutex_t *mutex;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &mutex)==SUCCESS && mutex) {
		RETURN_LONG(pthread_mutex_destroy(mutex));
	}
} /* }}} */

/* {{{ proto fhread_mutex_init() */
PHP_FUNCTION(fhread_mutex_init)
{
	pthread_mutex_t *mutex;
	if ((mutex=(pthread_mutex_t*) calloc(1, sizeof(pthread_mutex_t)))!=NULL) {
		pthread_mutex_init(mutex, NULL);
		RETURN_LONG((ulong)mutex);
	}
} /* }}} */

/* {{{ proto fhread_mutex_lock() */
PHP_FUNCTION(fhread_mutex_lock)
{
	pthread_mutex_t *mutex;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &mutex)==SUCCESS && mutex) {
		RETURN_LONG(pthread_mutex_lock(mutex));
	}
} /* }}} */

/* {{{ proto fhread_mutex_unlock() */
PHP_FUNCTION(fhread_mutex_unlock)
{
	pthread_mutex_t *mutex;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &mutex)==SUCCESS && mutex) {
		RETURN_LONG(pthread_mutex_unlock(mutex));
	}
} /* }}} */

/* {{{ proto fhread_cond_broadcast() */
PHP_FUNCTION(fhread_cond_broadcast)
{
	pthread_cond_t *cond;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &cond)==SUCCESS && cond) {
		RETURN_LONG(pthread_cond_broadcast(cond));
	}
} /* }}} */

/* {{{ proto fhread_cond_destroy() */
PHP_FUNCTION(fhread_cond_destroy)
{
	pthread_cond_t *cond;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &cond)==SUCCESS && cond) {
		RETURN_LONG(pthread_cond_destroy(cond));
	}
} /* }}} */

/* {{{ proto fhread_cond_init() */
PHP_FUNCTION(fhread_cond_init)
{
	pthread_cond_t *condition;
	pthread_cond_t *cond;
	if ((cond=(pthread_cond_t*) calloc(1, sizeof(pthread_cond_t)))!=NULL) {
		pthread_cond_init(cond, NULL);
		RETURN_LONG((ulong)cond);
	}
} /* }}} */

/* {{{ proto fhread_cond_signal() */
PHP_FUNCTION(fhread_cond_signal)
{
	pthread_cond_t *cond;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &cond)==SUCCESS && cond) {
		RETURN_LONG(pthread_cond_signal(cond));
	}
} /* }}} */

/* {{{ proto fhread_cond_wait() */
PHP_FUNCTION(fhread_cond_wait)
{
	pthread_cond_t *cond;
	pthread_mutex_t *mutex;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &cond, &mutex)==SUCCESS && cond  && mutex) {
		RETURN_LONG(pthread_cond_wait(cond, mutex));
	}
} /* }}} */

/* {{{ proto fhread_objects_store_top()
	Obtain current top value eg objects_store */
PHP_FUNCTION(fhread_objects_store_top)
{
	ZVAL_LONG(return_value, fhreads_zend_objects_store_top);
} /* }}} */

/* {{{ proto fhread_object_get_handle(object obj)
	Obtain the identifier of the given objects handle */
PHP_FUNCTION(fhread_object_get_handle)
{
	zval *obj;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &obj) == FAILURE) {
		RETURN_NULL();
	}
	ZVAL_LONG(return_value, Z_OBJ_HANDLE_P(obj));
} /* }}} */

/* {{{ proto fhread_self()
	Obtain the identifier of the current thread. */
PHP_FUNCTION(fhread_self)
{
	ZVAL_LONG(return_value, tsrm_thread_id());
} /* }}} */

/* {{{ proto fhread_create()
   Create a new thread, starting with execution of START-ROUTINE
   getting passed ARG.  Creation attributed come from ATTR.  The new
   handle is stored in thread_id which will be returned to php userland. */
PHP_FUNCTION(fhread_create)
{
	// init vars
	zval *runnable = NULL, *thread_id = NULL, *fhread_handle = NULL;

	// parse params
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "o|z/z/", &runnable, &thread_id, &fhread_handle) == FAILURE)
		return;

	// inject fhreads handlers
	Z_OBJ_HT_P(runnable) = &fhreads_handlers;

	// todo: check if free fhread object is available and reuse it...

	// create new fhread object
	fhread_object* fhread = fhread_object_create();
	// add new object and save handle
	uint32_t fhreads_object_handle = fhread_object_store_put(fhread);
	// save runnable handle from zval
	fhread->runnable_handle = Z_OBJ_HANDLE_P(runnable);
	// fhread->runnable = runnable;

	// add ref to runnable
	Z_ADDREF_P(runnable);

	// create thread and start fhread__routine
	int pthread_status = pthread_create(&fhread->thread_id, NULL, fhread_routine, &fhreads_object_handle);

	// check if second param was given for thread id reference
	if (thread_id) {
		zval_dtor(thread_id);
		ZVAL_LONG(thread_id, (long)fhread->thread_id);
	}

	if (fhread_handle) {
		zval_dtor(fhread_handle);
		ZVAL_LONG(fhread_handle, (long)fhreads_object_handle);
	}

	// wait on condition to get signal as soon as the thread is ready for takeoff
	pthread_cond_wait(&fhread->notify, &fhread->syncMutex);

	// return thread status
	RETURN_LONG((long)pthread_status);
} /* }}} */

/* {{{ proto fhread_join()
   Make calling thread wait for termination of the given thread id.  The
   exit status of the thread is stored in *THREAD_RETURN, if THREAD_RETURN
   is not NULL.*/
PHP_FUNCTION(fhread_join)
{
	uint32_t fhread_handle;
	fhread_object* fhread;
	zval *rv;
	int *prv;
	int status;

	// parse thread id for thread to join to
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|z/", &fhread_handle, &rv) == FAILURE) {
		RETURN_NULL();
	}

	// get fhread object
	fhread = fhread_objects.object_buckets[fhread_handle];

	// join thread if not done yet
	if (fhread->is_joined == 0) {
		fhread->is_joined = 1;
		status = pthread_join(fhread->thread_id, (void**)&prv);
	}

	ZVAL_LONG(rv, *prv);

	// return status
	RETURN_LONG((long)status);
} /* }}} */

/* {{{ proto fhread_detach()
   Marks the thread identified by thread
   as detached.  When a detached thread terminates, its resources are
   automatically released back to the system without the need for
   another thread to join with the terminated thread.*/
PHP_FUNCTION(fhread_detach)
{
	uint32_t fhread_handle;
	fhread_object* fhread;
	int status;

	// parse thread id for thread to join to
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &fhread_handle) == FAILURE) {
		RETURN_NULL();
	}

	// get fhread object
	fhread = fhread_objects.object_buckets[fhread_handle];

	// join thread if not done yet
	if (fhread->is_joined == 0) {
		fhread->is_joined = 1;
		status = pthread_detach(fhread->thread_id);
	}

	// return status
	RETURN_LONG((long)status);
}

PHP_FUNCTION(fhread_kill)
{
	uint32_t fhread_handle;
	fhread_object* fhread;
	int status;

	// parse thread id for thread to join to
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &fhread_handle) == FAILURE) {
		RETURN_NULL();
	}

	{
		// get fhread object
		fhread = fhread_objects.object_buckets[fhread_handle];

		RETURN_BOOL(pthread_kill(fhread->thread_id, FHREAD_KILL_SIGNAL)==SUCCESS);
	}
}

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(fhreads)
{
	// init module globals
	ZEND_INIT_MODULE_GLOBALS(fhreads, NULL, NULL);

	// register ini entries
	REGISTER_INI_ENTRIES();

	// init fhread object handlers
	fhread_init_object_handlers();

	// register throw exception hook and backup orig
	zend_throw_exception_hook_orig = zend_throw_exception_hook;
	zend_throw_exception_hook = fhread_throw_exception_hook;

	return SUCCESS;
} /* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(fhreads)
{
	// destroy fhreads object store
	fhread_object_store_destroy();

	// restore orig throw exception hook
	zend_throw_exception_hook = zend_throw_exception_hook_orig;

	// restore ini directives
	if (zend_copy_ini_directives() == SUCCESS) {
		zend_ini_refresh_caches(ZEND_INI_STAGE_STARTUP);
	}

	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
} /* }}} */


ZEND_MODULE_POST_ZEND_DEACTIVATE_D(fhreads)
{
	return SUCCESS;
}

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(fhreads)
{
	// init fhread objects store
	fhread_object_store_init();

	// prepare zend executor globals objects store
	fhread_prepare_executor_globals();

	return SUCCESS;
} /* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(fhreads)
{
	return SUCCESS;
} /* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(fhreads)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "fhreads support", "enabled");
	php_info_print_table_row(2, "fhreads version", PHP_FHREADS_VERSION);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
} /* }}} */

/* {{{ fhreads_functions[]
 *
 * Every user visible function must have an entry in fhreads_functions[]. */
const zend_function_entry fhreads_functions[] = {
	PHP_FE(fhread_object_get_handle, 		NULL)
	PHP_FE(fhread_objects_store_top,		NULL)

	PHP_FE(fhread_tsrm_get_ls_cache,		NULL)
	PHP_FE(fhread_self, 					NULL)
	PHP_FE(fhread_create, 					arginfo_fhread_create)
	PHP_FE(fhread_join, 					arginfo_fhread_join)
	PHP_FE(fhread_kill,						NULL)
	PHP_FE(fhread_detach, 					NULL)
	PHP_FE(fhread_mutex_init, 				NULL)
	PHP_FE(fhread_mutex_lock, 				NULL)
	PHP_FE(fhread_mutex_unlock,				NULL)
	PHP_FE(fhread_mutex_destroy,			NULL)
	PHP_FE(fhread_cond_broadcast,			NULL)
	PHP_FE(fhread_cond_destroy,				NULL)
	PHP_FE(fhread_cond_init,				NULL)
	PHP_FE(fhread_cond_signal,				NULL)
	PHP_FE(fhread_cond_wait,				NULL)
	PHP_FE_END	/* Must be the last line in fhreads_functions[] */
}; /* }}} */

/* {{{ fhreads_module_entry */
zend_module_entry fhreads_module_entry = {
	STANDARD_MODULE_HEADER,
	"fhreads",
	fhreads_functions,
	PHP_MINIT(fhreads),
	PHP_MSHUTDOWN(fhreads),
	PHP_RINIT(fhreads),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(fhreads),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(fhreads),
	PHP_FHREADS_VERSION,
	NO_MODULE_GLOBALS,
	ZEND_MODULE_POST_ZEND_DEACTIVATE_N(fhreads),
	STANDARD_MODULE_PROPERTIES_EX
}; /* }}} */

#ifdef COMPILE_DL_FHREADS
ZEND_GET_MODULE(fhreads)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
