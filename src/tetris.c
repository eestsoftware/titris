#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "../include/tetris.h"

const Vector2i wallKicksR[4] = { (Vector2i){.x=-1,.y=0}, (Vector2i){.x=-1,.y=-1}, (Vector2i){.x=0,.y=2}, (Vector2i){.x=-1,.y=2} };
const Vector2i wallKicksIR[4] = { (Vector2i){.x=-2,.y=0}, (Vector2i){.x=1,.y=0}, (Vector2i){.x=-2,.y=1}, (Vector2i){.x=1,.y=-2} };

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

void generateBag(Tetromino *next[14], int *queue) {
    if (*queue >= 14) return; 
    int bag[7] = {0, 1, 2, 3, 4, 5, 6};
    int bagSize = 7;
    srand(time(NULL));
    while(bagSize > 0 && *queue < 14) {
	int i = rand() % bagSize;
	Tetromino *bagPiece = malloc(sizeof *bagPiece);
	initTetromino(bagPiece, bag[i]);
	next[*queue] = bagPiece;
	++(*queue);
	shift_array(bag, bagSize, i);
	/*for (int j = i; j < bagSize - 1; ++j) {
	    bag[i] = bag[i + 1];
	}
	--bagSize;*/
    }
}

