/*---------------------------------------------------------------------------
 * Title: GG-exercise1-G2
 * Author: Buttinger Xaver - S2010307005
 * Version: 1.0
 * Time to complete: 15h
 * Additional material: slides, course notes
 *------------------------------------------------------------------------- */

#include "GL/freeglut.h"
#include <iostream>
#include <time.h>
#include "math.h"
#include "main.h"

 // ---------------------------------------------------------------------- 
 //						DEFINES
 // ----------------------------------------------------------------------

#define LOG_POSITION 0
#define LOG_MOUSE 0

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define MAZE_DIMENSION 10
#define MAZE_CUBE_SIZE 1.0f

// if we define 1.f we fix the bug, that we see the inside of cubes
// but then we can't navigate through the maze, because the walkways are too thin
#define DISTANCE_OFFSET_FOR_COLLISION 0.85f
#define MOUSE_MOVE_DELTA 0.015f

 // for comparing floats
#define EPSILON (0.00000005f)
#define ISEQUAL(x,y)     (fabs((x) - (y)) <= EPSILON)

#define SOLID_OBJECTS_COUNT 2
#define CUSTOM_OBJECTS_COUNT 2

#define JUMP_HEIGHT_MAX 1.f
#define JUMP_ORIGIN 0.f
#define JUMP_DELTA 0.03f
#define JUMP_EPSILON 0.005f

// ---------------------------------------------------------------------- 
//						TYPEDEFS
// ----------------------------------------------------------------------

/// <summary>
/// Function pointer typedef so we can pass different drawing functions (solid objects and custom objects) to our flexible_drawing function which takes care of transformations
/// </summary>
typedef void (*draw_function)(void);

typedef struct point {
	float x;
	float y;
	float z;
} point;

/// <summary>
/// Struct for objects placed in the map. We are using this only for two solid and two custom objects atm.
/// We will be using this struct for animations, distance detection and conditional draw if the object is hidden. 
/// The walls are fix positioned and not represented as object struct.
/// THe maze is built by using a matrix and not represented as object struct.
/// </summary>
typedef struct object {
	point position_world;
	float rotation_y_angle = 0.0f;
	float rotation_x_angle = 0.0f;
	time_t rotation_begin = NULL;
	bool is_visible = true;
} object;


// ---------------------------------------------------------------------- 
//						Function definitions
// ----------------------------------------------------------------------

void draw_solid_objects(void);
void draw_solid_object(object* object_to_draw);
void process_jump_up(void);
void draw_custom_objects(void);
void draw_walls(void);
void draw_x(void);
void draw_maze_cube(int i, int j);
void process_jump_down(void);
void process_jump();
void draw_custom_object(object* object_to_draw);
void init_callbacks();
void init_window();
void init(int argc, char** argv);
void mouse_move_up_or_down(int ycoor);
void mouse_move_left_or_right(int xcoor);
void draw_x_from_primitives(float z);
void rotate_object(object* object_to_rotate);
void rotate_close_object(void);
bool can_move(float x, float y, float z);
float calculate_distance(float x, float z, float object_x, float object_z);
void flexible_draw(object* object_to_draw, draw_function draw_fn);
void do_draw_solid_object(void);

// ---------------------------------------------------------------------- 
//						Variables
// ----------------------------------------------------------------------

/// <summary>
/// Array holding the solid objects
/// </summary>
object objects[SOLID_OBJECTS_COUNT];

/// <summary>
/// Array holding the custom objects
/// </summary>
object custom_objects[CUSTOM_OBJECTS_COUNT];

int windowid;
int window_width = 800;
int window_height = 600;

/// <summary>
/// This matrix reprresents our maze
/// 1 means a block will be rendered, 0 is empty
/// </summary>
bool maze[MAZE_DIMENSION][MAZE_DIMENSION] = {
	{ 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, },
	{ 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, },
	{ 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, },
	{ 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, },
	{ 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, },
	{ 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, },
	{ 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, },
	{ 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, },
	{ 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, },
	{ 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, },
};

// navigation variables
GLfloat navX = 0.0f;
GLfloat navZ = 9.5f; // we start 0.5 before the near wall
GLfloat navY = 0.0f; // wee need this for jumping

GLdouble x = 0.0f, z = 5.0f, y = 0.0f;    // XZ position of the camera

// Camera motion variables
GLdouble angle_left_right = 0.0f;          // angle of rotation for the camera direction
GLdouble angle_up_down = 0.0f;
GLdouble lx = 0.0f, lz = -1.0f, ly = 0.0f; // actual vector representing the camera's direciton

float mouse_x = 0.f;
float mouse_y = 0.f;

// states for jumping
bool jump_up = false;
bool jump_down = false;

// we position the maze cubes with some offset so it's nice positioned
float maze_offset_x = -4.75f;
float maze_offset_y = 0.f;
float maze_offset_z = -9.5f;

// ---------------------------------------------------------------------- 
//						INPUT EVENTS
// ----------------------------------------------------------------------

/// <summary>
/// Keyboard function for movement, object interaction and exit
/// </summary>
void keyboard(unsigned char key, int xcoor, int ycoor) {
	switch (key) {
	case 'a':
		if (can_move(navX - 0.1f, navY, navZ)) {
			navX -= 0.1f;
		}
		break;
	case 'd':
		if (can_move(navX + 0.3f /*0.5f*/, navY, navZ)) {
			navX += 0.1f;
		}
		else navX -= 0.15f;
		break;
	case 'w':
		if (can_move(navX, navY, navZ - 0.1f)) {
			navZ -= 0.1f;
		}
		else navZ += 0.15f;
		break;
	case 's':
		if (can_move(navX, navY, navZ + 0.3f)) {
			navZ += 0.1f;
		}
		else navZ -= 0.15f;
		break;
	case 'e':
		rotate_close_object();
		break;
	case 32: // space
		jump_up = true;
		break;
	case 27: // escape key
		glutDestroyWindow(windowid);
		exit(0);
		break;
	}
	if (LOG_POSITION) {
		std::cout << "Camera position: navX: " << navX << ", navZ " << navZ << std::endl;
		std::cout << "target position: navX + lx: " << navX + lx << ", navZ + lz " << navZ + lz << std::endl;
	}
	glutPostRedisplay();
}

//Taken from http://www.lighthouse3d.com/tutorials/glut-tutorial/keyboard-example-moving-around-the-world/
void processSpecialKeys(int key, int xcoor, int ycoor) {
	float fraction = 0.1f;

	switch (key) {
	case GLUT_KEY_LEFT:
		angle_left_right -= 0.01f;
		lx = sin(angle_left_right);
		lz = -cos(angle_left_right);
		break;
	case GLUT_KEY_RIGHT:
		angle_left_right += 0.01f;
		lx = sin(angle_left_right);
		lz = -cos(angle_left_right);
		break;
	case GLUT_KEY_UP:
		x += lx * fraction;
		z += lz * fraction;
		break;
	case GLUT_KEY_DOWN:
		x -= lx * fraction;
		z -= lz * fraction;
		break;
	}
}

void mouse(int xcoor, int ycoor) {

	mouse_move_left_or_right(xcoor);
	mouse_move_up_or_down(ycoor);

	if (LOG_MOUSE) {
		std::cout << "mouse coordinates: x (" << xcoor << "), y (" << ycoor << ")" << std::endl;
	}

	mouse_x = window_width / 2.f;
	mouse_y = window_height / 2.f;
	// reset the mouse position to the middle of our screen
	// without this, we would not be able to move endless in one direction,
	// because the mouse would just leave our window 
	glutWarpPointer(window_width / 2, window_height / 2);
}

// ---------------------------------------------------------------------- 
//						UTIL
// ----------------------------------------------------------------------

///
/// Checks if two points are closer than our defined DISTANCE_OFFSET_FOR_COLLISION.
///  Usually used for Player to Object distance evaluation.
/// 
bool is_close(float x, float z, float object_x, float object_z) {
	float distance = calculate_distance(x, z, object_x, object_z);
	if (distance < 0) distance *= -1;

	return distance < DISTANCE_OFFSET_FOR_COLLISION;
}

///
/// Calculates the distance between two points in world coordinates.
/// Usually used for Player to Object distance evaluation.
/// (Thanks pythagoras)
/// 
float calculate_distance(float x, float z, float object_x, float object_z) {
	return (float) sqrt(pow(x - object_x, 2) + sqrt(pow(z - object_z, 2))); // this conversion is no problem
}


bool can_move(float x, float y, float z) {
	// calculate the distance from player to each cube
	// if distance is under defined treshhold DISTANCE_OFFSET_FOR_COLLISION we can't move
	for (int i = 0; i < MAZE_DIMENSION; i++) {
		for (int j = 0; j < MAZE_DIMENSION; j++) {
			if (maze[i][j] == 0) continue;
			float object_x = maze_offset_x + j;
			float object_z = maze_offset_z + i;

			if (is_close(x, z, object_x, object_z))
				return false;
		}
	}
	return true;
}

void mouse_move_left_or_right(int xcoor)
{
	float center_x = window_width / 2.f;

	if (xcoor < mouse_x) {
		angle_left_right -= MOUSE_MOVE_DELTA;
		lx = sin(angle_left_right);
		lz = -cos(angle_left_right);
	}
	else if (xcoor > mouse_x) {
		angle_left_right += MOUSE_MOVE_DELTA;
		lx = sin(angle_left_right);
		lz = -cos(angle_left_right);
	}
}

void mouse_move_up_or_down(int ycoor)
{
	if (ycoor < mouse_y) {
		angle_up_down += MOUSE_MOVE_DELTA;
		ly = sin(angle_up_down);
	}
	else if (ycoor > mouse_y) {
		angle_up_down -= MOUSE_MOVE_DELTA;
		ly = sin(angle_up_down);
	}
}

///
/// This function should be used for drawing.
/// We can pass a object and the according draw function and flexible_draw will take care of transformations.
///
void flexible_draw(object* object_to_draw, draw_function draw_fn) {
	if (!object_to_draw->is_visible)
		return;

	glPushMatrix();
	glTranslatef(object_to_draw->position_world.x, object_to_draw->position_world.y, object_to_draw->position_world.z);
	glRotatef(object_to_draw->rotation_x_angle, 1.0f, 0.f, 0.f);
	glRotatef(object_to_draw->rotation_y_angle, 0.0f, 1.f, 0.f);
	draw_fn();
	glPopMatrix();

}

// ---------------------------------------------------------------------- 
//				DRAW
// ----------------------------------------------------------------------


//  --- --- --- --- --- --- --- --- --- --- --- ---
//					maze
//  --- --- --- --- --- --- --- --- --- --- --- ---
void draw_maze(void) {
	glPushMatrix();
	glTranslatef(maze_offset_x, maze_offset_y, maze_offset_z); // move the maze to it's starting position

	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			if (maze[j][i]) {
				draw_maze_cube(i, j);
			}
		}
	}
	glPopMatrix();
}

void draw_maze_cube(int i, int j)
{
	glPushMatrix();
	glColor3f(0.9f, 0.9f, 0.9f);// grey
	glTranslatef((float)i, 0.f, (float)j); // those conversions are no problem
	glutSolidCube(MAZE_CUBE_SIZE);
	glPopMatrix();
}

//  --- --- --- --- --- --- --- --- --- --- --- ---
//					walls
//  --- --- --- --- --- --- --- --- --- --- --- ---

///
///	This function will draw our walls
///
void draw_walls(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f(0.9f, 0.9f, 0.9f);// grey
	glBegin(GL_QUADS);  // drawing walls using quads
	glPushMatrix();

	float fleft = -2.5f;
	float fright = 2.5f;
	float fnear = 0.f;
	float ffar = 10.f;
	float ftop = 1.f;
	float fbottom = -0.5f;

	// right wall
	glVertex3f(fright, ftop, fnear); // top far
	glVertex3f(fright, fbottom, fnear); // bottorm far
	glVertex3f(fright, fbottom, ffar); // bottowm near
	glVertex3f(fright, ftop, ffar); // top near

	// left wall
	glVertex3f(fleft, ftop, fnear); // top far
	glVertex3f(fleft, fbottom, fnear); // bottom far
	glVertex3f(fleft, fbottom, ffar); // bottom near
	glVertex3f(fleft, ftop, ffar); // bottom near

	// near wall
	glVertex3f(fleft, ftop, ffar);  // left top
	glVertex3f(fleft, fbottom, ffar);  // left bottom
	glVertex3f(2.5f, fbottom, ffar);  // right bottom
	glVertex3f(2.5f, ftop, ffar);  // right bottowm

	// far wall with door - left part
	glVertex3f(fleft, ftop, fnear); // top far
	glVertex3f(fleft, fbottom, fnear); // bottom far
	glVertex3f(fbottom, fbottom, fnear); // bottom near
	glVertex3f(fbottom, ftop, fnear); // bottom near

	// far wall with door - right part
	glVertex3f(2.5f, ftop, fnear); // top far
	glVertex3f(2.5f, fbottom, fnear); // bottom far
	glVertex3f(ftop, fbottom, fnear); // bottom near
	glVertex3f(ftop, ftop, fnear); // bottom near

	glPopMatrix();
	glEnd();
}

//  --- --- --- --- --- --- --- --- --- --- --- ---
//					solid objects
//  --- --- --- --- --- --- --- --- --- --- --- ---

void draw_solid_objects(void)
{
	for (int i = 0; i < SOLID_OBJECTS_COUNT; i++) {
		draw_solid_object(&objects[i]);
	}
	return;
}


void draw_solid_object(object* object_to_draw) {
	flexible_draw(object_to_draw, do_draw_solid_object);
}

/// <summary>
/// This function will draw our solid object (cube and a sphere on top of it)
/// </summary>
void do_draw_solid_object(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPushMatrix();
	glColor3f(0.0f, 1.0f, 0.0f);
	glutSolidCube(0.5f);
	glTranslatef(0.0f, 0.5f, 0.0f);
	glutSolidSphere(0.25f, 20, 20);
	glPopMatrix();
}


//  --- --- --- --- --- --- --- --- --- --- --- ---
//			custom objects made of primitives
//  --- --- --- --- --- --- --- --- --- --- --- ---
void draw_custom_objects(void)
{
	glColor3f(0.f, 0.f, 1.f);
	for (int i = 0; i < CUSTOM_OBJECTS_COUNT; i++) {
		draw_custom_object(&custom_objects[i]);
	}
}

/// <summary>
/// Draws a single custom object (letter X)
/// </summary>
void draw_custom_object(object* object_to_draw) {
	flexible_draw(object_to_draw, draw_x);
}

/// <summary>
/// This will draw the letter x constructed by glVertexes
/// </summary>
void draw_x(void) {
	glBegin(GL_LINE_LOOP);  // drawing walls using quads
	glPushMatrix();

	// near side
	draw_x_from_primitives(0.1f);
	// far side
	draw_x_from_primitives(-0.1f);

	glPopMatrix();
	glEnd();
}

void draw_x_from_primitives(float z) {
	// those are coordinates for drawing the letter X by single points (12 points for near, 12 points for far)
	// i constructed those coordinates by drawing a coordinate system on paper
	glVertex3f(-0.5f, 0.5f, z);
	glVertex3f(-0.4f, 0.5f, z);
	glVertex3f(0.0f, 0.2f, z);
	glVertex3f(0.4f, 0.5f, z);
	glVertex3f(0.5f, 0.5f, z);
	glVertex3f(0.2f, 0.f, z);
	glVertex3f(0.5f, -0.5f, z);
	glVertex3f(0.4f, -0.5f, z);
	glVertex3f(0.0f, -0.2f, z);
	glVertex3f(-0.4f, -0.5f, z);
	glVertex3f(-0.5f, -0.5f, z);
	glVertex3f(-0.2f, 0.f, z);
}


/// <summary>
/// Our rendering function. Here we will draw all necessary game objects (solid objects, custom objects, walls and maze)
/// </summary>
void render(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
	glLoadIdentity();

	gluLookAt(navX, navY, navZ,				// camera position
		navX + lx, y + ly, navZ + lz,       // target position (at)
		0.0f, 1.0f, 0.0f);				    // up vector

	draw_walls();
	draw_solid_objects();
	draw_maze();
	draw_custom_objects();
	
	glutSwapBuffers();
}

// ---------------------------------------------------------------------- 
//							JUMP
// ----------------------------------------------------------------------

/// <summary>
/// If the user pressed the space button, the state variable jump_up will be true.
/// We then want to increase the jump properties until we reached the maximum. 
/// Then jump_up will be set to false, but jump_down becomes true.
/// We then want to decrease the jump properties.
/// </summary>
void process_jump(void)
{
	if (jump_up) {
		process_jump_up();
	}
	else if (jump_down) {
		process_jump_down();
	}
}

void process_jump_up(void)
{
	navY = MIN(navY + JUMP_DELTA, JUMP_HEIGHT_MAX);
	bool maximum_reached = (JUMP_HEIGHT_MAX - navY) < JUMP_EPSILON;
	if (maximum_reached) {
		jump_up = false;
		jump_down = true;
	}
}

void process_jump_down(void)
{
	navY = MAX(navY - JUMP_DELTA, JUMP_ORIGIN);
	bool minimum_reached = navY < JUMP_EPSILON;
	if (minimum_reached) {
		jump_down = false;
		navY = JUMP_ORIGIN;
	}
}

// ---------------------------------------------------------------------- 
//				ROTATE and HIDE (Interaction with objects)
// ----------------------------------------------------------------------

/// <summary>
/// Will be called if user presses 'e'. Check if any object is near. If yes, set the rotation_begin timestamp so our idle function will rotate it.
/// </summary>
/// <param name=""></param>
void rotate_close_object(void) {
	for (int i = 0; i < SOLID_OBJECTS_COUNT; i++) {
		if (!is_close(navX, navZ, objects[i].position_world.x, objects[i].position_world.z))
			continue;

		time(&objects[i].rotation_begin);
	}

	for (int i = 0; i < CUSTOM_OBJECTS_COUNT; i++) {
		if (!is_close(navX, navZ, custom_objects[i].position_world.x, custom_objects[i].position_world.z))
			continue;

		time(&custom_objects[i].rotation_begin);
	}
}

/// <summary>
/// Trys to rotate each available object
/// </summary>
void rotate_objects(void) {
	for (int i = 0; i < SOLID_OBJECTS_COUNT; i++) {
		rotate_object(&objects[i]);
	}

	for (int i = 0; i < CUSTOM_OBJECTS_COUNT; i++) {
		rotate_object(&custom_objects[i]);
	}
}

/// <summary>
/// Rotates the passed object if rotation_begin time conditions are set.
/// </summary>
void rotate_object(object* object_to_rotate) {
	time_t rawtime = time(NULL);
	if (object_to_rotate->rotation_begin == NULL)
		return;

	double difference_seconds = difftime(rawtime, object_to_rotate->rotation_begin);
	if (difference_seconds < 3)
		object_to_rotate->rotation_y_angle += 10.f;
	else
		object_to_rotate->is_visible = false;
}

// ---------------------------------------------------------------------- 
//						IDLE (rotate and hide, jump)
// ----------------------------------------------------------------------

void idleFunc(void) {
	rotate_objects();
	process_jump();
	glutPostRedisplay();
}

// ---------------------------------------------------------------------- 
//						reshape
// ----------------------------------------------------------------------

void reshapeFunc(int xwidth, int yheight) {
	if (yheight == 0 || xwidth == 0) return;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(40.0f, (GLdouble)xwidth / (GLdouble)yheight, 0.5f, 100.0f);
	glViewport(0, 0, xwidth, yheight);

	window_width = xwidth;
	window_height = yheight;
}

// ---------------------------------------------------------------------- 
//						INIT
// ----------------------------------------------------------------------
void init_callbacks()
{
	glutReshapeFunc(reshapeFunc);
	glutDisplayFunc(render);
	glutIdleFunc(idleFunc);
	glutPassiveMotionFunc(mouse);
	glutKeyboardFunc(keyboard);
	// i found this function in the docs; we need to use this, because the glutMouseFunc only triggers when we press a mouse button
	glutSpecialFunc(processSpecialKeys);
}

void init_window()
{
	glutInitWindowPosition(500, 500);
	glutInitWindowSize(window_width, window_height);
	windowid = glutCreateWindow("Xavers first 3D game");
}

/// <summary>
/// Here we define our solid object data which will be drawn during the rendering process
/// </summary>
void init_solid_object_data(void) {
	object object1;
	object1.rotation_x_angle = 0.0f;
	object1.rotation_y_angle = 0.0f;
	object1.position_world.x = 1.0f;
	object1.position_world.y = -0.5f;
	object1.position_world.z = 5.0f;

	object object2;
	object2.rotation_x_angle = 180;
	object2.rotation_y_angle = 0.0f;
	object2.position_world.x = -1.0f;
	object2.position_world.y = .0f;
	object2.position_world.z = 5.0f;

	objects[0] = object1;
	objects[1] = object2;
}

/// <summary>
/// Here we define our custom object data which will be drawn during the rendering process
/// </summary>
void init_custom_object_data() {
	object object1;
	object1.rotation_x_angle = 0.0f;
	object1.rotation_y_angle = 0.0f;
	object1.position_world.x = -1.0f;
	object1.position_world.y = 0.f;
	object1.position_world.z = 1.0f;

	object object2;

	object2.rotation_x_angle = 0;
	object2.rotation_y_angle = 0.0f;
	object2.position_world.x = 1.0f;
	object2.position_world.y = .0f;
	object2.position_world.z = 2.0f;

	custom_objects[0] = object1;
	custom_objects[1] = object2;
}

void init(int argc, char** argv)
{
	init_solid_object_data();
	init_custom_object_data();

	glutInit(&argc, argv);
	
	glDepthFunc(GL_EQUAL);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glMatrixMode(GL_MODELVIEW);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	init_window();


	init_callbacks();

	glutMainLoop();
}

void print_start(void) {
	using std::cout;
	using std::endl;
	cout << "--------------------------------" << endl;
	cout << " Xavers amazing 3D maze<< endl " << endl;
	cout << "--------------------------------" << endl;
	cout << "         Thanks OpenGL " << endl;
	cout << "--------------------------------" << endl;
	cout << "Move with w,a,s,d, space and mouse" << endl;
	cout << "You can remove objects by pressing 'e' when you are really close to them" << endl;
	cout << "You can walk through the walls in the first room, but not through maze walls!" << endl;
	cout << "Have fun exploring the maze!" << endl;
}

// ---------------------------------------------------------------------- 
//						MAIN
// ----------------------------------------------------------------------
int main(int argc, char** argv) {
	print_start();
	init(argc, argv);
	return 0;
}