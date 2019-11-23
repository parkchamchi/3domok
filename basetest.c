/*****
basetest.c: to test base program in console interface.
*****/

#include <stdio.h>
#include <stdlib.h>
#include "base.h"

void PrintMenu(void);
/*Show grid in 2D display.*/
void ShowGrid(void);

void Save(void);
/*Returns -1 when failed.*/
int Load(void);

int main(void) {
	int input;
	enum values current_player=P0;
	int inputres, result;
	int score[2]={0};
	int isdebugmode=0;

	PrintMenu();

	/*Initial grid intiation*/
	InitGrid();

	while (1) {
		ShowGrid();

		/*input coord*/
		while (1) {
			if (isdebugmode)
				current_player=P0;
			printf("\nplayer%d(%c): ", (current_player == P0) ? 0 : 1, current_player);
			scanf("%d", &input);

			/*quit*/
			if (input == -1) {
				printf("Terminating...\n");
				break;
			}
			switch (input) {
				int di, dj, dk;
			/*Debug set: set all grids except outer ones to P0, and toggle debug mode*/
			case -2:
				for (di=1; di < lines-1; di++)
					for (dj=1; dj < lines-1; dj++)
						for (dk=1; dk < lines-1; dk++)
							grid[di][dj][dk]=P0;
				piececount=(lines-2)*(lines-2)*(lines-2);
				
				isdebugmode=1;
				printf("Debug set:\n");
				ShowGrid();
				continue;
			/*Debug mode: player is fixed to P0.*/
			case -3:
				if (isdebugmode) {
					isdebugmode=0;
					printf("Debug mode canceled.\n");
				}
				else {
					isdebugmode=1;
					printf("Debug mode activated.\n");
				}
				continue;
			/*Clear the grid.*/
			case -4:
				InitGrid();
				printf("Grid has been cleared.\n");
				ShowGrid();
				continue;
			/*Change game rules.*/
			case -5:
				printf("New lines (0 to pass): ");
				scanf("%d", &input);
				if (input > LINESMAX)
					printf("Var lines should not be bigger than %d. Discarded...\n", LINESMAX);
				else if (input < 2 && input != 0)
					printf("Invalid input. Discarded...\n");
				else
					lines=input;
				
				printf("New win_cond (0 to pass): ");
				scanf("%d", &input);
				if (input > lines) {
					printf("Var win_cond should not be bigger than lines. ");
					if (win_cond <= lines)
						printf("Discarded...\n");
					else {
						printf("It has been set to the val of var lines.\n");
						win_cond=lines;
					}
				}
				else if (input < 0)
					printf("Invalid input. Discarded...\n");
				else
					win_cond=input;

				/*Allocation*/
				if (InitGrid()) {//Allocation failed
					printf("Err: InitGrid() couldn't the allocate grid\n");
					printf("Terminating...\n");
					exit(1);
				}
				printf("Grid has been reset.\n");
				ShowGrid();
				continue;
			/*Save.*/
			case -6:
				Save();
				continue;
			/*Load.*/
			case -7:
				if (Load() == 0)
					printf("Loaded.\n");
				ShowGrid();
				continue;
			/*Print the menu again.*/
			case -10:
				PrintMenu();
				continue;
			}
			
			if ((inputres=InputPiece(current_player, input)) == -1)
				printf("%03d is invalid coordinate.\n", input);
			else if (inputres == -2)
				printf("%03d is already used.\n", input);
			else
				break;
		}
		if (input == -1)
			break;

		/*Check status*/
		result=CheckStatus();
		switch (result) {
		case 0:
			printf("Normal status.\n");
			current_player = (current_player == P0) ? P1 : P0; //switches player.
			break;
		case P0:
			printf("Player0 Wins!\n"); //In other cases, the loser inputs first next round.
			score[0]++;
			ShowGrid();
			printf("Game has been reset.\n");
			InitGrid();
			break;
		case P1:
			printf("Player1 Wins!\n");
			score[1]++;
			ShowGrid();
			printf("Game has been reset.\n");
			InitGrid();
			break;
		case -1:
			printf("An error has been occured.\n");
			ShowGrid();
			printf("Game has been reset.\n");
			InitGrid();
			break;
		case -2:
			printf("Draw; grid if full.\n");
			ShowGrid();
			printf("Game has been reset.\n");
			InitGrid();
			break;
		/*Error; quits*/
		default:
			printf("Err: CheckStatus() returned invalid value.\n");
			printf("Terminating...");
			exit(1);
			break;
		}
	}

	printf("P0: %d, P1: %d\n", score[0], score[1]);
	printf("Terminating normaly.\n");
	return 0;
}

void PrintMenu(void) {
	puts("basetest.c running; it is to test base program in console interface.\n");
	puts("Input coordinate to input a piece, -1 to quit.");
	puts("Input -2 to get the debug set; -3 to toggle debug mode.");
	puts("Input -4 to clear.");
	puts("Input -5 to reset and change the rules.");
	puts("Input -6 to save.");
	puts("Input -7 to load.");
	puts("Input -10 to show this menu again.");
	putchar('\n');
}

void ShowGrid(void) {
	int zi, yi, xi;

	for (zi=0; zi < lines; zi++) {
		printf("z=%d\n", zi);
		for (yi=0; yi < lines; yi++) {
			printf("y=%d ", yi);
			for (xi=0; xi < lines; xi++) {
				printf("x=%d<%c> ", xi, grid[zi][yi][xi]);
			}
			printf("\n");
		}
		printf("\n");
	}
}

void Save(void) {
	FILE *savefile=fopen("save.bin", "wb");
	fwrite(&lines, sizeof (lines), 1, savefile); //lines
	fwrite(&win_cond, sizeof (win_cond), 1, savefile); //wind_cond
	
	for (int z=0; z < lines; z++) //grid
		for (int y=0; y < lines; y++)
			fwrite(grid[z][y], sizeof (short), lines, savefile);

	fclose(savefile);
}

int Load(void) {
	FILE *savefile=fopen("save.bin", "rb");
	if (!savefile) {
		printf("No save file found.\n");
		return -1;
	}

	fread(&lines, sizeof (lines), 1, savefile); //lines
	fread(&win_cond, sizeof (win_cond), 1, savefile); //wind_cond
	InitGrid(); //to alloc the grid.
	
	for (int z=0; z < lines; z++) //grid
		for (int y=0; y < lines; y++)
			fread(grid[z][y], sizeof (short), lines, savefile);

	fclose(savefile);
	return 0;
}