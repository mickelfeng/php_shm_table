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

PHP_FUNCTION(shm_test);

extern zend_module_entry shm_table_module_entry;
#define phpext_shm_table_ptr &shm_table_module_entry

#endif /* PHP_SHM_TABLE_H_ */
