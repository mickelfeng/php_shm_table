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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define ERR_ENT_EXIST 1
#define ERR_OUT_OF_TABLE 2
#define ERR_NO_FREE_SPECE 3
#define ERR_NOT_FOUND 4
#define ERR_SYSTEM 5

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

void mfat_init_head(void * mem, uint32_t table_size, uint32_t mem_size, uint32_t seg_size) {
	bzero(mem, mem_size);
	uint32_t i;
	uint32_t * u32_mem = (uint32_t *) mem;
	u32_mem[0] = table_size;
	u32_mem[1] = mem_size;
	u32_mem[2] = seg_size;
	u32_mem[3] = 0; // entries count
	u32_mem[4] = 0; // seg used
	u32_mem[5] = 0; // real used
	for(i = 1; i <= table_size; i ++) {
		u32_mem[(i+1) * 3] = i;
		u32_mem[(i+1) * 3 + 1] = 0;
		u32_mem[(i+1) * 3 + 2] = 0;
	}
}

uint32_t * mfat_get_free_segs(void *mem, uint32_t count) {
	uint32_t *result = malloc(sizeof(uint32_t *) * count);
	uint32_t *u32_mem = (uint32_t *) mem;
	uint32_t offset = (u32_mem[0] + 2) * 3 * sizeof(uint32_t);
	void *data_mem;
	uint32_t founded_segs = 0;
	uint32_t counter = 1;
	while(offset < u32_mem[1]) {
		data_mem = mem + offset;
		uint32_t seg_flag = * (uint32_t *) (data_mem + u32_mem[2] - sizeof(uint32_t));
		if (seg_flag == 0) {
			result[founded_segs] = counter;
			if (++founded_segs == count) {
				return result;
			}
		}
		offset += u32_mem[2];
		counter ++;
	}
	free(result);
	return NULL;
}

uint32_t mfat_create_entry(void *mem, uint32_t id, void *data, uint32_t data_size) {
	uint32_t * u32_mem = (uint32_t *) mem;
	if (id > u32_mem[0]) {
		return ERR_OUT_OF_TABLE;
	}
	uint32_t * u32_table = mem + ((id + 1) * 3 * sizeof(uint32_t));
	if (u32_table[1] != 0) {
		return ERR_ENT_EXIST;
	}
	uint32_t real_seg_size = u32_mem[2] - sizeof(uint32_t);
	uint32_t seg_count = (data_size + (real_seg_size - 1)) / real_seg_size;
	uint32_t *free_segs = mfat_get_free_segs(mem, seg_count);
	if (free_segs == NULL) {
		return ERR_NO_FREE_SPECE;
	}
	u32_table[1] = free_segs[0];
	u32_table[2] = data_size;

	uint32_t i;
	uint32_t offset = (u32_mem[0] + 2) * 3 * sizeof(uint32_t);
	uint32_t need_to_write = data_size;
	for (i = 0; i < seg_count; i ++) {
		void *data_seg = mem + offset + (free_segs[i] - 1) * u32_mem[2];
		uint32_t * seg_flag = (uint32_t *) (data_seg + real_seg_size);
		if (i + 1 == seg_count) {
			* seg_flag = -1;
		} else {
			* seg_flag = free_segs[i + 1];
		}
		if (need_to_write > real_seg_size) {
			memcpy(data_seg, data + (i * real_seg_size), real_seg_size);
			need_to_write -= real_seg_size;
		} else {
			memcpy(data_seg, data + (i * real_seg_size), need_to_write);
			need_to_write = 0;
		}
	}
	u32_mem[3] += 1;
	u32_mem[4] += seg_count;
	u32_mem[5] += data_size;
	return EXIT_SUCCESS;
}

uint32_t mfat_read_entry(void * mem, uint32_t id, void **ret_data, uint32_t *data_size) {
	uint32_t * u32_mem = (uint32_t *) mem;
	if (id > u32_mem[0]) {
		return ERR_OUT_OF_TABLE;
	}
	uint32_t * u32_table = mem + ((id+1) * 3 * sizeof(uint32_t));
	if (u32_table[1] == 0) {
		return ERR_NOT_FOUND;
	}
	uint32_t ds = * data_size = u32_table[2];
	void * data = * ret_data = emalloc(ds);
	uint32_t need_to_read = ds;
	uint32_t real_seg_size = u32_mem[2] - sizeof(uint32_t);
	uint32_t offset = (u32_mem[0] + 2) * 3 * sizeof(uint32_t);
	void * seg_data = mem + offset + (u32_table[1] - 1) * u32_mem[2];
	while(need_to_read) {
		uint32_t seg_flag = * (uint32_t *) (seg_data + real_seg_size);
		if (seg_flag == 0) {
			free(* ret_data);
			* ret_data = 0;
			return ERR_SYSTEM;
		}
		if (seg_flag == -1) {
			if (need_to_read > real_seg_size) {
				free(* ret_data);
				* ret_data = 0;
				return ERR_SYSTEM;
			}
			memcpy(data, seg_data, need_to_read);
			need_to_read = 0;
		} else {
			memcpy(data, seg_data, real_seg_size);
			seg_data = mem + offset + (seg_flag - 1) * u32_mem[2];
			need_to_read -= real_seg_size;
			data += real_seg_size;
		}
	}
	return EXIT_SUCCESS;
}

uint32_t mfat_read_to_stream(void * mem, uint32_t id, FILE *stream) {
	uint32_t * u32_mem = (uint32_t *) mem;
	if (id > u32_mem[0]) {
		return ERR_OUT_OF_TABLE;
	}
	uint32_t * u32_table = mem + ((id + 1) * 3 * sizeof(uint32_t));
	if (u32_table[1] == 0) {
		return ERR_NOT_FOUND;
	}
	uint32_t need_to_read = u32_table[2];
	uint32_t real_seg_size = u32_mem[2] - sizeof(uint32_t);
	uint32_t offset = (u32_mem[0] + 2) * 3 * sizeof(uint32_t);
	void * seg_data = mem + offset + (u32_table[1] - 1) * u32_mem[2];
	while(need_to_read) {
		uint32_t seg_flag = * (uint32_t *) (seg_data + real_seg_size);
		if (seg_flag == 0) {
			return ERR_SYSTEM;
		}
		if (seg_flag == -1) {
			if (need_to_read > real_seg_size) {
				return ERR_SYSTEM;
			}
			fwrite(seg_data, need_to_read, 1, stream);
			need_to_read = 0;
		} else {
			fwrite(seg_data, real_seg_size, 1, stream);
			seg_data = mem + offset + (seg_flag - 1) * u32_mem[2];
			need_to_read -= real_seg_size;
		}
	}
	return EXIT_SUCCESS;
}

uint32_t mfat_remove(void * mem, uint32_t id) {
	uint32_t * u32_mem = (uint32_t *) mem;
	if (id > u32_mem[0]) {
		return ERR_OUT_OF_TABLE;
	}
	uint32_t * u32_table = mem + ((id + 1) * 3 * sizeof(uint32_t));
	if (u32_table[1] == 0) {
		return ERR_NOT_FOUND;
	}
	uint32_t real_seg_size = u32_mem[2] - sizeof(uint32_t);
	uint32_t offset = (u32_mem[0] + 2) * 3 * sizeof(uint32_t);
	void * seg_data = mem + offset + (u32_table[1] - 1) * u32_mem[2];
	uint32_t work = 1;
	while(work) {
		uint32_t * seg_flag = (uint32_t *) (seg_data + real_seg_size);
		if (!*seg_flag) {
			return ERR_SYSTEM;
		}
		if (*seg_flag == -1) {
			*seg_flag = 0;
			work = 0;
		} else {
			seg_data = mem + offset + (*seg_flag - 1) * u32_mem[2];
			*seg_flag = 0;
		}
	}
	u32_table[1] = 0;
	u32_mem[3] -= 1;
	u32_mem[4] -= (u32_table[2] + (real_seg_size - 1)) / real_seg_size;
	u32_mem[5] -= u32_table[2];
	return EXIT_SUCCESS;
}

int le_shm_table;

static function_entry shm_table_functions[] = {
    PHP_FE(shm_table_open, NULL)
    PHP_FE(shm_table_get, NULL)
    PHP_FE(shm_table_print, NULL)
    PHP_FE(shm_table_remove, NULL)
    PHP_FE(shm_table_create, NULL)
    PHP_FE(shm_table_set, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry shm_table_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_SHM_TABLE_EXTNAME,
    shm_table_functions,
    PHP_MINIT(shm_table),
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

static void php_shm_table_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	php_shm_table *table = (php_shm_table*)rsrc->ptr;
	if (table) {
		efree(table);
	}
}

PHP_MINIT_FUNCTION(shm_table)
{
	le_shm_table = zend_register_list_destructors_ex(php_shm_table_dtor, NULL, PHP_SHM_TABLE_RES_NAME, module_number);
	return SUCCESS;
}

PHP_FUNCTION(shm_table_open)
{
	key_t key;
	php_shm_table *shared_table;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &key) == FAILURE) {
		php_printf("parse error");
		RETURN_FALSE;
	}

	int shmid;
	if ((shmid = shmget(key, 6 * sizeof(uint32_t), 0666)) < 0) {
		php_printf("shmget");
		RETURN_FALSE;
	}

	void * mem;
	if ((mem = shmat(shmid, NULL, 0)) == (void *) -1) {
		php_printf("shmat");
		RETURN_FALSE;
	}

	uint32_t * u32_mem = (uint32_t *) mem;

	shared_table = emalloc(sizeof(php_shm_table));
	shared_table->table_size = u32_mem[0];
	shared_table->mem_size = u32_mem[1];
	shared_table->seg_size = u32_mem[2];

	if ((shmid = shmget(key, shared_table->mem_size, 0666)) < 0) {
		php_printf("shmget");
		RETURN_FALSE;
	}

	if ((mem = shmat(shmid, NULL, 0)) == (void *) -1) {
		php_printf("shmat");
		RETURN_FALSE;
	}

	shared_table->key = key;
	shared_table->shmid = shmid;
	shared_table->mem = mem;

	ZEND_REGISTER_RESOURCE(return_value, shared_table, le_shm_table);
}

PHP_FUNCTION(shm_table_get)
{
	zval *zshm_table;
	php_shm_table *shared_table;
	long id;
	long ret_err;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zshm_table, &id) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(shared_table, php_shm_table*, &zshm_table, -1, PHP_SHM_TABLE_RES_NAME, le_shm_table);
	char * data;
	int data_size;
	if ((ret_err = mfat_read_entry(shared_table->mem, id, (void **) &data, &data_size)) != 0) {
		RETURN_LONG(ret_err);
	}
	RETURN_STRING(data, data_size);
}

PHP_FUNCTION(shm_table_print)
{
	zval *zshm_table;
	php_shm_table *shared_table;
	long id;
	long ret_err;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zshm_table, &id) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(shared_table, php_shm_table*, &zshm_table, -1, PHP_SHM_TABLE_RES_NAME, le_shm_table);
	char * data;
	int data_size;
	if ((ret_err = mfat_read_entry(shared_table->mem, id, (void **) &data, &data_size)) != 0) {
		RETURN_LONG(ret_err);
	}
	PHPWRITE(data, data_size);
	RETURN_TRUE;
}

PHP_FUNCTION(shm_table_remove)
{
	zval *zshm_table;
	php_shm_table *shared_table;
	long id;
	long ret_err;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zshm_table, &id) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(shared_table, php_shm_table*, &zshm_table, -1, PHP_SHM_TABLE_RES_NAME, le_shm_table);
	if ((ret_err = mfat_remove(shared_table->mem, id)) != 0) {
		RETURN_LONG(ret_err);
	}
	RETURN_TRUE;
}

PHP_FUNCTION(shm_table_create)
{
	key_t key;
	long table_size;
	long mem_size;
	long seg_size;
	php_shm_table *shared_table;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llll", &key, &table_size, &mem_size, &seg_size) == FAILURE) {
		RETURN_FALSE;
	}

	int shmid;
	if ((shmid = shmget(key, mem_size, IPC_CREAT | 0666)) < 0) {
		php_printf("shmget");
		RETURN_FALSE;
	}

	void * mem;
	if ((mem = shmat(shmid, NULL, 0)) == (void *) -1) {
		php_printf("shmat");
		RETURN_FALSE;
	}

	mfat_init_head(mem, table_size, mem_size, seg_size);

	shared_table = emalloc(sizeof(php_shm_table));
	shared_table->table_size = table_size;
	shared_table->mem_size = mem_size;
	shared_table->seg_size = seg_size;
	shared_table->key = key;
	shared_table->shmid = shmid;
	shared_table->mem = mem;

	ZEND_REGISTER_RESOURCE(return_value, shared_table, le_shm_table);
}

PHP_FUNCTION(shm_table_set)
{
	zval *zshm_table;
	php_shm_table *shared_table;
	char * data;
	long data_size;
	long id;
	long ret_err;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rls", &zshm_table, &id, &data, &data_size) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(shared_table, php_shm_table*, &zshm_table, -1, PHP_SHM_TABLE_RES_NAME, le_shm_table);
	if ((ret_err = mfat_create_entry(shared_table->mem, id, data, data_size)) != 0) {
		RETURN_LONG(ret_err);
	}
	RETURN_TRUE;
}



