#ifndef _WOB_BUFFER_H
#define _WOB_BUFFER_H

#include <stddef.h>

int wob_shm_create();

void *wob_shm_alloc(int shmid, size_t size);

#endif
