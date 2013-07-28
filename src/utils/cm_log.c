#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cm_log.h"

#include "cm_hashtable.h"

const char *LOG_SUBSYSTEM = NULL;

static const char *log_sev2str[] = {
	[LOG_TRACE] = "TRACE",
	[LOG_DEBUG] = "DEBUG",
	[LOG_INFO] = "INFO",
	[LOG_WARN] = "WARN",
	[LOG_ERROR] = "ERROR",
	[LOG_FATAL] = "FATAL"
};

struct log_config {
	int level;
	struct cm_hashtable *subsystem_level;
	int has_subsystem_conf;
};

static struct option long_options[] =
	{
		{"log-level",	required_argument,	0, 0},
		{"log-backend",	required_argument,	0, 0},
		{"help",	no_argument,		0, 0},
		{0, 0, 0, 0}
	};

static struct log_config *log_gconf = NULL;

static void
parse_subsystem_levels(struct log_config *log_conf, char *subsystem_conf)
{
	char *tok = NULL;
	char *sub = NULL;
	char *lvl = NULL;
	char *s1, *s2;
	int *lvlint;

	tok = strtok_r(subsystem_conf, ",", &s1);
	while (tok != NULL) {
		sub = strtok_r(tok, "=", &s2);
		lvl = strtok_r(NULL, "=", &s2);

		lvlint = malloc(sizeof *lvlint);
		*lvlint = atoi(lvl);

		cm_hashtable_insert(log_conf->subsystem_level, sub, lvlint);

		tok = strtok_r(NULL, ",", &s1);
	}

	log_conf->has_subsystem_conf = (cm_hashtable_size(log_conf->subsystem_level) > 0);
}

static void
print_help_and_exit(void)
{
	printf("Allowed options: %s [options]\n", "kmssnake");
	printf("	-l --log-level <level>: log level (1-6)\n");
	printf("	-b --log-backend <confstring>: backend log level.\n");
	printf("		Override log-level setting for the particular backend.\n");
	printf("		confstring is in the form backend=level,backend2=level\n");
	printf("		eg. --log-backend=main=3,drm=6\n");
	printf("	-h --help: this message\n");
	exit(0);
}

CM_EXPORT int
log_configure(int argc, char *argv[])
{
	struct log_config *conf;
	int option_index = 0, optret;

	conf = malloc(sizeof *conf);

	if (!conf) {
		return -ENOMEM;
	}

	conf->level = LOG_DEBUG;
	conf->has_subsystem_conf = 0;
	conf->subsystem_level = cm_hashtable_create(cm_str_hash, cm_str_equal);

	while (1) {
		optret = getopt_long(argc, argv, "l:b:h", long_options, &option_index);

		if (optret == 0) {
			switch (option_index) {
			case 0:
				conf->level = atoi(optarg);
				break;
			case 1:
				parse_subsystem_levels(conf, optarg);
				break;
			case 2:
				print_help_and_exit();
				break;
			}
		} else if (optret > 0){
			switch (optret) {
			case 'l':
				conf->level = atoi(optarg);
				break;
			case 'b':
				parse_subsystem_levels(conf, optarg);
				break;
			case 'h':
				print_help_and_exit();
				break;
			}
		} else {
			break;
		}
	}

	log_gconf = conf;
}

CM_EXPORT void
log_write(int level, const char *subsystem,
	  const char *format, ...)
{
	int *lvl;
	va_list list;
	int lev = LOG_TRACE;

	if (!log_gconf) return;

	lev = log_gconf->level;

	if (log_gconf->has_subsystem_conf) {
		lvl = cm_hashtable_lookup(log_gconf->subsystem_level, subsystem);
		if (lvl != NULL) {
			lev = *lvl;
		}
	}

	if (lev >= level) {
		fprintf(stderr, "[%s] %s: ", log_sev2str[level], subsystem);

		va_start(list, format);
		vfprintf(stderr, format, list);
		va_end(list);

		fprintf(stderr, "\n", subsystem);
	}
}
