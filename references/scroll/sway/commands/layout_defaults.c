#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include "sway/commands.h"

struct cmd_results *cmd_layout_default_height(int argc, char **argv) {
	struct cmd_results *error;
	if ((error = checkarg(argc, "layout_default_height", EXPECTED_AT_LEAST, 1))) {
		return error;
	}
	config->layout_default_height = strtod(argv[0], NULL);
	return cmd_results_new(CMD_SUCCESS, NULL);
}

struct cmd_results *cmd_layout_default_width(int argc, char **argv) {
	struct cmd_results *error;
	if ((error = checkarg(argc, "layout_default_width", EXPECTED_AT_LEAST, 1))) {
		return error;
	}
	config->layout_default_width = strtod(argv[0], NULL);
	return cmd_results_new(CMD_SUCCESS, NULL);
}

struct cmd_results *cmd_layout_heights(int argc, char **argv) {
	struct cmd_results *error;
	if ((error = checkarg(argc, "layout_heights", EXPECTED_AT_LEAST, 1))) {
		return error;
	}
	list_t *list = parse_double_array(argv[0]);
	if (list) {
		list_free_items_and_destroy(config->layout_heights);
		config->layout_heights = list;
		return cmd_results_new(CMD_SUCCESS, NULL);
	} else {
		return cmd_results_new(CMD_FAILURE, "Error parsing layout_heights array");
	}
}

struct cmd_results *cmd_layout_widths(int argc, char **argv) {
	struct cmd_results *error;
	if ((error = checkarg(argc, "layout_widths", EXPECTED_AT_LEAST, 1))) {
		return error;
	}
	list_t *list = parse_double_array(argv[0]);
	if (list) {
		list_free_items_and_destroy(config->layout_widths);
		config->layout_widths = list;
		return cmd_results_new(CMD_SUCCESS, NULL);
	} else {
		return cmd_results_new(CMD_FAILURE, "Error parsing layout_widths array");
	}
}
