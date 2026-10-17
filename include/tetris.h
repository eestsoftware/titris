#ifndef TETRIS_H
#define TETRIS_H

//xs array, x size, start pos
#define shift_array(xs, x, y)\
do {\
for (int xsi = y; xsi < x - 1; ++xsi) {\
xs[xsi] = xs[xsi + 1];\
}\
--x;\
} while(0)

typedef struct Vector2i {
    int x;
    int y;
} Vector2i;

typedef struct Tetromino {
    Vector2i position;
    int rotation;
    Vector2i points[4];
    int id;
} Tetromino;

enum scoreAction { 
   NONE, SINGLE, DOUBLE, TRIPLE, TETRIS, MTSP, MTSP_SINGLE, MTSP_DOUBLE, TSP, TSP_SINGLE, TSP_DOUBLE, TSP_TRIPLE
};
// 4 - tetris, 5 - mini start, 8 - tsp start

extern const Vector2i wallKicksR[4];
extern const Vector2i wallKicksIR[4];

// GRACE
extern int grace, graceAdds, realSeconds; //grace
#define GRACE_LIMIT 9
void graceCheck();

// GAME VARIABLES
extern int paused, didHold, lost;
extern long score;
extern int lines, level, frameTime, b2b, lastScoreAction, tSpin, lastMoveWasRotation;

extern int **minos;
extern int inQueue;
extern Tetromino *next[14];

extern Tetromino *dropping;
extern Tetromino *held;

#define boardOffset 8 //visual offset
extern Vector2i board;

void initTetromino(Tetromino *piece, int id);
// 0: I, 1: J, 2: L, 3: O, 4: S, 5: Z, 6: T
Vector2i rotatedTetromino(Vector2i points, int rotation, int id);
void generateBag(Tetromino *next[14], int *queue);

int tryMoveTetromino(Tetromino *piece, Vector2i move, int pretend);
int tryRotateTetromino(Tetromino *piece, int rotation, int cClockwise);
Tetromino* getShadow(Tetromino *piece, int nullIfSame);
void reCheckLevel();
void checkLineClears();
void holdPiece(Tetromino *piece);
void setDropped(Tetromino *piece, int doBonus);
void updateHoldings();

#endif
