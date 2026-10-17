#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

int colors = 1, termColors = 1, showLineClear = 1, nextBoxes = 3, boardX = 11, boardY = 22, debugInfo = 0, startLevel = 1, whiteBorders = 0, zMagenta = 0;

#define ARG_AMT 9

static const struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"solid_color", no_argument, 0, 's'},
    {"term_colors", no_argument, 0, 't'},
    {"white_borders", no_argument, 0, 'w'},
    {"z_magenta", no_argument, 0, 'z'},
    {"popup_score", no_argument, 0, 'p'},
    {"level_start", required_argument, 0, 'l'},
    {"next_boxes", required_argument, 0, 'n'},
    {"x_width", required_argument, 0, 'x'},
    {"y_height", required_argument, 0, 'y'},
    {"debug_info", no_argument, 0, 'd'},
    {0, 0, 0, 0}
};


const char* descs[] = { "Show this menu.",\
"Disable rendering tetrominos with solid colors, and instead render with text characters.",\
"Disable rendering tetrominos with the terminal color scheme, and use the default one instead (Based on GNU Emacs Tetris)",\
"Render borders with solid white. (Useful for displaying in a TTY)",\
"Render Z Tetromino as Magenta color (same as J) to render correctly in a TTY.",\
"Disable the text that displays the last action that gave points.",\
"Level at which to start the game. AMT (1-15), default: 1.",\
"Amount of next preview windows to display. AMT (0-7), default: 3.",\
"Width of the playing field. AMT(7-100), default: 10.",\
"Height of the playing field. AMT(8-100), default: 22.",\
"Show debug information on the side bar.",\
"",};

#define CONTROLS_AMT 9

const char* controls[] = {\
"UP", "Rotate Clockwise",\
"LEFT", "Left",\
"RIGHT", "Right",\
"DOWN", "Soft Drop",\
"SPACE", "Hard Drop",\
"C", "Hold",\
"Z", "Rotate Counter-Clockwise",\
"P", "Pause",\
"Q", "Quit",\
};

static void printHelp(char *progName, const struct option* options, const char** opt_descs, int optCount) {
    printf("Usage: %s [OPTIONS]\n\nOptions:\n", progName);
    for (int i = 0; i < optCount; ++i) {
	printf("   -%c --%s%s", options[i].val, options[i].name, options[i].has_arg ? "=AMT" : "");
	size_t whiteSpaceOffset = 26;
	for (size_t wspace = strlen(options[i].name) + 8; wspace < whiteSpaceOffset - (options[i].has_arg ? 4 : 0); ++wspace) printf(" ");
	char *splitDesc = strdup(opt_descs[i]);
	size_t dLen = strlen(splitDesc);
	int brokeLine = 0;
	int lineBreak = 0;
	for (size_t j = 0; dLen >= 50 && j < dLen; ++j, ++lineBreak) {
	    if (lineBreak < 50 || splitDesc[j] != ' ') continue;
	    splitDesc[j] = '\n';
	    lineBreak = 0;
	    brokeLine = 1;
	}// instead of padding the string by insreting, do this BS
	for (size_t j = 0; j < dLen; ++j) {
	    if (splitDesc[j] == '\n') {
		printf("\n");
		for (size_t wspace = 0; wspace < whiteSpaceOffset; ++wspace) printf(" ");
		continue;
	    }
	    printf("%c", splitDesc[j]);
	}
	printf("\n");
	if (brokeLine) printf("\n");
	free(splitDesc);
    }

    printf("\nControls:\n");

    for (int i = 0; i < CONTROLS_AMT * 2; i += 2) {
	printf("   %s", controls[i]);
	for (int j = strlen(controls[i]); j < 6; ++j) printf(" ");
	printf("- %s\n", controls[i + 1]);
    }
    //printf("\n\nGithub Repo: \n", progName); TODO: Add github repo
}

int parseArgs(int argc, char *argv[]) {
   char opt;
    while ((opt = getopt_long(argc, argv, "stwzphdl:n:x:y:", long_options, NULL)) != -1) {
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
	case 'd': 
	    debugInfo = 1;
	    break; 
	case 'h':
	    printHelp(argv[0], long_options, descs, ARG_AMT);
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

