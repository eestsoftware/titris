#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <math.h>
#include <time.h>
#include "../include/tetris.h"

int playing = 1, frames = 0, paused = 0, colors = 1, termColors = 1, grace = -1, graceAdds = 0, didHold = 0, lost = 0;
long score = 0;
int lines = 0;
int level = 1, frameTime = 100;

const Vector2i board = (Vector2i){ .x = 11, .y = 22 };
const int boardOffset = 8;
int **minos;

int nextBoxes = 3;
int inQueue = 0; //next.size
Tetromino *next[14];

Tetromino *dropping;
Tetromino *held;

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
    attron(COLOR_PAIR(24));
    mvprintw(offset.y, offset.x + (bounds.x / 2) + 1, "%s", title);
    attroff(COLOR_PAIR(24));
}

int tryMoveTetromino(Tetromino *piece, Vector2i move, int pretend) {
    for (int i = 0; i < 4; ++i) {
	if (piece == NULL) return 1;
	Vector2i nPos = rotatedTetromino(piece->points[i], piece->rotation, piece->id);
	nPos.x += move.x + piece->position.x;
	nPos.y += move.y + piece->position.y;
	if (nPos.x >= board.x + boardOffset || nPos.x <= boardOffset) return 1;//OOB checks
	if (nPos.y >= board.y) return 1;
	if (minos[nPos.y][nPos.x - boardOffset] >= 1) return 1; //Collision check
    }
    if (pretend) return 0;
    piece->position.x += move.x;
    piece->position.y += move.y;
    return 0;
}

int tryRotateTetromino(Tetromino *piece, int rotation) {
    int failed = 0;
    for (int i = 0; i < 4; ++i) {
	Vector2i nPos = rotatedTetromino(piece->points[i], rotation, piece->id);
	nPos.x += piece->position.x;
	nPos.y += piece->position.y;
	if (nPos.x >= board.x + boardOffset || nPos.x <= boardOffset) failed = 1;//OOB checks
	if (nPos.y >= board.y) failed = 1;
	if (minos[nPos.y][nPos.x - boardOffset] >= 1) failed = 1; //Collision check
    }
    int oldRot = piece->rotation;
    piece->rotation = rotation;
    if (failed) { // WALLKICKS WILL WORK!
	for (int i = 0; i < 4; ++i) {
	    if (piece == NULL) return 1;
	    Vector2i move = piece->id > 0 ? wallKicksR[i] : wallKicksIR[i]; //wallkicksR is rotation 1
	    if (piece->id == 0 && (rotation == 3 || rotation == 0)) {
		move = wallKicksIR[(i % 2 == 0) ? (i + 1) : (i - 1)];
	    }
	    if (rotation == 2 || rotation == 3) {
		move.x *= -1;
		move.y *= -1;
	    }
	    if (tryMoveTetromino(piece, move, 0) == 0) return 0;
	}
	piece->rotation = oldRot;
	return 1;
    }
    return 0;
}

Tetromino* getShadow(Tetromino *piece, int nullIfSame) {
    Tetromino *shadow = malloc(sizeof *shadow);
    initTetromino(shadow, piece->id);
    shadow->position = piece->position;
    shadow->rotation = piece->rotation;
    int times = 0;
    while (tryMoveTetromino(shadow, (Vector2i){.x=0,.y=1}, 0) == 0) ++times;
    if (times <= 0 && nullIfSame) {
	free(shadow);
	shadow = NULL;
    }
    return shadow;
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

    attron(COLOR_PAIR(24));
    mvprintw(8,  2, "SCORE: %ld%s", (score < 100000) ? score : score / 1000, (score < 100000) ? "" : "K");
    mvprintw(9,  2, "LEVEL: %d", level);
    mvprintw(10, 2, "LINES: %d", lines);
    //mvprintw(11, 2, "FRAMES: %d", frames);
    mvprintw(11, 2, "GRACE: %d", grace);
    mvprintw(12, 2, "GRACEADDS: %d", graceAdds);
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
    if (paused) mvprintw(board.y + 2, boardOffset + board.x / 2, "PAUSED");
    if (lost) mvprintw(board.y + 3, boardOffset + board.x / 2, "GAME OVER. GG.\nFINAL SCORE: %'ld", score);
}

void checkLineClears() {
    int linesToClear[6];
    int lineClears = 0;
    for (int y = 0; y < board.y; ++y) {
	for (int x = 1; x <board.x; ++x) {
	    if (minos[y][x] <= 0) break;
	    if (x < board.x - 1) continue;
	    linesToClear[lineClears] = y;
	    ++lineClears;
	}
    }
    if (lineClears <= 0) return;
    lines += lineClears;
    level = 1 + (lines / 10);
    float newFrameTime = pow(0.8-((float)(level-1.0)*0.007), level-1);
    frameTime = floor(newFrameTime * 100);
    
    int plusScore = 100;
    plusScore += 200 * (lineClears - 1);
    if (lineClears >= 4) plusScore += 100;
    score += plusScore * level;

    int writey = board.y - 1;
    for (int y = board.y - 1; y >= 0; --y) {
        int cleared = 0;
        for (int i = 0; i < lineClears; ++i) {
            if (linesToClear[i] == y) {
                cleared = 1;//If this line is makred for clear ignor
                break;
            }
        }

        if (cleared) continue;

	for (int x = 0; x < board.x; ++x) minos[writey][x] = minos[y][x];
        --writey;
    }

    for (;writey >= 0;--writey) {
        for (int x = 1; x < board.x; ++x) minos[writey][x] = 0;
    }
//
//    int startY = linesToClear[0];
//    for (int i = 0; i < lineClears; ++i) {
//	if (linesToClear[i] > startY) startY = linesToClear[i];
//	for (int x = 0; x < board.x; ++x) {
//	    minos[linesToClear[i]][x] = 0;
//	}
//    }
//    for (int y = startY; y > 0; --y) {
//	if (y + lineClears >= board.y) continue; // Might be a bug here?
//	for (int x = 1; x < board.x; ++x) {
//	    if (minos[y][x] <= 0) continue;
//	    minos[y + lineClears][x] = minos[y][x];
//	    minos[y][x] = 0;
//	}
//    }

    checkLineClears();
}

void updateHoldings() {
    if (lost) return;
    while (inQueue <= 7) generateBag(next, &inQueue);
    dropping = next[0];
    dropping->position.x = boardOffset + (board.x / 2);
    dropping->position.y = dropping->id > 0 ? 1 : 0;
    grace = -1;
    graceAdds = 0;
    if (tryMoveTetromino(dropping, (Vector2i){ .x = 0, .y = 0 }, 0)) lost = 1;
    for (int i = 0; i < nextBoxes; ++i) {
	next[i + 1]->position.x = (boardOffset + 3) + board.x;
	next[i + 1]->position.y = (i * 5) + 1;
    }
}

void holdPiece(Tetromino *piece) {
    if (didHold) return;
    didHold = 1;
    if (held != NULL) {
	Tetromino *store = dropping;
	dropping = held;
	held = store;
	dropping->position.x = boardOffset + (board.x / 2);
    } else {
	held = dropping;
	dropping = NULL;
	next[0] = NULL;
	shift_array(next, inQueue, 0);
	updateHoldings();
    }
    held->position.x = 2;
    held->position.y = 2;
}

void setDropped(Tetromino *piece) {
    Tetromino *shadow = getShadow(piece, 0);
    for (int i = 0; i < 4; ++i) {
	Vector2i nPos = rotatedTetromino(shadow->points[i], shadow->rotation, piece->id);
	nPos.x += shadow->position.x - boardOffset;
	nPos.y += shadow->position.y;
	if (nPos.y >= board.y || nPos.x >= board.x || nPos.y < 0 || nPos.x < 0) continue;
	minos[nPos.y][nPos.x] = 1+piece->id;
    }
    free(shadow);
    free(dropping);
    dropping = NULL;
    next[0] = NULL;
    shift_array(next, inQueue, 0);
    checkLineClears();
    didHold = 0;
    updateHoldings();
}

void graceCheck(int tryMove, int punish) {
    if (tryMove == 0 && (graceAdds < 2 || punish) && grace > -1 && grace <= 1) {
	++graceAdds;
	if (!punish) ++grace;
	else if (grace > 0) --grace;
    }
}

void getInput(int ch) {
    if (ch == 'q') playing = 0;
    if (ch == 'p') paused ^= 1;

    if (dropping == NULL || paused) return;
    
    if (ch == KEY_LEFT || ch == KEY_RIGHT) {
	int tryMove = tryMoveTetromino(dropping, (Vector2i){ .x = ch == KEY_LEFT ? -1 : 1, .y = 0 }, 0); 
	graceCheck(tryMove, 0);
    }
    if (ch == KEY_DOWN) {
	if (tryMoveTetromino(dropping, (Vector2i){ .x = 0, .y = 1 }, 0) == 1 && grace < 0) grace = 1;
	else if (grace > -1 && frames % 8 == 0) --grace;
    } else if (ch == KEY_UP || ch == 'z') {
	int tryMove = tryRotateTetromino(dropping, (dropping->rotation + 1) % 4);
	graceCheck(tryMove, 1);
    }
    if (ch == ' ') setDropped(dropping);

    if (ch == 'c') holdPiece(dropping);
}

void initColors() {
    init_color(20, 392, 392, 392);
    //bg grey
    int buffColor = 20;
    init_pair(8, buffColor, colors ? buffColor : 0);
    init_color(21, 940, 0, 0);
    //I red
    buffColor = termColors ? 4 : 21;
    init_pair(9, buffColor, colors ? buffColor : 0);
    init_pair(17, buffColor, 0);
    init_color(22, 666, 0, 941);
    //J purple
    buffColor = termColors ? 5 : 22;
    init_pair(10, buffColor, colors ? buffColor : 0);
    init_pair(18, buffColor, 0);
    init_color(23, 917, 917, 0);
    //L yellow
    buffColor = termColors ? 6 : 23;
    init_pair(11, buffColor, colors ? buffColor : 0);
    init_pair(19, buffColor, 0);
    init_color(24, 0, 0, 805);
    //O blue
    buffColor = termColors ? 1 : 24;
    init_pair(12, buffColor, colors ? buffColor : 0);
    init_pair(20, buffColor, 0);
    init_color(25, 0, 921, 921);
    //S teal
    buffColor = termColors ? 3 : 25;
    init_pair(13, buffColor, colors ? buffColor : 0);
    init_pair(21, buffColor, 0);
    init_color(26, 925, 0, 925);
    //Z pink
    buffColor = termColors ? 13 : 26;
    init_pair(14, buffColor, colors ? buffColor : 0);
    init_pair(22, buffColor, 0);
    init_color(27, 0, 933, 0);
    //T green
    buffColor = termColors ? 10 : 27;
    init_pair(15, buffColor, colors ? buffColor : 0);
    init_pair(23, buffColor, 0);
    init_color(28, 933, 933, 933);
    //X white
    buffColor = termColors ? 7 : 28;
    init_pair(16, 28, colors ? buffColor : 0);
    init_pair(24, 28, 0);
}

int main() {
    //init board
    minos = malloc(board.y * sizeof(*minos));
    for (int i = 0; i < board.y; ++i) {
	minos[i] = malloc(board.x * sizeof(*minos[i]));
	for (int j = 0; j < board.x; ++j) minos[i][j] = 0;
    }

    updateHoldings();

    // Init curses
    initscr();
    start_color();
    use_default_colors();
    initColors();
    curs_set(0);
    cbreak();

    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    nodelay(stdscr, TRUE);
    struct timespec deadline;
    const long frame_ns = 1000000000L / 100;

    clock_gettime(CLOCK_MONOTONIC, &deadline);

    while (playing) {
	printBoard();
	if (dropping != NULL && (frames == 0 || grace == 0) && (tryMoveTetromino(dropping, (Vector2i){.x=0,.y=1}, 0) == 1 || tryMoveTetromino(dropping, (Vector2i){.x=0,.y=2}, 1) == 1)) {
	    if (grace == 0) { setDropped(dropping); }
	    else if (grace < 0) {
		grace = 1;
	    } else {
		--grace;
	    }
	} else if (dropping != NULL && tryMoveTetromino(dropping, (Vector2i){.x=0,.y=1}, 1) == 0) {
	    grace = -1;
	}
	getInput(getch());
	refresh();
	if (!paused && !lost) {
	    frames = (frames + 1) % frameTime;
	}

        deadline.tv_nsec += frame_ns;
        if (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_nsec -= 1000000000L;
            deadline.tv_sec++;
        }
        clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline,NULL );
    }
 
    echo();
    nocbreak();
    nodelay(stdscr, FALSE);
    curs_set(1);

    endwin();
    
    return 0;
}
