#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "../include/tetris.h"
#include "../include/parseargs.h"

const Vector2i wallKicksR[4] = { (Vector2i){.x=-1,.y=0}, (Vector2i){.x=-1,.y=-1}, (Vector2i){.x=0,.y=2}, (Vector2i){.x=-1,.y=2} };
const Vector2i wallKicksIR[4] = { (Vector2i){.x=-2,.y=0}, (Vector2i){.x=1,.y=0}, (Vector2i){.x=-2,.y=1}, (Vector2i){.x=1,.y=-2} };

//I lowk dont belive in C bools

int paused = 0, didHold = 0, lost = 0;//game params
int grace = 0, graceAdds = 0, realSeconds = 1;

inline void graceCheck() {
    if (grace == 0 || graceAdds >= GRACE_LIMIT) return;
    if (realSeconds >= 5) ++graceAdds; //grace period GET IT
    realSeconds = 1;
}

long score = 0;
int lines = 0;
int level = 1, frameTime = 100;
int b2b = 0, lastScoreAction = 0, tSpin = 0, lastMoveWasRotation = 0;

Vector2i board = (Vector2i){ .x = 11, .y = 22 };

int **minos;

int inQueue = 0; //next.size
Tetromino *next[14];

Tetromino *dropping;
Tetromino *held;

void initTetromino(Tetromino *piece, int id) {
    piece->id = id;
    switch (id) {
    case 0://I
	piece->points[0] = (Vector2i){ .x = 0, .y = 1 };
	piece->points[1] = (Vector2i){ .x = 1, .y = 1 };
	piece->points[2] = (Vector2i){ .x = 2, .y = 1 };
	piece->points[3] = (Vector2i){ .x = 3, .y = 1 };
	break;
    case 1://J
	piece->points[0] = (Vector2i){ .x = 0, .y = 0 };
	piece->points[1] = (Vector2i){ .x = 0, .y = 1 };
	piece->points[2] = (Vector2i){ .x = 1, .y = 1 };
	piece->points[3] = (Vector2i){ .x = 2, .y = 1 };
	break;
    case 2://L
	piece->points[0] = (Vector2i){ .x = 2, .y = 0 };
	piece->points[1] = (Vector2i){ .x = 2, .y = 1 };
	piece->points[2] = (Vector2i){ .x = 1, .y = 1 };
	piece->points[3] = (Vector2i){ .x = 0, .y = 1 };
	break;
    case 3://O
	piece->points[0] = (Vector2i){ .x = 0, .y = 0 };
	piece->points[1] = (Vector2i){ .x = 0, .y = 1 };
	piece->points[2] = (Vector2i){ .x = 1, .y = 0 };
	piece->points[3] = (Vector2i){ .x = 1, .y = 1 };
	break;
    case 4://S
	piece->points[0] = (Vector2i){ .x = 1, .y = 0 };
	piece->points[1] = (Vector2i){ .x = 2, .y = 0 };
	piece->points[2] = (Vector2i){ .x = 0, .y = 1 };
	piece->points[3] = (Vector2i){ .x = 1, .y = 1 };
	break;
    case 5://Z
	piece->points[0] = (Vector2i){ .x = 0, .y = 0 };
	piece->points[1] = (Vector2i){ .x = 1, .y = 0 };
	piece->points[2] = (Vector2i){ .x = 1, .y = 1 };
	piece->points[3] = (Vector2i){ .x = 2, .y = 1 };
	break;
    case 6://T
	piece->points[0] = (Vector2i){ .x = 1, .y = 0 };
	piece->points[1] = (Vector2i){ .x = 0, .y = 1 };
	piece->points[2] = (Vector2i){ .x = 1, .y = 1 };
	piece->points[3] = (Vector2i){ .x = 2, .y = 1 };
	break;
    case 7://X?!
	piece->points[0] = (Vector2i){ .x = 1, .y = 0 };
	piece->points[1] = (Vector2i){ .x = 0, .y = 1 };
	piece->points[2] = (Vector2i){ .x = 2, .y = 1 };
	piece->points[3] = (Vector2i){ .x = 1, .y = 2 };
	break;
    }
}

Vector2i rotatedTetromino(Vector2i points, int rotation, int id) {
    if (id == 3) return points;

    int pivotx = id > 0 ? 1 : 3;//I hate I piece
    int pivoty = id > 0 ? 1 : 0;

    for (int j = 0; j < rotation; ++j) {
	int x = points.x - (id > 0 ? pivotx : 0);
	int y = points.y - (id > 0 ? pivoty : 0);

        points.x = pivotx - y;
        points.y = pivoty + x;
    }
    return points;
}

void generateBag(Tetromino *bagNext[14], int *queue) {
    if (*queue >= 14) return; 
    int bag[7] = {0, 1, 2, 3, 4, 5, 6};
    int bagSize = 7;
    srand(time(NULL));
    while(bagSize > 0 && *queue < 14) {
	int i = rand() % bagSize;
	Tetromino *bagPiece = malloc(sizeof *bagPiece);
	initTetromino(bagPiece, bag[i]);
	bagNext[*queue] = bagPiece;
	++(*queue);
	shift_array(bag, bagSize, i);
	/*for (int j = i; j < bagSize - 1; ++j) {
	    bag[i] = bag[i + 1];
	}
	--bagSize;*/
    }
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

void reCheckLevel() {
    level = startLevel + (lines / 10);
    float newFrameTime = pow(0.8-((float)(level-1.0)*0.007), level-1);
    frameTime = floor(newFrameTime * 100);
    if (frameTime <= 0) frameTime = 1;
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
