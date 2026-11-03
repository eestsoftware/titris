#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

int colors = 1, termColors = 1, showLineClear = 1, nextBoxes = 3, boardX = 11, boardY = 22, debugInfo = 0, startLevel = 1, whiteBorders = 0, zMagenta = 0, variableGoal = 0, hideOnPause = 1;

static const struct option long_options[] = {
    {"help"		, no_argument, 0,	'h'},
    {"level_start"	, required_argument, 0, 'l'},
    {"variable_goal"	, no_argument, 0,	'v'},
    {"next_boxes"	, required_argument, 0, 'n'},
    {"x_width"		, required_argument, 0, 'x'},
    {"y_height"		, required_argument, 0, 'y'},
    {"popup_score"	, no_argument, 0,	'p'},
    {"hide_on_pause"	, no_argument, 0,	'o'},
    {"solid_color"	, no_argument, 0,	's'},
    {"term_colors"	, no_argument, 0,	't'},
    {"white_borders"	, no_argument, 0,	'w'},
    {"z_magenta"	, no_argument, 0,	'z'},
    {"debug_info"	, no_argument, 0,	'd'},
    {0, 0, 0, 0}
};

struct desc_information {
    char* desc		;
    unsigned int is_opt	;
    char* arg_name	;
};

static const struct desc_information descs[] = {
    { "", 30, "OFFS" },\
    { "Show this menu.", 1, "" },\
    { "\n Gameplay Options:", 0, "" },\
    { "Level at which to start the game. AMT (1-15), default: 1.", 1, "<AMT>" },\
    { "Use a variable-goal system instead of the fixed-goal one. Where each level requires 5*(Level) lines to clear, instead of a fixed 10 lines per level.", 1, "" },\
    { "Amount of next preview windows to display. AMT (0-7), default: 3.", 1, "<AMT>" },\
    { "Width of the playing field. AMT(7-100), default: 10.", 1, " <AMT>" },\
    { "Height of the playing field.AMT(8-100), default: 22.", 1, "<AMT>" },\
    { "Disable the text that displays the last action that gave points.", 1, "" },\
    { "Dont hide the playing field when the game is paused.", 1, "" },\
    { " Color Options:", 0, "" },\
    { "Disable rendering tetrominos with solid colors, and instead render with text characters.", 1, "" },\
    { "Disable rendering tetrominos with the terminal color scheme, and use the default one instead (Based on GNU Emacs Tetris)", 1, "" },\
    { "Render borders with solid white. (Useful for displaying in a TTY)", 1, "" },\
    { "Render Z Tetromino as Magenta color (same as J) to render correctly in a TTY.", 1, "" },\
    { " Other:", 0, "" },\
    { "Show debug information on the side bar.", 1, "" },\
    { "", 10, "OFFS" },\
    { "\nControls:", 0, "" },\
    { "Rotate Clockwise",		2, "UP" },\
    { "Left",				2, "LEFT" },\
    { "Right",				2, "RIGHT" },\
    { "Soft Drop",			2, "DOWN" },\
    { "Hard Drop",			2, "SPACE" },\
    { "Hold",				2, "C" },\
    { "Rotate Counter-Clockwise",	2, "Z" },\
    { "  ", 2, "" },\
    { "Pause",				2, "P" },\
    { "Quit",				2, "Q" },\
    { "Restart",			2, "R" },\
    { "\nMade by Silenceman (eestsoftware@gmail.com)\nGithub Repo: https://github.com/eestsoftware/titris", 0, "NOCOL"},\
};

#define COLOR_HEADING   "\x1b[32m"
#define COLOR_OPTION    "\x1b[36m"
#define COLOR_ARG	"\x1b[34m"
#define COLOR_DESC	"\x1b[0m"
#define COLOR_RESET	"\x1b[0m"

#define MAX_LINE_LEN 70

static void print_help(const struct desc_information* opt_descs, int desc_length, char *prog_name, const struct option* options) {
    if (prog_name != NULL) {
    printf(COLOR_HEADING	"Usage:" COLOR_RESET\
    COLOR_OPTION		" %s" COLOR_RESET\
    COLOR_ARG			" [OPTIONS]" COLOR_RESET "\n\n"\
    COLOR_HEADING		"Options:" COLOR_RESET "\n"\
    , prog_name);
    }
    //dsc is seperate for description category formatting
    size_t whitespace_offset = 30;
    for (int i = 0, dsc = 0; dsc < desc_length; ++i, ++dsc) { 
	char *splitDesc = strdup(opt_descs[dsc].desc);
        if (strlen(splitDesc) <= 1) {
	    if (strncmp(opt_descs[dsc].arg_name, "OFFS", 5) == 0) whitespace_offset = opt_descs[dsc].is_opt;
	    --i;
	    continue;
	}
	size_t wspace = 10; // This is all the space and - characters in a default formatted opt
	if (opt_descs[dsc].is_opt == 0) {
	    printf("%s%s" COLOR_RESET "\n", strncmp(opt_descs[dsc].arg_name, "NOCOL", 6) == 0 ? COLOR_RESET : COLOR_HEADING, splitDesc);
	    --i;
	    free(splitDesc);
	    continue;
	} else if (opt_descs[dsc].is_opt == 2) {
	    printf(COLOR_OPTION "    %s" COLOR_RESET, opt_descs[dsc].arg_name);
	    wspace = 4 + strlen(opt_descs[dsc].arg_name);
	} else {
	    if (options == NULL) continue;
	    printf(COLOR_OPTION "   -%c, --%s" COLOR_RESET COLOR_ARG " %s" COLOR_RESET,
	    options[i].val,
	    options[i].name,
	    options[i].has_arg ? opt_descs[dsc].arg_name : "");
	    wspace += strlen(options[i].name);
	}
	for (; wspace < whitespace_offset - (opt_descs[dsc].is_opt == 1 ? strlen(opt_descs[dsc].arg_name) : 0); ++wspace) printf(" ");
	printf(COLOR_DESC);
	size_t dLen = strlen(splitDesc);
	int lineBreak = 0;
	for (size_t j = 0; dLen >= MAX_LINE_LEN && j < dLen; ++j, ++lineBreak) {
	    if (lineBreak < MAX_LINE_LEN || splitDesc[j] != ' ') continue;
	    splitDesc[j] = '\n';
	    lineBreak = 0;
	}// instead of padding the string by insreting, do this BS
	for (size_t j = 0; j < dLen; ++j) {
	    if (splitDesc[j] == '\n') {
		printf("\n");
		for (size_t wspace = 0; wspace < whitespace_offset; ++wspace) printf(" ");
		continue;
	    }
	    printf("%c", splitDesc[j]);
	}
	printf(COLOR_RESET "\n");
	free(splitDesc);
    }
}

int parseArgs(int argc, char *argv[]) {
   char opt;
    while ((opt = getopt_long(argc, argv, "stwzphdvol:n:x:y:", long_options, NULL)) != -1) {
	switch(opt) {
	case 's':
	    colors = 0;
	    break;
	case 't': 
	    termColors = 0;
	    break;
	case 'w':
	    whiteBorders = 1;
	    break;
	case 'z':
	    zMagenta = 1;
	    break;
	case 'p': 
	    showLineClear = 0;
	    break; 
	case 'o': 
	    hideOnPause = 0;
	    break; 
	case 'v':
	    variableGoal = 1;
	    break;
	case 'd': 
	    debugInfo = 1;
	    break; 
	case 'h':
	    print_help(descs, sizeof(descs) / sizeof(descs[0]), argv[0], long_options);
	    return 1;
	    break;
	case 'l':
	    startLevel = atoi(optarg);
	    if (startLevel >= 1 && startLevel <= 15) break;
	    fprintf(stderr, "Starting level AMT must be between a value between 1 and 15.\n");
	    return 1;
	    break;
	case 'n':
	    nextBoxes = atoi(optarg);
	    if (nextBoxes >= 0 && nextBoxes <= 7) break;
	    fprintf(stderr, "Next boxes AMT must be between a value between 0 and 7.\n");
	    return 1;
	    break;
	case 'x':
	    boardX = atoi(optarg) + 1;
	    if (boardX >= 7 && boardX <= 100) break;
	    fprintf(stderr, "Width must be a value between 7 and 100.\n");
	    return 1;
	    break;
	case 'y':
	    boardY = atoi(optarg);
	    if (boardY >= 8 && boardY <= 100) break;
	    fprintf(stderr, "Height must be a value between 8 and 100.\n");
	    return 1;
	    break;
	default:
	    fprintf(stderr, "Use --help for more information.\n");
	    return 1;
	    break;
	}
    }
    return 0;
}

