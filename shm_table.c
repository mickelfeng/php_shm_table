/*
 * shm_table.c
 *
 *  Created on: Sep 22, 2011
 *      Author: valery
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_shm_table.h"
#include <sys/ipc.h>
#include <sys/shm.h>

static function_entry shm_table_functions[] = {
    PHP_FE(shm_test, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry shm_table_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_SHM_TABLE_EXTNAME,
    shm_table_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_SHM_TABLE_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SHM_TABLE
ZEND_GET_MODULE(shm_table)
#endif

PHP_FUNCTION(shm_test)
{
	char* key;
	int key_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
		RETURN_NULL();
	}
	if(fopen((const char *) key, "r") != NULL) {
		RETURN_BOOL(1);
	} else {
		RETURN_BOOL(0);
	}
	//RETURN_NULL();
	//RETURN_STRING(sprintf("we try connect to %s shared memory", key), 1);

}

