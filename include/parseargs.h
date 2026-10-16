#ifndef T_PARSEARGS_H
#define T_PARSEARGS_H

//settings
extern int colors, termColors, showLineClear, nextBoxes, boardX, boardY;

void printHelp(const struct option long_options, const char* descs);
int parseArgs(int argc, char *argv[]);

#endif
