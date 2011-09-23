PHP_ARG_ENABLE(shm_table, whether to enable shared memory table manipulation,
[ --enable-shm_table   Enable Shared memory table manipulation])

if test "$PHP_SHM_TABLE" = "yes"; then
  AC_DEFINE(HAVE_SHM_TABLE, 1, [Whether you have Shared Memory Table])
  PHP_NEW_EXTENSION(shm_table, shm_table.c, $ext_shared)
fi