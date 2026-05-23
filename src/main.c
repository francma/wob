#define WOB_FILE "main.c"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "global_configuration.h"
#include "log.h"
#include "pledge.h"
#include "wob.h"

bool
wob_file_get_contents(char **contents, char *path)
{
	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		wob_log_error("Failed to open config file %s", path);
		return false;
	}

	int fd = fileno(fp);
	if (fd == -1) {
		perror("fileno");
		return false;
	}

	struct stat st;
	if (fstat(fd, &st) == -1) {
		perror("fstat");
		return false;
	}

	if (st.st_size > 1024 * 10) {
		wob_log_error("File too big");
		return false;
	}

	size_t file_size = (size_t) st.st_size;

	*contents = calloc(file_size + 1, sizeof(char));
	if (*contents == NULL) {
		return false;
	}

	size_t bytes_read = fread(*contents, 1, file_size, fp);
	if (bytes_read != file_size) {
		free(*contents);
		return false;
	}

	return true;
}

int
main(int argc, char **argv)
{
	wob_log_use_colors(isatty(STDERR_FILENO));
	wob_log_level_warn();

	// libc is doing fstat syscall to determine the optimal buffer size and that can be problematic to wob_pledge()
	// to solve this problem we can just pass the optimal buffer ourselves
	static char stdin_buffer[INPUT_BUFFER_LENGTH];
	if (setvbuf(stdin, stdin_buffer, _IOFBF, sizeof(stdin_buffer)) != 0) {
		wob_log_error("Failed to set stdin buffer size to %zu", sizeof(stdin_buffer));
		return EXIT_FAILURE;
	}
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	static struct option long_options[] = {
		{"config", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'V'},
		{"verbose", no_argument, NULL, 'v'},
		{0, 0, 0, 0},
	};

	const char *usage =
		"Usage: wob [options]\n"
		"  -c, --config <config>  Specify a config file.\n"
		"  -v, --verbose          Increase verbosity of messages, defaults to errors and warnings only.\n"
		"  -h, --help             Show help message and quit.\n"
		"  -V, --version          Show the version number and quit.\n"
		"\n";

	int c;
	int option_index = 0;
	char *wob_config_path = NULL;
	while ((c = getopt_long(argc, argv, "hvVc:", long_options, &option_index)) != -1) {
		switch (c) {
			case 'V':
				printf("wob version " WOB_VERSION "\n");
				free(wob_config_path);
				return EXIT_SUCCESS;
			case 'h':
				printf("%s", usage);
				free(wob_config_path);
				return EXIT_SUCCESS;
			case 'v':
				wob_log_inc_verbosity();
				break;
			case 'c':
				// fail if -c option is given multiple times
				if (wob_config_path != NULL) {
					free(wob_config_path);
					fprintf(stderr, "%s", usage);
					return EXIT_FAILURE;
				}

				free(wob_config_path);
				wob_config_path = strdup(optarg);
				break;
			default:
				fprintf(stderr, "%s", usage);
				free(wob_config_path);
				return EXIT_FAILURE;
		}
	}

	wob_log_info("wob version %s started with pid %jd", WOB_VERSION, (intmax_t) getpid());

	if (wob_config_path == NULL) {
		wob_config_path = wob_config_default_path();
	}

	char *raw_config_string = NULL;
	if (wob_config_path != NULL) {
		wob_log_info("Using configuration file at %s", wob_config_path);
		if (!wob_file_get_contents(&raw_config_string, wob_config_path)) {
			return false;
		}

		printf("%s\n", raw_config_string);
	}

	wob_connect();

	char *disable_pledge_env = getenv("WOB_DISABLE_PLEDGE");
	if (disable_pledge_env == NULL || strcmp(disable_pledge_env, "1") != 0) {
		wob_pledge();
	}

	struct wob_config *config = wob_config_create();
	if (raw_config_string != NULL) {
		if (!wob_config_load(config, raw_config_string)) {
			wob_config_destroy(config);
			free(wob_config_path);
			free(raw_config_string);
			return EXIT_FAILURE;
		}
	}

	wob_config_debug(config);
	free(wob_config_path);
	free(raw_config_string);

	return wob_run(config);
}
