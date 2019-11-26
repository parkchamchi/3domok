/*****
visual.c: the visualization program
*****/

#include <stdlib.h> //for allocation
#include <string.h> //for floating text
#include <assert.h> //for checking LINESMAX
#include <math.h> //for camera

#if defined (PLATFORM_WEB) //for web version
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include "raylib.h"
#include "base.h"
#include "visualscript.h"

#if defined (PLATFORM_WEB)
#define USECUBES //if the game would use cubes instead of spheres
#endif
//#define USECUBES //Toggle this one

#define PI_F 3.141592f //float value of pi, as FLT_DIG >= 6

#define GRIDLEN 30 //length of the grid, excluding the end of the axis.
#define PIECESPACING ((float) GRIDLEN / (lines-1)) //space between pieces. Init. 10
#define PIECESIZE ((float) PIECESPACING / 5) //size of a piece, Init. 2.0f
#define CUBESIZE PIECESIZE*1.25f //for web version. size of cube

/*Position of pieces in screen, used to draw text around pieces*/ 
#define PIECESCREENPOS(X, Y, Z, CAM) \
	(GetWorldToScreen((Vector3) {piecepos[(Z)][(Y)][(X)].x, piecepos[(Z)][(Y)][(X)].y, piecepos[(Z)][(Y)][(X)].z}, (CAM)))
/*Color of pieces*/
#define PIECECOLOR(X, Y, Z) \
	((grid[(Z)][(Y)][(X)] == NONE) ? (Fade(GRAY, 0.50f)) : ((grid[(Z)][(Y)][(X)] == P0) ? (Fade(RED, 0.75f)) : (Fade(BLUE, 0.75f))))
/*Get the position of the camera*/
#define GETCAMERAPOS(POS_P, R, THETA, PHI) { \
	(POS_P)->x = (R) * sinf(THETA) * cosf(PHI); \
	(POS_P)->y = (R) * sinf(THETA) * sinf(PHI); \
	(POS_P)->z = (R) * cosf(THETA); \
}
/*Get square of the difference of two Vector3's*/
#define VECTOR3_DIFF(V1, V2) \
	(pow(((V1).x - (V2).x), 2) + pow(((V1).y - (V2).y), 2) + pow(((V1).z - (V2).z), 2))

/*Screen size*/
const int SCREENWIDTH = 1280;
const int SCREENHEIGHT = 720;

/*to change size of pieces*/
float piecesize_mult=1.0f;
#if !defined (PLATFORM_WEB)
/*fps to draw on infobox*/
int fps;
#endif

/*The camera*/
static Camera camera = {0};
/*Positions of pieces, [z][y][x]*/
static Vector3 ***piecepos;
/*End of axis*/
static Vector3 x_axis_end, y_axis_end, z_axis_end;
/*The value of lines used in piecepos*/
static int cur_lines;

#ifdef USECUBES
/*Boxes of the cubes of piecepos[z][y][x] for web version*/
static BoundingBox ***piececube;
#endif

/*Camera positions*/
static float cam_r=3*GRIDLEN; //distance of camera from {0, 0, 0}
static float cam_theta=1.4f, cam_phi=4.9f; //for camera. 0 <= theta <= pi, 0 <= phi < 2*pi.

/*Update the frame*/
static void UpdateDrawFrame(void);
/*Update the camera position*/
static void UpdateCameraPos(void);
/*Check mouse collision*/
static void CheckMouseCollision(void);
/*Update scriptboxes*/
static void UpdateScriptBoxes(void);

#if defined (PLATFORM_DESKTOP)
/*Update the FPS*/
static void UpdateFPS(void);
#endif

/*Allocate piecepos*/
static void AllocPiecepos(void);
/*Set axis_end's*/
static void SetAxisEnd(void);



int main(void) {
    //Initialization
    //--------------------------------------------------------------------------------------
	assert(LINESMAX < 10);

    InitWindow(SCREENWIDTH, SCREENHEIGHT, "3D Omok");

	/*Camera init.*/
	GETCAMERAPOS(&camera.position, cam_r, cam_theta, cam_phi);
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 0.0f, 1.0f};
    camera.fovy = 45.0f;
    camera.type = CAMERA_PERSPECTIVE;

	SetCameraMode(camera, CAMERA_CUSTOM);

	if (GameStarted() == -1) exit(1);

#if defined (PLATFORM_WEB)
	emscripten_sample_gamepad_data(); //to prevent unknown error
	emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);

    //--------------------------------------------------------------------------------------

    //Main game loop
    while (!WindowShouldClose()) { //Detect window close button or ESC key
        UpdateDrawFrame();
    }
#endif

    //De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow(); //Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

static void UpdateDrawFrame(void) {
#ifndef USECUBES
	const int PIECE_DETAIL=6; //detail of pieces;
#endif
	const int FONTSIZE_GENERAL=20; //Default font size
	const int FONTSIZE_FLOAT=10; //font size of floating texts around pieces
	const int FONTSIZE_FLOAT_AXIS=25; //font size of floating text around end of axes

	const Color GRID_COLOR=BLACK; //color of the grid	
	const Color GENERAL_COLOR=BLACK; //default text color
	const Color FLOATTEXT_COLOR=Fade(DARKGREEN, 0.8f); //color of floating texts

	const int MARGIN=5; //margin of scriptboxes
	const Vector2 SCRIPTBOX_SIZE = (Vector2) {(SCREENWIDTH-SCREENHEIGHT)/2 - 2*MARGIN, (SCREENHEIGHT - 2*MARGIN)}; //def. sb size

	const Vector2 INFOBOX_V = (Vector2) {MARGIN, MARGIN}; //left upper point of infobox	
	const Vector2 INFOBOX_SIZE = 
		(Vector2) {SCRIPTBOX_SIZE.x, SCRIPTBOX_SIZE.y * ((float) 3/4)}; //size of infobox

	const Vector2 NOTIBOX_V = (Vector2) {MARGIN, INFOBOX_V.y+INFOBOX_SIZE.y}; //left upper point of infobox	
	const Vector2 NOTIBOX_SIZE = {SCRIPTBOX_SIZE.x, SCRIPTBOX_SIZE.y - INFOBOX_SIZE.y}; //size of infobox
	const int NOTIBOX_COLOR=0xcccccc33;

	const Vector2 MENUBOX_V = (Vector2) {SCREENWIDTH-SCRIPTBOX_SIZE.x-MARGIN, MARGIN}; //left upper point of menubox
	const Vector2 MENUBOX_SIZE = SCRIPTBOX_SIZE; //size of menubox
	const int MENUBOX_COLOR=0xcccccc33;

	int x, y, z; //for iteration
	
	//Update
	//----------------------------------------------------------------------------------

	/*Move camera by keyboard*/
	UpdateCameraPos();
	/*Check mouse collision*/
	CheckMouseCollision();
	/*Scriptbox update*/
	UpdateScriptBoxes();
	
	/*When var lines is changed*/
	if (cur_lines != lines) {
		AllocPiecepos();
		SetAxisEnd();
		cur_lines=lines;
	}

	//----------------------------------------------------------------------------------

	//Draw
	//----------------------------------------------------------------------------------
	BeginDrawing();

		ClearBackground(RAYWHITE);

		BeginMode3D(camera);	

			/*Draw pieces*/
			for (z=0; z < lines; z++)
				for (y=0; y < lines; y++)
					for (x=0; x < lines; x++) {
					#ifndef USECUBES
						DrawSphereEx(piecepos[z][y][x], PIECESIZE*piecesize_mult, PIECE_DETAIL, PIECE_DETAIL, PIECECOLOR(x, y, z));
						DrawSphereWires(piecepos[z][y][x], PIECESIZE*piecesize_mult, PIECE_DETAIL, PIECE_DETAIL, PIECECOLOR(x, y, z));
					#else
						DrawCube(piecepos[z][y][x], 
							CUBESIZE*piecesize_mult, CUBESIZE*piecesize_mult, CUBESIZE*piecesize_mult, PIECECOLOR(x, y, z));
						DrawCubeWires(piecepos[z][y][x], 
							CUBESIZE*piecesize_mult, CUBESIZE*piecesize_mult, CUBESIZE*piecesize_mult, PIECECOLOR(x, y, z));
					#endif
					}

			/*Draw lines*/
			/*z lines*/
			for (y=0; y < lines; y++)
				for (x=0; x < lines; x++)
					DrawLine3D(piecepos[0][y][x], piecepos[lines-1][y][x], GRID_COLOR);
			/*y lines*/
			for (x=0; x < lines; x++)
				for (z=0; z < lines; z++)
					DrawLine3D(piecepos[z][0][x], piecepos[z][lines-1][x], GRID_COLOR);
			/*x lines*/
			for (y=0; y < lines; y++)
				for (z=0; z < lines; z++)
					DrawLine3D(piecepos[z][y][0], piecepos[z][y][lines-1], GRID_COLOR);

			/*Draw axis*/
			DrawLine3D(piecepos[0][0][0], z_axis_end, GRID_COLOR);
			DrawLine3D(piecepos[0][0][0], y_axis_end, GRID_COLOR);
			DrawLine3D(piecepos[0][0][0], x_axis_end, GRID_COLOR);

		EndMode3D();
		
		/*Draw text around pieces*/
		for (z=0; z < lines; z++)
			for (y=0; y < lines; y++)
				for (x=0; x < lines; x++)
					DrawText(FormatText("%d%d%d", x, y, z), 
						PIECESCREENPOS(x, y, z, camera).x, PIECESCREENPOS(x, y, z, camera).y, FONTSIZE_FLOAT, FLOATTEXT_COLOR);
			
		/*Draw text around axes*/
		DrawText("x", GetWorldToScreen(x_axis_end, camera).x, GetWorldToScreen(x_axis_end, camera).y, FONTSIZE_FLOAT_AXIS, FLOATTEXT_COLOR);
		DrawText("y", GetWorldToScreen(y_axis_end, camera).x, GetWorldToScreen(y_axis_end, camera).y, FONTSIZE_FLOAT_AXIS, FLOATTEXT_COLOR);
		DrawText("z", GetWorldToScreen(z_axis_end, camera).x, GetWorldToScreen(z_axis_end, camera).y, FONTSIZE_FLOAT_AXIS, FLOATTEXT_COLOR);

		/*Left upper side: infobox*/
		DrawRectangleV(INFOBOX_V, INFOBOX_SIZE, GetColor(infobox_color));
		DrawText(infobox, INFOBOX_V.x+MARGIN, INFOBOX_V.y+MARGIN, FONTSIZE_GENERAL, GENERAL_COLOR);

		/*Left lower side: notibox*/
		DrawRectangleV(NOTIBOX_V, NOTIBOX_SIZE, GetColor(NOTIBOX_COLOR));
		DrawText(notibox, NOTIBOX_V.x+MARGIN, NOTIBOX_V.y+MARGIN, FONTSIZE_GENERAL, GENERAL_COLOR);

		/*Right side: menubox*/
		DrawRectangleV(MENUBOX_V, MENUBOX_SIZE, GetColor(MENUBOX_COLOR));
		DrawText(menubox, MENUBOX_V.x+MARGIN, MENUBOX_V.y+MARGIN, FONTSIZE_GENERAL, GENERAL_COLOR);


	EndDrawing();
	//----------------------------------------------------------------------------------
}

static void UpdateCameraPos(void) {
	static int cam_theta_increasing=1; //decide if theta should be increasing
	static float cam_sensitivity=0.05; //sensitivity of the camera
	static const float cam_zoom_mult=10; //sensitivity of camera zooming

	if (IsKeyDown(KEY_W) || IsKeyDown(KEY_KP_8)) { //UP
		if (cam_theta_increasing)
			cam_theta += cam_sensitivity;
		else
			cam_theta -= cam_sensitivity;
	}
	else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_KP_5)) { //DOWN
		if (cam_theta_increasing)
			cam_theta -= cam_sensitivity;
		else
			cam_theta += cam_sensitivity;
	}
	if (IsKeyDown(KEY_A) || IsKeyDown(KEY_KP_4)) cam_phi += cam_sensitivity; //LEFT
	else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_KP_6)) cam_phi -= cam_sensitivity; //RIGHT

	if (cam_theta < 0) {
		cam_theta *= -1;
		cam_theta_increasing = (cam_theta_increasing) ? 0 : 1;
		camera.up.z *= -1;
		cam_phi += PI_F;
	}
	else if (cam_theta > PI_F) {
		cam_theta = 2*PI_F - cam_theta;
		cam_theta_increasing = (cam_theta_increasing) ? 0 : 1;
		camera.up.z *= -1;
		cam_phi += PI_F;
	}
	if (cam_phi < 0) cam_phi += 2*PI_F;
	else if (cam_phi >= 2*PI_F) cam_phi -= 2*PI_F;

	/*Zoom in/out by mouse wheel or keypad*/
	cam_r -= GetMouseWheelMove()*cam_zoom_mult;
	if (IsKeyPressed(KEY_KP_7) || IsKeyPressed(KEY_Q)) cam_r += cam_zoom_mult;
	else if (IsKeyPressed(KEY_KP_9) || IsKeyPressed(KEY_E)) cam_r -= cam_zoom_mult;

	if (cam_r <= 0) cam_r = cam_zoom_mult; //min
	else if (cam_r > 6*GRIDLEN) cam_r = 6*GRIDLEN; //max 	
	
	/*Camera Update*/
	GETCAMERAPOS(&camera.position, cam_r, cam_theta, cam_phi);
	UpdateCamera(&camera);
}

static void CheckMouseCollision(void) {
	Ray ray={0}; //for mouse click detection
	static int collision=0; //how many collisions are detected 
	static int collision_piece=0; //the piece collision is detected, x*100 + y*10 + z
	static double collision_distance=0; //distance between the piece where collision detected and the camera

	int x, y, z; //for iteration

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		if (!collision) {
			ray = GetMouseRay(GetMousePosition(), camera);

			for (z=0; z < lines; z++) {
				for (y=0; y < lines; y++) {
					for (x=0; x < lines; x++) {
						if ( //collsion detected
						#ifndef USECUBES
							CheckCollisionRaySphere(ray,  piecepos[z][y][x], PIECESIZE*piecesize_mult)
						#else
							CheckCollisionRayBox(ray, piececube[z][y][x])
						#endif
						) {
							collision++; //counter the collision for loop to be done when (lines) of collisions are detected

							/*if it is the first time collision is detected or the detected is closer to the camera than previous one*/
							if (collision == 1 || VECTOR3_DIFF(piecepos[z][y][x], camera.position) < collision_distance) {
								collision_piece = 100*x + 10*y + z;
								collision_distance = VECTOR3_DIFF(piecepos[z][y][x], camera.position);
							}
						}
						if (collision >= lines) break;
					}
					if (collision >= lines) break;
				}
				if (collision >= lines) break;
			}
		}

		else collision=collision_piece=collision_distance=0; //reset the values
	}
	if (collision) {
		PieceClicked(collision_piece); 
		collision=collision_piece=collision_distance=0;
	}
}

static void UpdateScriptBoxes(void) {
	int menukey;

	/*Menubox*/
	for (menukey=0; menukey < 10; menukey++)
		if (IsKeyPressed(KEY_ZERO + menukey)) break;
	UpdateMenuBox(menukey);

	/*Infobox*/
#if !defined (PLATFORM_WEB)
	UpdateFPS();
#endif
	UpdateInfoBox(cam_r, cam_theta, cam_phi);

	/*Notibox*/
	UpdateNotiBox(NULL);
}

static void AllocPiecepos(void) {
	/*If the variable lines is changed, allocate then set the piecepos*/
	if (cur_lines == lines) { //doesn't need to change
		return;
	}

	int z, y, x; //for iteration

	/*Free the piecepos*/
	for (z=0; z < cur_lines; z++) {
		for (y=0; y < cur_lines; y++)
			free(piecepos[z][y]); //free x

		free(piecepos[z]); //free y
	}

	free(piecepos); //free z
	
	/*Alloc the piecepos*/
	piecepos = (Vector3 ***) calloc(lines, sizeof (Vector3 **)); //alloc z
	if (!piecepos) exit(1);

	for (z=0; z < lines; z++) {
		piecepos[z] = (Vector3 **) calloc(lines, sizeof (Vector3 *)); //alloc y
		if (!piecepos[z]) exit(1);

		for (y=0; y < lines; y++) {
			piecepos[z][y] = (Vector3 *) calloc(lines, sizeof (Vector3)); //alloc x
			if (!piecepos[z][y]) exit(1);
		}
	}

	/*Set the piecepos*/
	for (z=0; z < lines; z++)
		for (y=0; y < lines; y++)
			for (x=0; x < lines; x++) {
				piecepos[z][y][x].x = -GRIDLEN/2.0f + x*PIECESPACING;
				piecepos[z][y][x].y = -GRIDLEN/2.0f + y*PIECESPACING;
				piecepos[z][y][x].z = -GRIDLEN/2.0f + z*PIECESPACING;
			}

#ifdef USECUBES
	float len=CUBESIZE/2.0f;
	Vector3 pos;

	/*Free the piececube*/
	for (z=0; z < cur_lines; z++) {
		for (y=0; y < cur_lines; y++)
			free(piececube[z][y]); //free x

		free(piececube[z]); //free y
	}

	free(piececube); //free z

	/*Alloc the piececube*/
	piececube = (BoundingBox ***) calloc(lines, sizeof (BoundingBox **)); //alloc z
	if (!piececube) exit(1);

	for (z=0; z < lines; z++) {
		piececube[z] = (BoundingBox **) calloc(lines, sizeof (BoundingBox *)); //alloc y
		if (!piececube[z]) exit(1);

		for (y=0; y < lines; y++) {
			piececube[z][y] = (BoundingBox *) calloc(lines, sizeof (BoundingBox)); //alloc x
			if (!piececube[z][y]) exit(1);
		}
	}

	/*Set the piecepos*/
	for (z=0; z < lines; z++)
		for (y=0; y < lines; y++)
			for (x=0; x < lines; x++) {
				pos=piecepos[z][y][x];
				piececube[z][y][x].min = (Vector3) {pos.x-len, pos.y-len, pos.z-len}; 
				piececube[z][y][x].max = (Vector3) {pos.x+len, pos.y+len, pos.z+len};
			}
#endif

	//cur_lines is changed in the loop.
}

/*Set axis_end's*/
static void SetAxisEnd(void) {
	x_axis_end = piecepos[0][0][lines-1];
	x_axis_end.x += GRIDLEN/3;

	y_axis_end = piecepos[0][lines-1][0];
	y_axis_end.y += GRIDLEN/3;

	z_axis_end = piecepos[lines-1][0][0];
	z_axis_end.z += GRIDLEN/3;
}

#if !defined (PLATFORM_WEB)
/*Update FPS every second. This is a modificaion of DrawFPS().*/
static void UpdateFPS(void) {
    // NOTE: We are rendering fps every second for better viewing on high framerates
    static int counter = 0;
    static int refresh_rate = 20;

    if (counter < refresh_rate) counter++;
    else {
        fps = GetFPS();
        refresh_rate = fps;
        counter = 0;
    }
}
#endif