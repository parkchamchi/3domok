/*****
visualscript.c: script for visual.c
*****/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "visualscript.h"
#include "base.h"

//#define USEGAMEMODE //Toggle this one
#ifdef USEGAMEMODE
#include <jni.h>
#include <jni_md.h>
#include <stdlib.h> //for exit()

JNIEnv *env;
jclass cls;
jmethodID set_id, put_id, reset_id;

int gamemode=0;

static void Create_vm(void);
static void ChangeGamemode(int gm);
static void PutGamemode(int coord); //returns -1 when there's nothing to delete.
static void ResetGamemode(void);
#endif

#define TIMESTRLEN 30

#define EVALUES2INT(EV) (((EV) == P0) ? (0) : (1))
#define COORD2Z(COORD) ((COORD)%10)
#define COORD2Y(COORD) ((COORD)%100/10)
#define COORD2X(COORD) ((COORD)/100)

/*Scriptbox strings*/
char infobox[IB_LINE_MAX*SB_LEN_MAX];
char notibox[NB_LINE_MAX*SB_LEN_MAX];
char menubox[MB_LINE_MAX*SB_LEN_MAX];

/*Color of infobox, in hex value*/
int infobox_color;

static enum values cur_player=P0;
static int score[2];

static clock_t start_clock=-1;
static int timelimit=60; //sec.

static void ResetGrid(int confirm);
static int ChangeGameRules(int new_val);
static void ChangeTimelimit(int new_timelimit);
#if !defined (PLATFORM_WEB)
static void SaveTheGame(int confirm);
static void LoadTheGame(int confirm, FILE *save);
#endif
static void ChangePiecesizeMult(int size);
static void SwapPlayers(int confirm);
static int InputCoord(int c); //returns 0 when done

static void ChangeLines(int new_lines);
static void ChangeWinCond(int new_win_cond);

void UpdateInfoBox(float r, float theta, float phi) {
	
	clock_t cur_clock=clock();
	int timer = ((float) (cur_clock-start_clock) / CLOCKS_PER_SEC);
	static enum values cur_cur_player=NONE;

	char tempstr[SB_LEN_MAX];

	/*Set timer*/
	if (start_clock == -1 || cur_cur_player != cur_player) { //also initialize
		start_clock=clock();
		timer=timelimit;
		cur_cur_player=cur_player;
	}
	else if (cur_player != NONE && timer >= timelimit) {
		UpdateNotiBox("Timeover!\n");
		cur_player = (cur_player == P0) ? P1 : P0;
	} 

	sprintf(infobox,
	#if !defined (PLATFORM_WEB) 
		"%2d FPS\n"
	#else
		"HTML version\n"
	#endif
		"3D OMOK alpha v0.1\n"
		"By parkchamchi@gmail.com\n"
		"- Game Rules -\n"
		"Lines: %d\n"
		"Winning condition: %d\n"
		"- Score -\n"
		"P0 (RED): %d, P1 (BLUE): %d\n"
		"- Camera -\n"
		"r: %.2f, th: %.2f, ph: %.2f\n"
		"\n",
	#if !defined (PLATFORM_WEB)
		fps,
	#endif 
		lines, win_cond, 
		score[0], score[1],
		r, theta, phi
	);

	if (cur_player != NONE) {
		sprintf(tempstr, "Time for P%d left: %2ds\n", EVALUES2INT(cur_player), timelimit-timer);
		strcat(infobox, tempstr);
	}

	infobox_color = (cur_player == P0) ? (0xff000033) : ((cur_player == P1) ? (0x0000ff33) : (0xffffff33));
}

void UpdateNotiBox(char *newnoti) {
	static char notis[NB_LINE_MAX][SB_LEN_MAX]={{0}};
	static int notiscount=0;

	if (!newnoti) return; //no new notification
	
	strcpy(notis[notiscount%NB_LINE_MAX], newnoti);

	strcpy(notibox, ""); //reset notibox
	if (notiscount < NB_LINE_MAX) { //if notis is not full yet
		for (int i=0; i <= notiscount; i++)
			strcat(notibox, notis[i]);
	}
	else { //when notis is full
		for (int i=0; i < NB_LINE_MAX; i++)
			strcat(notibox, notis[(notiscount+1+i)%NB_LINE_MAX]); //starting from the next noti from newnoti 
	}

	notiscount++;
}

void UpdateMenuBox(int key) {
	static int menu;
	int notdone=0;
	
#if !defined (PLATFORM_WEB)
	static FILE *save=NULL;
	char timestr[TIMESTRLEN];
#endif

	const char *DEFMENU =
		"WASD | KP8456:\n Move the grid\n"
		"LCLICK: Select a piece\n"
		"QE | KP79, MWHEEL:\n Zoom in/out\n"
		"ESC: Exit\n"
		"-----------------------\n"
		"1. Reset the game.\n"
		"2. Change game rules.\n"
		"3. Change the timelimit.\n"
	#ifdef USEGAMEMODE
		"4. Change the gamemode.\n"
	#endif
	#if !defined (PLATFORM_WEB)
		"5. Save the game.\n"
		"6. Load the game.\n"
	#endif
		"7. Change the piece size.\n"
		"8. Swap players.\n"
		"9. Insert a coordinate.\n"
		;

	if (!(menubox[0])) strcpy(menubox, DEFMENU); //initialize

	if (!(0 <= key && key <= 9)) return;

	/*Show submenu*/
	if (menu == 0) {
		switch (menu=key) {
		case 0:
			break;
		case 1: //Reset the game.
			strcpy(menubox, 
				"Will you reset the game?\n"
				"0: No.\n"
				"1: Yes.\n"
			);
			break;
		case 2: //Change game rules.
			strcpy(menubox,
				"Insert new lines.\n"
				"(2 <= lines <= 9)\n"
				"Then insert new win.cond.\n"
				"(2 <= win.c. <= lines)\n"
			);
			break;
		case 3: //Change the timelimit
			strcpy(menubox,
			"Insert new timelimit/10.\n"
			"(0 < timelimit/10)\n"
			);
			break;
	#ifdef USEGAMEMODE
		case 4: //Change the gamemode
			strcpy(menubox,
				"0: Default.\n"
				"1. Volatile.\n"
			);
			break;
	#endif
	#if !defined (PLATFORM_WEB)
		case 5: //Save the game.
			strcpy(menubox,
				"Will you save the game?\n"
				"0: No.\n"
				"1: Yes.\n"
			);
			break;
		case 6: //Load the game.
			save=fopen("save.bin", "rb");
			if (!save) {
				UpdateNotiBox("Savefile not found.\n");
				menu=0;
				break;	
			}

			fread(timestr, sizeof (timestr[0]), TIMESTRLEN, save);
			sprintf(menubox, " Saved when:\n%s\n", timestr);
			strcat(menubox,
				"Will you load the game?\n"
				"0: No.\n"
				"1: Yes.\n"
			);
			break;
	#endif
		case 7: //Change the picece size.
			strcpy(menubox, 
				"Change the piece size?\n"
				"0: No.\n"
				"1: To small.\n"
				"2: To default.\n"
				"3: To big.\n"
			);
			break;
		case 8: //Swap players.
			strcpy(menubox, 
				"Swap players?\n"
				"This only applys when\n"
				"Required: 5 pieces located.\n"
				"0: No.\n"
				"1: Yes.\n");
			break;
		case 9: //Insert a coordinate.
			strcpy(menubox, "Insert: ");
			break;
		}
	}
	/*When already in submenu*/
	else {
		switch (menu) {
		case 1:
			ResetGrid(key);
			break;

		case 2:
			notdone=ChangeGameRules(key);
			break;

		case 3:
			ChangeTimelimit(key);
			break;

	#ifdef USEGAMEMODE
		case 4:
			ChangeGamemode(key);
			break;
	#endif

	#if !defined (PLATFORM_WEB)
		case 5:
			SaveTheGame(key);
			break;

		case 6:
			LoadTheGame(key, save);
			break;
	#endif

		case 7:
			ChangePiecesizeMult(key);
			break;

		case 8:
			SwapPlayers(key);
			break;

		case 9:
			notdone=InputCoord(key);
			break;
		}	
		if (!notdone) {
			menu=0; //return to main menu
			strcpy(menubox, DEFMENU);
		}
	}
}

/*When game started. Returns -1 if fails.*/
int GameStarted(void) {
	if (InitGrid() == -1) return -1;

#ifdef USEGAMEMODE
	Create_vm();
#endif
	UpdateNotiBox("Game has been started!\n");
	return 0;
}

/*When a piece is clicked. coord == x*100 + y*10 + z*/
void PieceClicked(int coord) {
	char tempstr[SB_LEN_MAX];
	int status=0;

	if (cur_player == NONE) {
		return;
	}

	status=InputPiece(cur_player, coord);
	switch (status) { //error
	case 0: //normal status
		break;
	case -1:
		sprintf(tempstr, "ERROR: invalid coord.\n");
		UpdateNotiBox(tempstr);
		return;
		break;
	case -2:
		sprintf(tempstr, "%d%d%d is already occupied.\n", COORD2X(coord), COORD2Y(coord), COORD2Z(coord));
		UpdateNotiBox(tempstr);
		return;
		break;
	default:
		sprintf(tempstr, "ERROR: unknown(PC).\n");
		UpdateNotiBox(tempstr);
		return;
		break;
	}

#ifdef USEGAMEMODE
	PutGamemode(coord);
#endif
	sprintf(tempstr, "P%d selected %d%d%d.\n", EVALUES2INT(cur_player), COORD2X(coord), COORD2Y(coord), COORD2Z(coord));
	UpdateNotiBox(tempstr);

	switch (status=CheckStatus()) {
	case P0:
	case P1:
		sprintf(tempstr, "P%d has won!\n", EVALUES2INT(cur_player));
		UpdateNotiBox(tempstr);

		score[EVALUES2INT(cur_player)]++;
		cur_player=NONE;

		break;
	case -1:
		sprintf(tempstr, "ERROR: checking status\n");
		UpdateNotiBox(tempstr);
		break;
	case -2:
		sprintf(tempstr, "Grid is full!\n");
		UpdateNotiBox(tempstr);
		cur_player=NONE;
		break;
	}
	
	if (cur_player != NONE) cur_player = (cur_player == P0) ? P1 : P0;
}

static void ResetGrid(int confirm) {
	if (confirm != 1) return;

	if (InitGrid() == -1) {
		UpdateNotiBox("ERROR: InitGrid Fail\n");
		return;
	}

#ifdef USEGAMEMODE
	ResetGamemode();
#endif

	cur_player=P0;
	UpdateNotiBox("Game has been reset.\n");
}

static int ChangeGameRules(int new_val) {
	static int status=0;
	static int new_lines, new_win_cond;
	
	if (status == 0) {
		new_lines=new_val;
		UpdateNotiBox("Got new lines.\n");
		status=1;
		return 1;
	}

	new_win_cond=new_val;
	UpdateNotiBox("Got new win. cond.\n");
	status=0;

	ChangeLines(new_lines);
	ChangeWinCond(new_win_cond);
	ResetGrid(1);
	return 0;
}

static void ChangeTimelimit(int new_timelimit) {
	if (new_timelimit == 0) {
		UpdateNotiBox("Invalid timelimit value.\n");
		return;
	}

	timelimit=new_timelimit*10;
	UpdateNotiBox("Timelimit changed.\n");
}

#if !defined (PLATFORM_WEB)

static void SaveTheGame(int confirm) {
	FILE *save=NULL;
	time_t savetime;
	char timestr[TIMESTRLEN];
	
	if (confirm != 1) return;

	save=fopen("save.bin", "wb");
	savetime = time(NULL);
	strcpy(timestr, ctime(&savetime));

	fwrite(timestr, sizeof (timestr[0]), TIMESTRLEN, save);

	fwrite(&lines, sizeof (lines), 1, save);
	fwrite(&win_cond, sizeof (win_cond), 1, save);
	for (int z=0; z < lines; z++)
		for (int y=0; y < lines; y++)
			fwrite(grid[z][y], sizeof (grid[0][0][0]), lines, save);
	fwrite(&piececount, sizeof (piececount), 1, save);
	
	fwrite(&cur_player, sizeof (cur_player), 1, save);
	fwrite(score, sizeof (score[0]), 2, save);
	fwrite(&timelimit, sizeof (timelimit), 1, save);

	fclose(save);

	UpdateNotiBox("Game saved.\n");
}

static void LoadTheGame(int confirm, FILE *save) {
	if (confirm != 1) {
		fclose(save);
		return;
	}

	fread(&lines, sizeof (lines), 1, save);
	fread(&win_cond, sizeof (win_cond), 1, save);
	ResetGrid(1);

	for (int z=0; z < lines; z++)
		for (int y=0; y < lines; y++)
			fread(grid[z][y], sizeof (grid[0][0][0]), lines, save);
	fread(&piececount, sizeof (piececount), 1, save);
	
	fread(&cur_player, sizeof (cur_player), 1, save);
	fread(score, sizeof (score[0]), 2, save);
	fread(&timelimit, sizeof (timelimit), 1, save);

	fclose(save);

	start_clock=clock();
	UpdateNotiBox("Game has been loaded.\n");	
}	

#endif

static void ChangePiecesizeMult(int size) {
	if (!size) return;

	switch (size) {
	case 1:
		piecesize_mult=0.75f;
		break;
	case 2:
		piecesize_mult=1.0f;
		break;
	case 3:
		piecesize_mult=1.25f;
		break;
	default:
		return;
		break;
	}

	UpdateNotiBox("Piece size changed.\n");
	return;
}	

static void SwapPlayers(int confirm) {
	if (confirm != 1) return;
	
	if (piececount != 5) {
		UpdateNotiBox("There should be 5 pieces.\n");
		return;
	}

	for (int z=0; z < lines; z++)
		for (int y=0; y < lines; y++)
			for (int x=0; x < lines; x++)
				if (grid[z][y][x])
					grid[z][y][x] = (grid[z][y][x] == P0) ? P1 : P0;
	
	UpdateNotiBox("Swapped players.\n");
	cur_player = (cur_player == P0) ? P1 : P0;
}

static int InputCoord(int c) {
	static int coord;
	static int count;

	char tempstr[SB_LEN_MAX];

	if (c < 0 || c >= lines) {
		UpdateNotiBox("Invalid coordinate.\n");
		coord=count=0;
		return 0;
	}

	coord = coord*10 + c;
	count++;
	sprintf(tempstr, "Got %c == %d.\n", (count == 1) ? 'x' : (count == 2) ? 'y' : 'z', c);
	UpdateNotiBox(tempstr);

	if (count >= 3) {
		PieceClicked(coord);
		coord=count=0;
		return 0;
	}
	else 
		return 1;
}

static void ChangeLines(int new_lines) {
	if (new_lines < 2 || new_lines > 9) {
		UpdateNotiBox("Invalid lines value.\n");
		return;
	}

	lines=new_lines;
	UpdateNotiBox("Lines value changed.\n");
	if (win_cond > lines) {
		win_cond=lines;
		UpdateNotiBox("Win.c. changed to lines.\n");
	}

	return;
}

static void ChangeWinCond(int new_win_cond) {
	if (new_win_cond < 2 || new_win_cond > lines) {
		UpdateNotiBox("Invalid win.c. value.\n");
		return;
	}

	win_cond=new_win_cond;
	UpdateNotiBox("Winning condition changed.\n");
}

#ifdef USEGAMEMODE

static void Create_vm(void) {
    JavaVM *vm;

    JavaVMInitArgs vm_args;
	jint res;

	vm_args.version = JNI_VERSION_1_8; //JDK version. This indicates version 1.6
    vm_args.nOptions = 0;
    
    res = JNI_CreateJavaVM(&vm, (void **)&env, &vm_args);
    if(res != JNI_OK) exit(1);
    
	cls = (*env)->FindClass(env, "Gamemode");
	if (cls == NULL) exit(1);

	set_id = (*env)->GetStaticMethodID(env, cls, "set", "(I)I");
	if (set_id == NULL) exit(2);
	put_id = (*env)->GetStaticMethodID(env, cls, "put", "(I)I");
	if (put_id == NULL) exit(2);
	reset_id = (*env)->GetStaticMethodID(env, cls, "reset", "()V");
	if (reset_id == NULL) exit(2);
}

static void ChangeGamemode(int gm) {
	if (gm != 0 && gm != 1) {
		UpdateNotiBox("Invalid gamemode.\n");
		return;
	}

	switch ((*env)->CallStaticIntMethod(env, cls, set_id, gm)) {
	case -1:
		UpdateNotiBox("Err: gamemode change fail.\n");
		break;
	default:
		UpdateNotiBox("Gamemode set.\n");
		gamemode=gm;
		break;
	}
}

static void PutGamemode(int coord) {
	int todel = (*env)->CallStaticIntMethod(env, cls, put_id, coord);

	if (todel < 0) return;

	DeletePiece(coord);
	UpdateNotiBox("A piece deleted.\n");
}

static void ResetGamemode(void) {
	(*env)->CallStaticVoidMethod(env, cls, reset_id);
}
#endif