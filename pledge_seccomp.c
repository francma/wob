#include <stdio.h>
#include <stdlib.h>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/signal.h>
#include <seccomp.h>
#include <sys/ptrace.h>

#include "pledge.h"

bool
wob_pledge(void)
{
	const int scmp_sc[] = {
		SCMP_SYS(close),
		SCMP_SYS(exit),
		SCMP_SYS(exit_group),
		SCMP_SYS(fcntl),
		SCMP_SYS(fstat),
		SCMP_SYS(poll),
		SCMP_SYS(read),
		SCMP_SYS(readv),
		SCMP_SYS(recvmsg),
		SCMP_SYS(restart_syscall),
		SCMP_SYS(sendmsg),
		SCMP_SYS(write),
		SCMP_SYS(writev),
	};

	int ret;
	scmp_filter_ctx scmp_ctx = seccomp_init(SCMP_ACT_KILL);
	if (scmp_ctx == NULL) {
		fprintf(stderr, "seccomp_init(SCMP_ACT_KILL) failed\n");
		return false;
	}

	for (size_t i = 0; i < sizeof(scmp_sc) / sizeof(int); ++i) {
		if ((ret = seccomp_rule_add(scmp_ctx, SCMP_ACT_ALLOW, scmp_sc[i], 0)) < 0) {
			fprintf(stderr, "seccomp_rule_add(scmp_ctxm, SCMP_ACT_ALLOW, %d) failed with return value %d\n", scmp_sc[i], ret);
			seccomp_release(scmp_ctx);
			return false;
		}
	}

	if ((ret = seccomp_load(scmp_ctx)) < 0) {
		fprintf(stderr, "seccomp_load(scmp_ctx) failed with return value %d\n", ret);
		seccomp_release(scmp_ctx);
		return false;
	}

	seccomp_release(scmp_ctx);

	return true;
}