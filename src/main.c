#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <math.h>
#include <time.h>
#include "../include/tetris.h"
#include "../include/parseargs.h"

//I lowk dont belive in C bools
int playing = 1, frames = 0, paused = 0, didHold = 0, lost = 0;//game params
int grace = 0, graceAdds = 0, realSeconds = 0; //grace

void graceCheck() {
    //if (tryMove == 0 && (graceAdds < graceLimit) && grace > -1 && grace <= 1) 
    if (grace == 0 || graceAdds > 6) return;
    ++graceAdds;
    realSeconds = 1;
}

long score = 0;
int lines = 0;
int level = 1, frameTime = 100;
int b2b = 0, lastScoreAction = 0, tSpin = 0;

int lastMoveWasRotation = 0;

Vector2i board = (Vector2i){ .x = 11, .y = 22 };
const int boardOffset = 8;
int **minos;

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
    lastMoveWasRotation = 0;
    piece->position.x += move.x;
    piece->position.y += move.y;
    return 0;
}

int tryRotateTetromino(Tetromino *piece, int rotation, int cClockwise) {
    if (rotation < 0) rotation = 3;
    int failed = 0;
    for (int i = 0; i < 4; ++i) {
	Vector2i nPos = rotatedTetromino(piece->points[i], rotation, piece->id);
	nPos.x += piece->position.x;
	nPos.y += piece->position.y;
	if (nPos.x >= board.x + boardOffset || nPos.x <= boardOffset) failed = 1;//OOB checks
	else if (nPos.y >= board.y || nPos.y < 0) failed = 1;
	else if (minos[nPos.y][nPos.x - boardOffset] >= 1) failed = 1; //Collision check
	if (failed) break;
    }

    int oldRot = piece->rotation;
    piece->rotation = rotation;
    if (!failed) {
	lastMoveWasRotation = 1;
	return 0;
    }

    // WALLKICKS WILL WORK!
    for (int i = 0; i < 4; ++i) {
	if (piece == NULL) return 1;
	Vector2i move = piece->id > 0 ? wallKicksR[i] : wallKicksIR[i]; //wallkicksR is rotation 1
	if (piece->id == 0 && (oldRot == 3 || oldRot == 1 || (oldRot == 0 && cClockwise) || (oldRot == 2 && cClockwise) )) {
	    move = wallKicksIR[(i % 2 == 0) ? (i + 1) : (i - 1)]; //swap 0 with 1 , 1 - 0, 2 - 3, 3 - 2
	}
	if (oldRot == 1)  {
	    move.x *= -1;
	    if (piece->id > 0 && !cClockwise) move.y *= -1;
	}
	if ((oldRot == 2 && !cClockwise) || (oldRot == 0 && cClockwise))  {
	    move.x *= -1;
	    if (piece->id <= 0) move.y *= -1;
	}
	if (oldRot == 3 && (piece->id > 0 || !cClockwise) ) { //Dont do this for cclockwise I
	    move.y *= -1;
	}
	if (piece->id <= 0 && cClockwise && oldRot == 2) move.y *= 1;
	if (tryMoveTetromino(piece, move, 0) == 0) {
	    lastMoveWasRotation = i < 3 ? 1 : 2;
	    return 0;
	}
    }
    piece->rotation = oldRot;
    return 1;
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
	mvprintw(13, 2, "%s\n%s", clearType, b2b ? "BACK-TO-BACK" : "");
    }
    //mvprintw(11, 2, "GRACE: %d", grace);
    //mvprintw(12, 2, "GRACEADDS: %d", graceAdds);
    //mvprintw(13, 2, "SECS: %d", realSeconds);
    //mvprintw(13, 2, "LASTWROT?: %d", lastMoveWasRotation);
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
    if (lost) mvprintw(board.y + 3, boardOffset + board.x / 2, "GAME OVER. GG.\nFINAL SCORE: %'ld", score);
}

void reCheckLevel() {
    level = 1 + (lines / 10);
    float newFrameTime = pow(0.8-((float)(level-1.0)*0.007), level-1);
    frameTime = floor(newFrameTime * 100);
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
    if (lineClears <= 0) {
	if (tSpin > 0) {
	    score += (tSpin == MTSP ? 100 : 400) * level;
	    lastScoreAction = tSpin;
	}
	return;
    }
    lines += lineClears;
    reCheckLevel();
    
    int plusScore = 100;
    plusScore += 200 * (lineClears - 1);
    if (lineClears >= 4) plusScore += 100;
    b2b = 0;
    lineClears += tSpin; // for easier score measuring
    if (lineClears == 4 && lastScoreAction == TETRIS) b2b = 1;
    if (lineClears >= MTSP_SINGLE) {
	plusScore += lineClears >= TSP ? 100 : 700;
	if (lineClears >= TSP_DOUBLE) plusScore += 200;
	if (lineClears >= TSP_TRIPLE) plusScore += 400;
    }
    if (lineClears >= TETRIS && lastScoreAction >= TETRIS &&\
    lineClears != MTSP && lineClears != TSP &&\
    lastScoreAction != MTSP && lastScoreAction != TSP) {	
	plusScore *= 1.5;
	b2b = 1;
    }
    score += plusScore * level;
    tSpin = 0;
    lastScoreAction = lineClears;

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
    checkLineClears();
}

void updateHoldings() {
    if (lost) return;
    while (inQueue <= 7) generateBag(next, &inQueue);
    dropping = next[0];
    dropping->position.x = boardOffset + (board.x / 2);
    dropping->position.y = dropping->id > 0 ? 1 : 0;
    grace = 0;
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
	Tetromino *store = piece;
	dropping = held;
	held = store;
	dropping->position.x = boardOffset + (board.x / 2);
    } else {
	held = piece;
	dropping = NULL;
	next[0] = NULL;
	shift_array(next, inQueue, 0);
	updateHoldings();
    }
    held->position.x = 2;
    held->position.y = 2;
}

Vector2i checkTSpin(Vector2i centerPos, int rotation) {
    Vector2i corners = {0};//x is front, y is back
    for (int i = 0; i < 4; ++i) {
	Vector2i dir = (Vector2i){ .x = -1, .y = -1 };
	if (i % 2 == 1) dir.x *= -1;
	if (i >= 2) dir.y *= -1;
	Vector2i nPos = centerPos;
	nPos.x += dir.x;
	nPos.y += dir.y;
	if (nPos.y >= board.y || nPos.x >= board.x || nPos.y < 0 || nPos.x < 1) {
	    goto checkRot;
	}
	if (minos[nPos.y][nPos.x] <= 0) continue;
	checkRot:
	switch(rotation) { //Too lazy to do some rotation bs here so heres a hard-coded switch 
	case 0:
	    if (dir.y < 0) ++corners.x;
	    else ++corners.y;
	    break;
	case 1:
	    if (dir.x > 0) ++corners.x;
	    else ++corners.y;
	    break;
	case 2:
	    if (dir.y > 0) ++corners.x;
	    else ++corners.y;
	    break;
	case 3:
	    if (dir.x < 0) ++corners.x;
	    else ++corners.y;
	    break;
	}
    }
    if (lastMoveWasRotation >= 2) {
	int xCorners = corners.x;
	int yCorners = corners.y;
	corners.y = xCorners;
	corners.x = yCorners;
    }
    return corners;
}

void setDropped(Tetromino *piece, int doBonus) {
    tSpin = 0;
    Tetromino *shadow = getShadow(piece, 0);
    for (int i = 0; i < 4; ++i) {
	Vector2i nPos = rotatedTetromino(shadow->points[i], shadow->rotation, piece->id);
	nPos.x += shadow->position.x - boardOffset;
	if (doBonus) score += (piece->position.y) * 2;
	nPos.y += shadow->position.y;
	if (nPos.y >= board.y || nPos.x >= board.x || nPos.y < 0 || nPos.x < 1) continue;
	minos[nPos.y][nPos.x] = 1+piece->id;
	if (piece->id != 6 || i != 2 || lastMoveWasRotation <= 0) continue;

	Vector2i corners = checkTSpin(nPos, piece->rotation);
	if (corners.x >= 2 && corners.y > 0) tSpin = TSP;
	else if (corners.x > 0 && corners.y >= 2) tSpin = MTSP;
    }
    free(shadow);
    free(dropping);
    dropping = NULL;
    next[0] = NULL;
    shift_array(next, inQueue, 0);
    checkLineClears();
    didHold = 0;
    updateHoldings();
    reCheckLevel();
}

void getInput(int ch) {
    if (ch == 'q' || ch == 'Q') playing = 0;
    if (ch == 'p' || ch == 'P') paused ^= 1;

    if (dropping == NULL || paused || lost) return;
    
    if (ch == KEY_LEFT || ch == KEY_RIGHT) {
	graceCheck();
	tryMoveTetromino(dropping, (Vector2i){ .x = ch == KEY_LEFT ? -1 : 1, .y = 0 }, 0); 
    }
    if (ch == KEY_DOWN) { 
	// soft drop
	int tryMove = tryMoveTetromino(dropping, (Vector2i){ .x = 0, .y = 1 }, 0); 
	if (tryMove == 0) ++score;
	else if (tryMove == 1 && grace == 0) {
	    grace = 1;
	    realSeconds = 1;
	}
    } 
    if (ch == KEY_UP || ch == 'x' || ch == 'z') {
	graceCheck();
	tryRotateTetromino(dropping, (dropping->rotation + (ch != 'z' ? 1 : -1)) % 4, ch == 'z' ? 1 : 0);
    }
    if (ch == ' ') setDropped(dropping, 1);
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
    init_pair(17, buffColor, 8);
    init_color(22, 666, 0, 941);
    //J purple
    buffColor = termColors ? 5 : 22;
    init_pair(10, buffColor, colors ? buffColor : 0);
    init_pair(18, buffColor, 8);
    init_color(23, 917, 917, 0);
    //L yellow
    buffColor = termColors ? 6 : 23;
    init_pair(11, buffColor, colors ? buffColor : 0);
    init_pair(19, buffColor, 8);
    init_color(24, 0, 0, 805);
    //O blue
    buffColor = termColors ? 1 : 24;
    init_pair(12, buffColor, colors ? buffColor : 0);
    init_pair(20, buffColor, 8);
    init_color(25, 0, 921, 921);
    //S teal
    buffColor = termColors ? 3 : 25;
    init_pair(13, buffColor, colors ? buffColor : 0);
    init_pair(21, buffColor, 8);
    init_color(26, 925, 0, 925);
    //Z pink
    buffColor = termColors ? 13 : 26;
    init_pair(14, buffColor, colors ? buffColor : 0);
    init_pair(22, buffColor, 8);
    init_color(27, 0, 933, 0);
    //T green
    buffColor = termColors ? 10 : 27;
    init_pair(15, buffColor, colors ? buffColor : 0);
    init_pair(23, buffColor, 8);
    init_color(28, 933, 933, 933);
    //X white
    buffColor = termColors ? 7 : 28;
    init_pair(16, 28, colors ? buffColor : 0);
    init_pair(24, 28, 8);
}

int main(int argc, char *argv[]) {
    //parse flags
    if (parseArgs(argc, argv)) return 1;
    board.x = boardX;
    board.y = boardY;

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
	if (!paused && dropping != NULL && (frames == 0 || (grace == 1 && realSeconds == 0)) && (tryMoveTetromino(dropping, (Vector2i){.x=0,.y=1}, 0) == 1 || tryMoveTetromino(dropping, (Vector2i){.x=0,.y=2}, 1) == 1)) {
	    if (grace == 1 && realSeconds == 0 && tryMoveTetromino(dropping, (Vector2i){.x=0,.y=1}, 0) == 1) { setDropped(dropping, 0); } 
	    else if (grace == 0) {
		grace = 1;
		realSeconds = 1;
	    } 
	} else if (dropping != NULL && tryMoveTetromino(dropping, (Vector2i){.x=0,.y=3}, 1) == 0 && realSeconds <= 10 && realSeconds > 2) {
	    grace = 0;
	    realSeconds = 1;
	}
	getInput(getch());
	refresh();
	if (!paused && !lost) {
	    frames = (frames + 1) % frameTime;
	    if (grace >= 1) realSeconds = (realSeconds + 1) % 50;
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
