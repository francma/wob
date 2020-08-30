#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
   How to run: test-poll-pollhup (-d) | wob

   What happens: 0, 10, 20, ... 100 are emitted 1s apart and then the program
   exits. If -d is set, the pipe will be hang up with data: the last value (100)

   What this tests: If wob can handle the pipe POLLHUP event. This is emitted
   when the program that got it's stdout redirected to wob exits or the named
   pipe that wob listened was deleted, or anything like this.
*/
int
main(int argc, char **argv)
{
	static struct option long_options[] = {
		{"hangup-with-data", no_argument, NULL, 'd'},
	};
	bool hangup_with_data = false;
	int option_index = 0;
	int c;
	while ((c = getopt_long(argc, argv, "d", long_options, &option_index)) != -1) {
		switch (c) {
			case 'd':
				hangup_with_data = true;
				break;
			default:
				fprintf(stderr, "Unknown argument");
				return EXIT_FAILURE;
		}
	}
	unsigned int event_time = 1;

	int event_count = 11;
	int event_value = 10;
	for (int i = 0; i < event_count; i++) {
		if (hangup_with_data) {
			fflush(stdout);
		}
		sleep(event_time);
		int event = i * event_value;

		printf("%i\n", event);
		if (!hangup_with_data) {
			fflush(stdout);
		}
	}
	sleep(event_time);
	return 0;
}