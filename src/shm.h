#ifndef _WOB_SHM_H
#define _WOB_SHM_H

#include <stddef.h>
#include <stdint.h>

int wob_shm_open();

void *wob_shm_allocate(int shmid, size_t size);

#endif
