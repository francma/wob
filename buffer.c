#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "buffer.h"

bool
wob_create_argb_buffer(const size_t size, int *shmid, uint32_t **buffer)
{
	int shmid_local = -1;
	char shm_name[NAME_MAX];
	for (unsigned char i = 0; i < UCHAR_MAX; ++i) {
		if (snprintf(shm_name, NAME_MAX, "/wob-%hhu", i) >= NAME_MAX) {
			break;
		}
		shmid_local = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (shmid_local > 0 || errno != EEXIST) {
			break;
		}
	}

	if (shmid_local < 0) {
		perror("shm_open");
		return false;
	}

	if (shm_unlink(shm_name) != 0) {
		perror("shm_unlink");
		return false;
	}

	if (ftruncate(shmid_local, size) != 0) {
		perror("ftruncate");
		return false;
	}

	void *buffer_local = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmid_local, 0);
	if (buffer_local == MAP_FAILED) {
		perror("mmap");
		return false;
	}

	*buffer = (uint32_t *) buffer_local;
	*shmid = shmid_local;

	return true;
}
