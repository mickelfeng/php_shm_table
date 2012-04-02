/*
 * php_shm_table.h
 *
 *  Created on: Sep 22, 2011
 *      Author: valery
 */

#ifndef PHP_SHM_TABLE_H
#define PHP_SHM_TABLE_H 1

#define PHP_SHM_TABLE_VERSION "1.0"
#define PHP_SHM_TABLE_EXTNAME "shm_table"

PHP_FUNCTION(shm_table_open);
PHP_FUNCTION(shm_table_get);
PHP_FUNCTION(shm_table_print);
PHP_FUNCTION(shm_table_remove);
PHP_FUNCTION(shm_table_create);
PHP_FUNCTION(shm_table_set);

typedef struct _php_shm_table {
	char *path;
	key_t key;
	int shmid;
	void *mem;
	unsigned int table_size;
	unsigned int mem_size;
	unsigned int seg_size;
} php_shm_table;

#define PHP_SHM_TABLE_RES_NAME "Shared table"

PHP_MINIT_FUNCTION(shm_table);

extern zend_module_entry shm_table_module_entry;
#define phpext_shm_table_ptr &shm_table_module_entry

#endif /* PHP_SHM_TABLE_H_ */
