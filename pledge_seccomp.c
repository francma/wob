#define WOB_FILE "pledge_seccomp.c"

#include <stdlib.h>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/signal.h>
#include <seccomp.h>
#include <sys/ptrace.h>

#include "log.h"
#include "pledge.h"

bool
wob_pledge(void)
{
	const int scmp_sc[] = {
		SCMP_SYS(clock_gettime),
		SCMP_SYS(close),
		SCMP_SYS(exit),
		SCMP_SYS(exit_group),
		SCMP_SYS(fcntl),
		SCMP_SYS(gettimeofday),
		SCMP_SYS(_llseek),
		SCMP_SYS(lseek),
		SCMP_SYS(munmap),
		SCMP_SYS(poll),
		SCMP_SYS(ppoll),
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
		wob_log_error("seccomp_init(SCMP_ACT_KILL) failed");
		return false;
	}

	for (size_t i = 0; i < sizeof(scmp_sc) / sizeof(int); ++i) {
		wob_log_debug("Adding syscall %d to whitelist", scmp_sc[i]);
		if ((ret = seccomp_rule_add(scmp_ctx, SCMP_ACT_ALLOW, scmp_sc[i], 0)) < 0) {
			wob_log_error("seccomp_rule_add(scmp_ctxm, SCMP_ACT_ALLOW, %d) failed with return value %d", scmp_sc[i], ret);
			seccomp_release(scmp_ctx);
			return false;
		}
	}

	if ((ret = seccomp_load(scmp_ctx)) < 0) {
		wob_log_error("seccomp_load(scmp_ctx) failed with return value %d", ret);
		seccomp_release(scmp_ctx);
		return false;
	}
	wob_log_debug("Seccomp syscall whitelist successfully installed");

	seccomp_release(scmp_ctx);

	return true;
}
