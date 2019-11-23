/*****
base.h: Base program header file
*****/

#ifndef BASE_H
#define BASE_H

/*lines of grid. Init. 4*/
extern int lines;
/*Max lines*/
#define LINESMAX 9
/*Amount of successive pieces needed for the winning condition. No overline. Init. 4*/
extern int win_cond;
/*Max piececount*/
#define MAXPIECECOUNT lines*lines*lines

/*Allowed values in grid*/
enum values {NONE='\0', P0='O', P1='X'};

/*Global grid; allowed values are only those of enum values. [z][y][x]*/
extern short ***grid;
/*Count of the pieces deployed*/
extern int piececount;

/*Initialize the grid. Returns -1 if an error occurs.*/
int InitGrid(void);
/*Input a piece, returns 0 if successful. Returns -1 if coord is invalid. Returns -2 if coord is alredy used.*/
int InputPiece(enum values value, int coord);
/*Delete a piece.*/
void DeletePiece(int coord);
/*Checks if winning status is reached; returns P0 if player0 wins and P1 if player1 wins. Returns -1 if error occures.
Returns -2 if no more piece can be deployed. Returns 0 for normal status.*/
int CheckStatus(void);

#endif