#ifndef TETRIS_H
#define TETRIS_H

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

extern const Vector2i wallKicksR[4];
extern const Vector2i wallKicksIR[4];
extern const Vector2i wallKicksL[4];
extern const Vector2i wallKicksIL[4];

void initTetromino(Tetromino *piece, int id);
// 0: I, 1: J, 2: L, 3: O, 4: S, 5: Z, 6: T
Vector2i rotatedTetromino(Vector2i points, int rotation, int id);
void generateBag(Tetromino *next[14], int *queue);

#define da_append(xs, x)\
do {\
if (xs.size >= xs.capacity) {\
if (xs.capacity == 0) xs.capacity = 256;\
else xs.capacity *= 2;\
xs.items = realloc(xs.items, xs.capacity*sizeof(*xs.items));\
}\
xs.items[xs.size++] = x;\
} while(0)

//xs array, x size, start pos
#define shift_array(xs, x, y)\
do {\
for (int xsi = y; xsi < x - 1; ++xsi) {\
xs[xsi] = xs[xsi + 1];\
}\
--x;\
} while(0)

typedef struct Pieces {
    Tetromino *items;
    size_t size;
    size_t capacity;
} Pieces;

#endif
