#ifndef _WOB_BUFFER_H
#define _WOB_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

bool wob_create_argb_buffer(const size_t size, int *shmid, uint32_t **buffer);

#endif