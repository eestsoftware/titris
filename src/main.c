#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <curses.h>
#include <time.h>
#include "../include/tetris.h"
#include "../include/parseargs.h"
#include "../include/renderer.h"

//Github.

int playing = 1, frames = 0;//render param

void getInput(int ch) {
    if (ch == 'q' || ch == 'Q') playing = 0;
    if (ch == 'p' || ch == 'P') paused ^= 1;

    if (dropping == NULL || paused || lost) return;
    
    if (ch == KEY_LEFT || ch == 'h' || ch == KEY_RIGHT || ch == 'l') {
	int tryMove = tryMoveTetromino(dropping, (Vector2i){ .x = (ch == KEY_LEFT || ch == 'h') ? -1 : 1, .y = 0 }, 0); 
	if (tryMove == 0) graceCheck();
    }
    if (ch == KEY_DOWN || ch == 'j') { 
	// soft drop
	int tryMove = tryMoveTetromino(dropping, (Vector2i){ .x = 0, .y = 1 }, 0); 
	if (tryMove == 0) ++score;
	else if (tryMove == 1 && grace == 0) {
	    grace = 1;
	    realSeconds = 1;
	}
    } 
    if (ch == KEY_UP || ch == 'x' || ch == 'z' || ch == 'k') {
	graceCheck();
	tryRotateTetromino(dropping, (dropping->rotation + (ch != 'z' ? 1 : -1)) % 4, ch == 'z' ? 1 : 0);
    }
    if (ch == ' ') setDropped(dropping, 1);
    if (ch == 'c') holdPiece(dropping);
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

    reCheckLevel();
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

    struct timespec deadline;
    const long frame_ns = 1000000000L / 100;

    clock_gettime(CLOCK_MONOTONIC, &deadline);

    while (playing) { //Game loop
	printBoard();

	int tryDrop = -1;
	//If frame == 0, then drop for real.
	if ( !paused && dropping != NULL && (frames == 0) ) tryDrop = tryMoveTetromino(dropping, (Vector2i){.x=0,.y=1}, 0);
	//Check if after drop we are touching the ground
	int dryDropCheck = tryMoveTetromino(dropping, (Vector2i){.x=0,.y=1}, 1);
	//If frame != 0, but grace time expired, do the drop anyway.
	if (tryDrop < 0 && grace == 1 && realSeconds == 0 && dryDropCheck == 1) tryDrop = 1;
	if (tryDrop == 0 && dryDropCheck == 0) { //If can drop, then stop grace timer.
	    grace = 0;
	    if (graceAdds < GRACE_LIMIT) realSeconds = 1; //Prevent rotate abuse
	} else if (tryDrop >= 0) { //If cant drop, start grace timer.
	    if (grace == 1 && realSeconds == 0 && tryDrop == 1) { setDropped(dropping, 0); } 
	    else if (grace == 0) {
		grace = 1;
		if (graceAdds < GRACE_LIMIT) realSeconds = 1; //Prevent rotate abuse
	    } 
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
