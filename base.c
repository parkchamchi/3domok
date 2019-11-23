/*****
base.c: Base program source file
*****/

#include <stdlib.h> //only for allocation
#include "base.h"

/*lines of grid. Init. 4*/
int lines=4;
/*Amound of successive pieces needed for the winning condition. No overline. Init. 4*/
int win_cond=4;

/*Global grid; allowed values are only those of enum values. [z][y][x]*/
short ***grid=NULL;
/*Count of the pieces deployed*/
int piececount;

/*Used in CheckStatus(). Check winning status in given line, returns enum value values. Returns -1 if error is occured.*/
static int CheckLine(int x, int y, int z, int reset);

/*Initialize the grid. Returns -1 if an error occurs.*/
int InitGrid(void) {
	int z, y;
	static int former_lines; //for freeing grid

	/*Free the grid*/
	if (grid) {
		for (z=0; z < former_lines; z++) {
			for (y=0; y < former_lines; y++)
				free(grid[z][y]); //free x
			free(grid[z]); //free y
		}
		free(grid); //free x
	}
		
	/*Alloc the grid*/
	grid = (short ***) calloc(lines, sizeof (short **)); //alloc z
	if (!grid) return -1;

	for (z=0; z < lines; z++) {
		grid[z] = (short **) calloc(lines, sizeof(short *)); //alloc y
		if (!grid[z]) return -1;

		for (y=0; y< lines; y++) {
			grid[z][y] = (short *) calloc(lines, sizeof (short)); //alloc x
			if (!grid[z][y]) return -1;
		}
	}
	
	piececount=0;
	former_lines=lines;
	return 0;
}

/*Input a piece, returns 0 if successful. Returns -1 if coord is invalid. Returns -2 if coord is alredy used.*/
int InputPiece(enum values value, int coord) {
	int z=coord%10;
	int y=coord%100/10;
	int x=coord/100;

	/*Check if coord is valid*/
	if (x < 0 || lines <= x || y < 0 || lines <= y || z < 0 || lines <= z) //invalid coord
		return -1;		

	/*check if the piece is already placed in the coord*/
	if (grid[z][y][x] == P0 || grid[z][y][x] == P1)
		return -2;

	/*Input*/
	grid[z][y][x]=value;
	piececount++;
	return 0;
}

/*Delete a piece.*/
void DeletePiece(int coord) {
	int z=coord%10;
	int y=coord%100/10;
	int x=coord/100;

	/*Check if coord is valid*/
	if (x < 0 || lines <= x || y < 0 || lines <= y || z < 0 || lines <= z) //invalid coord
		return;

	/*Detele*/
	if (!grid[z][y][x]) return;
	grid[z][y][x]=NONE;
	piececount--;
}

/*Checks if winning status is reached; returns P0 if player0 wins and P1 if player1 wins. Returns -1 if error occures.
Returns -2 if no more piece can be deployed. Returns 0 for normal status.*/
int CheckStatus(void) {
	int x, y, z;
	int x_iter, y_iter, z_iter;
	int win; //if winning condition is achived. This is to check overline.

	/*Check if the grid is full*/
	if (piececount > MAXPIECECOUNT)
		return -2;

	//////////////////////////////////////////////////////////////////
	//CHECK 1D
	//////////////////////////////////////////////////////////////////

	/*Check x*/
	for (z=0; z < lines; z++) {
		for (y=0; y < lines; y++) {
			/*Reset CheckLine() vars*/
			CheckLine(0, 0, 0, 1);
			for (x=0; x < lines; x++) {
				win=CheckLine(x, y, z, 0);
				/*Winning condition without overline.*/
				if (win == P0*10 || win == P1*10)
					return win/10;

				/*ERROR*/
				if (win == -1)
					return -1;
			}

			if (win)
				return win;
		}
	}

	/*Check y*/
	for (z=0; z < lines; z++) {
		for (x=0; x < lines; x++) {
			/*Reset CheckLine() vars*/
			CheckLine(0, 0, 0, 1);
			for (y=0; y < lines; y++) {
				if ((win=CheckLine(x, y, z, 0)) == P0*10 || win == P0*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

			if (win) 
				return win;
		}
	}

	/*Check z*/
	for (x=0; x < lines; x++) {
		for (y=0; y < lines; y++) {
			CheckLine(0, 0, 0, 1);
			for (z=0; z < lines; z++) {
				if ((win=CheckLine(x, y, z, 0)) == P0*10 || win == P0*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

			if (win) 
				return win;
		}
	}

	//////////////////////////////////////////////////////////////////
	//CHECK 2D
	//////////////////////////////////////////////////////////////////
	
	/*Check xy, increasing x*/
	for (z=0; z < lines; z++) {
		for (y=lines-1; y >= 0; y--) {
			CheckLine(0, 0, 0, 1);
			for (x=0, y_iter=y; x < lines && y_iter < lines; x++, y_iter++) {
				if ((win=CheckLine(x, y_iter, z, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

			if (win)
				return win;
		}
	}

	/*Check xy, decreasing x*/
	for (z=0; z < lines; z++) {
		for (y=lines-1; y >= 0; y--) {
			CheckLine(0, 0, 0, 1);
			for (x=lines-1, y_iter=y; x >= 0 && y_iter < lines; x--, y_iter++) {
				if ((win=CheckLine(x, y_iter, z, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

			if (win)
				return win;
		}
	}

	/*Check yz, increasing y*/
	for (x=0; x < lines; x++) {
		for (z=lines-1; z >= 0; z--) {
			CheckLine(0, 0, 0, 1);
			for (y=0, z_iter=z; y < lines && z_iter < lines; y++, z_iter++) {
				if ((win=CheckLine(x, y, z_iter, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

			if (win)
				return win;
		}
	}

	/*Check yz, decreasing y*/
	for (x=0; x < lines; x++) {
		for (z=lines-1; z >= 0; z--) {
			CheckLine(0, 0, 0, 1);
			for (y=lines-1, z_iter=z; y >= 0 && z_iter < lines; y--, z_iter++) {
				if ((win=CheckLine(x, y, z_iter, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

			if (win)
				return win;
		}
	}

	/*Check zx, increasing z*/
	for (y=0; y < lines; y++) {
		for (x=lines-1; x >= 0; x--) {
			CheckLine(0, 0, 0, 1);
			for (z=0, x_iter=x; z < lines && x_iter < lines; z++, x_iter++) {
				if ((win=CheckLine(x_iter, y, z, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

			if (win)
				return win;
		}
	}

	/*Check zx, decreasing z*/
	for (y=0; y < lines; y++) {
		for (x=lines-1; x >= 0; x--) {
			CheckLine(0, 0, 0, 1);
			for (z=lines-1, x_iter=x; z >= 0 && x_iter < lines; z--, x_iter++) {
				if ((win=CheckLine(x_iter, y, z, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

			if (win)
				return win;
		}
	}

	//////////////////////////////////////////////////////////////////
	//Check 3D
	//////////////////////////////////////////////////////////////////

	/*Check xyz, x++, y++, z++*/
	for (x=0; x < lines; x++) {
		for (y=0; y < lines; y++) {
			CheckLine(0, 0, 0, 1);
			for (z=0, x_iter=x, y_iter=y; z < lines && x_iter < lines && y_iter < lines; z++, x_iter++, y_iter++) {
				if ((win=CheckLine(x_iter, y_iter, z, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

		if (win)
			return win;
		}
	}

	/*Check xyz, x++, y++, z--*/
	for (x=0; x < lines; x++) {
		for (y=0; y < lines; y++) {
			CheckLine(0, 0, 0, 1);
			for (z=lines-1, x_iter=x, y_iter=y; z >= 0 && x_iter < lines && y_iter < lines; z--, x_iter++, y_iter++) {
				if ((win=CheckLine(x_iter, y_iter, z, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

		if (win)
			return win;
		}
	}

	/*Check zxy, x--, y++, z--*/
	for (z=0; z < lines; z++) {
		for (x=0; x < lines; x++) {
			CheckLine(0, 0, 0, 1);
			for (y=0, x_iter=x, z_iter=z; y < lines && x_iter >= 0 && z_iter >= 0; y++, x_iter--, z_iter--) {
				if ((win=CheckLine(x_iter, y, z_iter, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

		if (win)
			return win;
		}
	}

	/*Check zxy, x--, y++, z++*/
	for (z=0; z < lines; z++) {
		for (x=0; x < lines; x++) {
			CheckLine(0, 0, 0, 1);
			for (y=0, x_iter=x, z_iter=z; y < lines && x_iter >= 0 && z_iter < lines; y++, x_iter--, z_iter++){
				if ((win=CheckLine(x_iter, y, z_iter, 0)) == P0*10 || win == P1*10) return win/10; //No overline.
				if (win == -1) return -1; //Error.
			}

		if (win)
			return win;
		}
	}

	/*Normal*/
	return 0;
}

/*Used in CheckStatus(). Check winning status in given line, returns enum value values. 
Returns (enum value values)*10 if winning status with no overline; no more processing is required.
Returns -1 if error is occured.*/
static int CheckLine(int x, int y, int z, int reset) {
	static int win; //if winning condition is achived. This is to check overline.
	static int successive;
	static enum values preval=NONE;

	/*RESET*/
	if (reset) {
		win=successive=preval=0;
		return 0;
	}

	/*check overline*/
	if (win) {
		if (grid[z][y][x] == preval) { //Overline!
			win=0;
			successive++;
			return 0;
		}
		else { //No overline, actual winning
			return win*10;
		}
	}

	switch (grid[z][y][x]) {
	case NONE:
		successive=0;
		break;
	case P0:
		if (preval == P0) 
			successive++;
		else
			successive=1;
		break;
	case P1:
		if (preval == P1)
			successive++;
		else
			successive=1;
		break;
	default:
		/*ERROR*/
		return -1;
		break;
	}

	if (successive == win_cond)
		win=preval;
	preval=grid[z][y][x];

	return win;
}