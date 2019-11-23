/*****
visualscript.h: script for visual.c
*****/

#ifndef VISUALSCRIPT_H
#define VISUALSCRIPT_H

/*max line(s) of scriptboxes*/
#define IB_LINE_MAX 18
#define NB_LINE_MAX 6
#define MB_LINE_MAX 24
/*max length of one scriptbox string*/
#define SB_LEN_MAX 28

/*Scriptbox strings*/
extern char infobox[IB_LINE_MAX*SB_LEN_MAX];
extern char notibox[NB_LINE_MAX*SB_LEN_MAX];
extern char menubox[MB_LINE_MAX*SB_LEN_MAX];

/*Color of notibox, in hex value*/
extern int infobox_color;

/*to change size of pieces*/
extern float piecesize_mult;
#if !defined (PLATFORM_WEB)
/*fps to draw on infobox*/
extern int fps;
#endif

/*Update scriptbox strings.*/
void UpdateInfoBox(float r, float theta, float phi);
void UpdateNotiBox(char *newnoti);
void UpdateMenuBox(int key); //key == 0, 1, ... 9 for update

/*Scripts*/
/*When game started. Returns -1 if fails.*/
int GameStarted(void);
/*When a piece is clicked. coord == x*100 + y*10 + z*/
void PieceClicked(int coord);

#endif