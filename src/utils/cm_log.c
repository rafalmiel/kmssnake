#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cm_log.h"

#include "cm_hashtable.h"

struct log_config {
	int level;
	struct cm_hashtable *subsystem_level;
	int has_subsystem_conf;
};

static struct option long_options[] =
	{
		{"log-level",	required_argument,	0, 0},
		{"log-backend",	required_argument,	0, 0},
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
		optret = getopt_long(argc, argv, "", long_options, &option_index);

		if (optret == 0) {
			if (option_index == 0) {
				conf->level = atoi(optarg);
			} else if (option_index == 1) {
				parse_subsystem_levels(conf, optarg);
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
	int lev = 5;

	if (!log_gconf) return;

	lev = log_gconf->level;

	if (log_gconf->has_subsystem_conf) {
		lvl = cm_hashtable_lookup(log_gconf->subsystem_level, subsystem);
		if (lvl != NULL) {
			lev = *lvl;
		}
	}

	if (lev >= level) {
		fprintf(stderr, "%s: ", subsystem);

		va_start(list, format);
		vfprintf(stderr, format, list);
		va_end(list);

		fprintf(stderr, "\n", subsystem);
	}
}
