#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include "../include/tetris.h"
#include "../include/parseargs.h"

void renderBox(Vector2i bounds, Vector2i offset, char *title) {
    offset.x = offset.x * 2; //Spacing offset
    for (int y = 0; y < bounds.y; ++y) {
	for (int x = 0; x < bounds.x; ++x) {
	    mvprintw(y + offset.y, (x * 2) + offset.x, ".");
	}
    }

    attron(COLOR_PAIR(8));
    for (int y = 0; y <= bounds.y; ++y) {
	mvprintw(y + offset.y, offset.x, "|");
	mvprintw(y + offset.y, bounds.x * 2 + offset.x, "|");
	if (!colors) continue;
	mvprintw(y + offset.y, offset.x - 1, "|");
	mvprintw(y + offset.y, bounds.x * 2 - 1 + offset.x, "|");
    }

    for (int x = 0; x <= bounds.x; ++x) {
	mvprintw(offset.y, (x * 2) + offset.x, "-");
	mvprintw(bounds.y + offset.y, (x * 2) + offset.x, "-");
	if (x == 0 || !colors) continue;
	mvprintw(offset.y, (x * 2) - 1 + offset.x, "-");
	mvprintw(bounds.y + offset.y, (x * 2) - 1 + offset.x, "-");
    }
    attroff(COLOR_PAIR(8));

    if (title == NULL) return;
    if (!whiteBorders) attron(COLOR_PAIR(24));
    mvprintw(offset.y, offset.x + (bounds.x / 2) + 1, "%s", title);
    attroff(COLOR_PAIR(24));
}

void renderTetromino(Tetromino *piece, int fallPreview) {
    if (piece == NULL) return;
    if (fallPreview == 1) renderTetromino(getShadow(piece, 1), -1);
    attron(COLOR_PAIR( ( fallPreview >= 0 ? 9 : 17) + piece->id));
    for (int i = 0; i < 4; ++i) {
	Vector2i points = rotatedTetromino(piece->points[i], piece->rotation, piece->id);
	mvprintw(points.y + piece->position.y, points.x * 2 + piece->position.x * 2, fallPreview >= 0 ? "@" : ",");
	if (colors) mvprintw(points.y + piece->position.y, (points.x * 2) - 1 + piece->position.x * 2, fallPreview >= 0 ? "@" : " ");
    }
    if (fallPreview < 0) free(piece);
     attroff(COLOR_PAIR(( fallPreview > 0 ? 9 : 15) + piece->id));
}

void printBoard() {
    erase();

    if (!whiteBorders) attron(COLOR_PAIR(24));
    mvprintw(8,  2, "SCORE: %'ld%s", (score < 100000) ? score : score / 1000, (score < 100000) ? "" : "K");
    mvprintw(9,  2, "LEVEL: %d", level);
    mvprintw(10, 2, "LINES: %d", lines);
    if (showLineClear == 1 && lastScoreAction > 0) {
	char *clearType = "SINGLE";
	switch(lastScoreAction) {
	case 2:
	    clearType = "DOUBLE";
	    break;
	case 3:
	    clearType = "TRIPLE";
	    break;
	case 4:
	    clearType = "TETRIS";
	    break;
	case 5:
	    clearType = "MINI T-SPIN";
	    break;
	case 6:
	    clearType = "MINI T-SPIN\nSINGLE";
	    break;
	case 7:
	    clearType = "MINI T-SPIN\nDOUBLE";
	    break;
	case 8:
	    clearType = "T-SPIN";
	    break;
	case 9:
	    clearType = "T-SPIN SINGLE";
	    break;
	case 10:
	    clearType = "T-SPIN DOUBLE";
	    break;
	case 11:
	    clearType = "T-SPIN TRIPLE";
	    break;
	}
	//if (b2b) sprintf(clearType, "%s", "\nBACK-TO-BACK");
	mvprintw(!debugInfo ? 13 : 16, 2, "%s\n%s", clearType, b2b ? "BACK-TO-BACK" : "");
    }
    if (debugInfo) {
	mvprintw(11, 2, "GRACE: %d", grace);
	mvprintw(12, 2, "GRACEADDS: %d", graceAdds);
	mvprintw(13, 2, "SECS: %d", realSeconds);
    }
    attroff(COLOR_PAIR(24));

    renderBox( board, (Vector2i){.x = boardOffset, .y = 0 }, NULL);
    renderBox( (Vector2i){.x = 5, .y = 5}, (Vector2i){.x = 1,. y = 1}, "HOLD");
    if (held != NULL) renderTetromino(held, 0);
    for (int i = 0; i < nextBoxes; ++i) {
	renderBox( (Vector2i){.x = 5, .y = 5}, (Vector2i){.x = (boardOffset + 2) + board.x,. y = i * 5}, i == 0 ? "NEXT" : NULL);
	renderTetromino(next[i + 1], 0);
    }

    for (int y = 0; y < board.y; ++y) {
	for (int x = 1; x < board.x; ++x) {
	    if (minos[y][x] <= 0) continue;
	    attron(COLOR_PAIR( 8 + minos[y][x]));
	    mvprintw(y, (x * 2) + boardOffset * 2, "#");
	    if (colors) mvprintw(y, (x * 2) - 1 + boardOffset * 2, "#");
	    attroff(COLOR_PAIR( 8 + minos[y][x]));
	}
    }

    if (dropping != NULL) renderTetromino(dropping, 1);
    if (paused) mvprintw(board.y + 2, boardOffset + (board.x / 2) + 10, "PAUSED");
    if (lost) {
	mvprintw(board.y + 3, boardOffset + board.x / 2, "GAME OVER. GG.\nFINAL SCORE: %'ld", score);
	if (variableGoal) mvprintw(board.y + 5, 0, "(VARIABLE-GOAL MODE)");
    }
}

#define START_COLOR 21 //Arbitrary, but to avoid conflicts with the first 0-15 colors
//20 -> gray, other are 20 + piece_idx

void initColors() {
    //bg grey
    init_color(START_COLOR - 1, 392, 392, 392);
    int buffColor = whiteBorders ? 7 : (START_COLOR - 1);
    init_pair(8, buffColor, colors ? buffColor : 0);
    //I red
    init_color(START_COLOR + 0, 940, 0, 0);
    buffColor = termColors ? 4 : (START_COLOR);
    init_pair(9, buffColor, colors ? buffColor : 0);
    init_pair(17, buffColor, COLOR_BLACK);
    //J purple
    init_color(START_COLOR + 1, 666, 0, 941);
    buffColor = termColors ? 5 : (START_COLOR + 1);
    init_pair(10, buffColor, colors ? buffColor : 0);
    init_pair(18, buffColor, COLOR_BLACK);
    //L yellow
    init_color(START_COLOR + 2, 917, 917, 0);
    buffColor = termColors ? 6 : (START_COLOR + 2);
    init_pair(11, buffColor, colors ? buffColor : 0);
    init_pair(19, buffColor, COLOR_BLACK);
    //O blue
    init_color(START_COLOR + 3, 0, 0, 805);
    buffColor = termColors ? 1 : (START_COLOR + 3);
    init_pair(12, buffColor, colors ? buffColor : 0);
    init_pair(20, buffColor, COLOR_BLACK);
    //S teal
    init_color(START_COLOR + 4, 0, 921, 921);
    buffColor = termColors ? 3 : (START_COLOR + 4);
    init_pair(13, buffColor, colors ? buffColor : 0);
    init_pair(21, buffColor, COLOR_BLACK);
    //Z pink
    init_color(START_COLOR + 5, 925, 0, 925);
    buffColor = termColors ? (zMagenta ? 5 : 13) : (START_COLOR + 5);
    init_pair(14, buffColor, colors ? buffColor : 0);
    init_pair(22, buffColor, COLOR_BLACK);
    //T green
    init_color(START_COLOR + 6, 0, 933, 0);
    buffColor = termColors ? 2 : (START_COLOR + 6);
    init_pair(15, buffColor, colors ? buffColor : 0);
    init_pair(23, buffColor, COLOR_BLACK);
    //X white
    init_color(START_COLOR + 7, 933, 933, 933);
    buffColor = (termColors || whiteBorders) ? 7 : (START_COLOR + 7);
    init_pair(16, 28, colors ? buffColor : 0);
    init_pair(24, buffColor, COLOR_BLACK);
}

