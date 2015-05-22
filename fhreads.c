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

/* True global resources - no need for thread safety here */
void ***fhread_global_tsrm_ls;

static int le_fhreads;
static zend_object_handlers fhreads_handlers;
static zend_object_handlers *fhreads_zend_handlers;
static HashTable fhreads_objects;

pthread_mutex_t fhread_global_mutex = PTHREAD_MUTEX_INITIALIZER;

zend_uint fhread_global_objects_store_top = 1;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("fhreads.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_fhreads_globals, fhreads_globals)
    STD_PHP_INI_ENTRY("fhreads.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_fhreads_globals, fhreads_globals)
PHP_INI_END()
*/
/* }}} */

void fhread_write_property(zval *object, zval *member, zval *value, const zend_literal *key TSRMLS_DC)
{
	zend_std_write_property(object, member, value, key TSRMLS_CC);
}

/* {{{ */
void fhreads_global_free(FHREAD *fhread) {
	// efree(&fhread);
} /* }}} */

/* {{{ */
static int fhread_opz_new_handler(ZEND_OPCODE_HANDLER_ARGS)
{
	// fhreaded objects to sync
	FHREAD *fhread;
	zend_mm_heap *old_mm_heap, *mm_heap;

	// lock global mutex
	pthread_mutex_lock(&fhread_global_mutex);

	// increment global object top counter
	FHREADS_EG(fhread_global_tsrm_ls, objects_store).top = fhread_global_objects_store_top++;

	if (fhread_global_objects_store_top == FHREADS_EG(fhread_global_tsrm_ls, objects_store).size) {

		FHREADS_EG(fhread_global_tsrm_ls, objects_store).size <<= 1;

		// set heap to be in global context scope
		mm_heap = zend_mm_set_heap(NULL, fhread_global_tsrm_ls);
		zend_mm_set_heap(mm_heap, fhread_global_tsrm_ls);
		old_mm_heap = zend_mm_set_heap(mm_heap TSRMLS_CC);

		// realloc new objects_bucket
		FHREADS_EG(fhread_global_tsrm_ls, objects_store).object_buckets = (zend_object_store_bucket *) erealloc(FHREADS_EG(fhread_global_tsrm_ls, objects_store).object_buckets, FHREADS_EG(fhread_global_tsrm_ls, objects_store).size * sizeof(zend_object_store_bucket));

		// reset heap to be back in current scope
		zend_mm_set_heap(old_mm_heap TSRMLS_CC);
	}

	// synchronize context fhread objects_store
	for (zend_hash_internal_pointer_reset(&fhreads_objects);
	     zend_hash_get_current_data(&fhreads_objects, (void **) &fhread) == SUCCESS;
	     zend_hash_move_forward(&fhreads_objects)
	) {
		FHREADS_EG(fhread->tsrm_ls, objects_store).free_list_head = FHREADS_EG(fhread_global_tsrm_ls, objects_store).free_list_head;
		FHREADS_EG(fhread->tsrm_ls, objects_store).top = fhread_global_objects_store_top;
		FHREADS_EG(fhread->tsrm_ls, objects_store).size = FHREADS_EG(fhread_global_tsrm_ls, objects_store).size;
		FHREADS_EG(fhread->tsrm_ls, objects_store).object_buckets = FHREADS_EG(fhread_global_tsrm_ls, objects_store).object_buckets;
	}

	// lock global mutex
	pthread_mutex_unlock(&fhread_global_mutex);

	// go on
	return ZEND_USER_OPCODE_DISPATCH;
}

/* {{{ Initialises the executor in fhreads context */
void fhread_init_executor(FHREAD *fhread TSRMLS_DC) /* {{{ */
{
	// check if executor was not inited before
	if (fhread->executor_inited == 0) {
		zend_init_fpu(TSRMLS_C);

		INIT_ZVAL(EG(uninitialized_zval));
		/* trick to make uninitialized_zval never be modified, passed by ref, etc. */
		Z_ADDREF(EG(uninitialized_zval));
		INIT_ZVAL(EG(error_zval));
		EG(uninitialized_zval_ptr)=&EG(uninitialized_zval);
		EG(error_zval_ptr)=&EG(error_zval);
/* destroys stack frame, therefore makes core dumps worthless */
#if 0&&ZEND_DEBUG
	original_sigsegv_handler = signal(SIGSEGV, zend_handle_sigsegv);
#endif

		EG(return_value_ptr_ptr) = NULL;
		EG(symtable_cache_ptr) = EG(symtable_cache) - 1;
		EG(symtable_cache_limit) = EG(symtable_cache) + SYMTABLE_CACHE_SIZE - 1;
		EG(no_extensions) = 0;
		EG(in_execution) = 0;
		EG(in_autoload) = NULL;
		EG(autoload_func) = NULL;
		EG(error_handling) = EH_NORMAL;

		// link functions
		EG(function_table) = FHREADS_CG(fhread->c_tsrm_ls, function_table);
		// link classes
		EG(class_table) = FHREADS_CG(fhread->c_tsrm_ls, class_table);
		// link constants
		EG(zend_constants) = FHREADS_EG(fhread->c_tsrm_ls, zend_constants);
		// link regular list
		EG(regular_list) = FHREADS_EG(fhread->c_tsrm_ls, regular_list);
		// link objects_store
		EG(objects_store) = FHREADS_EG(fhread->c_tsrm_ls, objects_store);

		zend_vm_stack_init(TSRMLS_C);
		zend_vm_stack_push((void *) NULL TSRMLS_CC);

		EG(ticks_count) = 0;
		EG(user_error_handler) = NULL;
		EG(current_execute_data) = NULL;

		EG(full_tables_cleanup) = 0;
	#ifdef ZEND_WIN32
		EG(timed_out) = 0;
	#endif

		EG(exception) = NULL;
		EG(prev_exception) = NULL;
		EG(scope) = NULL;
		EG(called_scope) = NULL;
		EG(This) = NULL;
		EG(active_op_array) = NULL;
		EG(active) = 1;
		EG(start_op) = NULL;

		// set executor inited flag
		fhread->executor_inited = 1;
	}

	// init executor globals for function run to execute
	EG(This) = (*fhread->runnable);
	EG(active_op_array) = (zend_op_array*) fhread->run;
	EG(scope) = Z_OBJCE_PP(fhread->runnable);
	EG(called_scope) = EG(scope);
	EG(in_execution) = 1;
}
/* }}} */

/* {{{ The routine to call for pthread_create */
void *fhread_routine (void *arg)
{
	// passed the object as argument
	FHREAD* fhread = (FHREAD *) arg;
	// init threadsafe manager local storage and create new context
	TSRMLS_D = fhread->tsrm_ls;
	// set interpreter context
	tsrm_set_interpreter_context(TSRMLS_C);
	// link server context
	SG(server_context) = FHREADS_SG(fhread->c_tsrm_ls, server_context);
	// init executor
	fhread_init_executor(fhread TSRMLS_CC);
	// exec run method
	zend_execute((zend_op_array*) fhread->run TSRMLS_CC);
	// exit thread
	pthread_exit(NULL);
#ifdef _WIN32
	return NULL; /* silence MSVC compiler */
#endif
} /* }}} */

/* {{{ proto fhread_tls_get_id()
	Obtain the identifier of currents ls context */
PHP_FUNCTION(fhread_tls_get_id)
{
	ZVAL_LONG(return_value, (long)TSRMLS_C);
} /* }}} */

/* {{{ proto fhread_mutex_init() */
PHP_FUNCTION(fhread_mutex_init)
{
	pthread_mutex_t *mutex;
	if ((mutex=(pthread_mutex_t*) calloc(1, sizeof(pthread_mutex_t)))!=NULL) {
		RETURN_LONG((ulong)mutex);
	}
} /* }}} */


/* {{{ proto fhread_mutex_lock() */
PHP_FUNCTION(fhread_mutex_lock)
{
	pthread_mutex_t *mutex;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &mutex)==SUCCESS && mutex) {
		pthread_mutex_lock(mutex);
	}
} /* }}} */


/* {{{ proto fhread_mutex_unlock() */
PHP_FUNCTION(fhread_mutex_unlock)
{
	pthread_mutex_t *mutex;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &mutex)==SUCCESS && mutex) {
		pthread_mutex_unlock(mutex);
	}
} /* }}} */

/* {{{ proto fhread_object_get_handle(object obj)
	Obtain the identifier of the given objects handle */
PHP_FUNCTION(fhread_object_get_handle)
{
	zval *obj;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &obj) == FAILURE) {
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

/* {{{ proto fhread_free_interpreter_context(long thread_id)
	frees an interpreter context created by a thread */
PHP_FUNCTION(fhread_free_interpreter_context)
{
	THREAD_T thread_id;
	int status;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &thread_id) == FAILURE) {
		RETURN_NULL();
	}

	//void ***f_tsrm_ls = (void ***) ts_resource_ex(0, &thread_id);
	//printf("fhread_free_interpreter_context: %d\n", f_tsrm_ls);
	// tsrm_free_interpreter_context(f_tsrm_ls);
} /* }}} */

/* {{{ proto fhread_create()
   Create a new thread, starting with execution of START-ROUTINE
   getting passed ARG.  Creation attributed come from ATTR.  The new
   handle is stored in thread_id which will be returned to php userland. */
PHP_FUNCTION(fhread_create)
{
	char *gid;
	int gid_len;
	int status;
	FHREAD* fhread;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &gid, &gid_len) == FAILURE) {
		RETURN_NULL();
	}

	// check if fhread was alreade prepared for gid
	if (zend_hash_find(&fhreads_objects, gid, gid_len + 1, (void **)&fhread)  == FAILURE) {
		// prepare fhread
		fhread = (FHREAD *)malloc(sizeof(FHREAD));
		// init executor flag
		fhread->executor_inited = 0;
		// set creator tsrm ls
		fhread->c_tsrm_ls = fhread_global_tsrm_ls;
		// create new tsrm ls for thread routine
		fhread->tsrm_ls = tsrm_new_interpreter_context();
		// add to globals hash
		zend_hash_add(&fhreads_objects, gid, gid_len + 1, (void*)fhread, sizeof(FHREAD), NULL);
	}

	// get runnable zval from creator symbol table
	zend_hash_find(&EG(symbol_table), gid, gid_len + 1, (void**)&fhread->runnable);
	// inject fhread handlers for runnable zval
	Z_OBJ_HT_PP(fhread->runnable) = &fhreads_handlers;
	// get run function from function table
	zend_hash_find(&Z_OBJCE_PP(fhread->runnable)->function_table, "run", sizeof("run"), (void**)&fhread->run);

	// create thread and start fhread__routine
	status = pthread_create(&fhread->thread_id, NULL, fhread_routine, fhread);

	// return thread id
	RETURN_LONG((long)fhread->thread_id);
} /* }}} */

/* {{{ proto fhread_join()
   Make calling thread wait for termination of the given thread id.  The
   exit status of the thread is stored in *THREAD_RETURN, if THREAD_RETURN
   is not NULL.*/
PHP_FUNCTION(fhread_join)
{
	long thread_id;
	int status;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &thread_id) == FAILURE) {
		RETURN_NULL();
	}

	// join thread with given id
	status = pthread_join((pthread_t)thread_id, NULL);

	RETURN_LONG((long)status);
} /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(fhreads)
{
	fhread_global_tsrm_ls = TSRMLS_C;

	zend_set_user_opcode_handler(ZEND_NEW, fhread_opz_new_handler);

	// Setup standard and fhreads object handlers
	fhreads_zend_handlers = zend_get_std_object_handlers();
	memcpy(&fhreads_handlers, fhreads_zend_handlers, sizeof(zend_object_handlers));

	// override object handlers
	fhreads_handlers.write_property = fhread_write_property;
	fhreads_handlers.get = NULL;
	fhreads_handlers.set = NULL;
	fhreads_handlers.get_property_ptr_ptr = NULL;

#if PHP_VERSION_ID > 50399
	/* when the gc runs, it will fetch properties, every time */
	/* so we pass in a dummy function to control memory usage */
	/* properties copied will be destroyed with the object */
	fhreads_handlers.get_gc = NULL;
#endif

	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/

	ZEND_INIT_MODULE_GLOBALS(fhreads, NULL, NULL);

	zend_hash_init(&FHREADS_G(fhreads), 64, NULL, (dtor_func_t) fhreads_global_free, 1);
	zend_hash_init(&fhreads_objects, 64, NULL, (dtor_func_t) fhreads_global_free, 1);

	return SUCCESS;
} /* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(fhreads)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/

	zend_hash_destroy(&FHREADS_G(fhreads));

	return SUCCESS;
} /* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(fhreads)
{
	return SUCCESS;
} /* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(fhreads)
{
	return SUCCESS;
} /* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(fhreads)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "fhreads support", "enabled");
	php_info_print_table_row(2, "fhreads version", PHP_FHREADS_VERSION);
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
} /* }}} */

/* {{{ fhreads_functions[]
 *
 * Every user visible function must have an entry in fhreads_functions[].
 */
const zend_function_entry fhreads_functions[] = {
	PHP_FE(fhread_tls_get_id, 				NULL)
	PHP_FE(fhread_object_get_handle, 		NULL)
	PHP_FE(fhread_free_interpreter_context, NULL)
	PHP_FE(fhread_self, 					NULL)
	PHP_FE(fhread_create, 					NULL)
	PHP_FE(fhread_join, 					NULL)
	PHP_FE(fhread_mutex_init, 				NULL)
	PHP_FE(fhread_mutex_lock, 				NULL)
	PHP_FE(fhread_mutex_unlock,				NULL)
	PHP_FE_END	/* Must be the last line in fhreads_functions[] */
}; /* }}} */

/* {{{ fhreads_module_entry
 */
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
	STANDARD_MODULE_PROPERTIES
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
