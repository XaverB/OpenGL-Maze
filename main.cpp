/*---------------------------------------------------------------------------
 * Title: Computer Graphics Lab 2 - Meshes and Transformations
 * Author: Christoph Anthes
 * Version: 1.0 (SS22)
 * Time to complete: 90 minutes
 * Additional material: slides, course notes
 *------------------------------------------------------------------------- */
#include "GL/freeglut.h"
#include <iostream>
#include <time.h>
#include "math.h"
#include "main.h"

#define LOG_POSITION 1
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

typedef struct point {
	float x;
	float y;
	float z;
} point;

typedef struct object {
	point position_world;
	float rotation_y_angle = 0.0f;
	float rotation_x_angle = 0.0f;
	time_t rotation_begin = NULL;
	bool is_visible = true;
} object;


object objects[2];
int objects_count = 2;

object custom_objects[2];
int custom_objects_count = 2;

int windowid; // the identifier of the GLUT window
int window_width = 800;
int window_height = 600;


GLfloat matrot[][4] = {          // a rotation matrix
  { 0.707f, 0.707f, 0.0f, 0.0f}, // it performs a rotation around z
  {-0.707f, 0.707f, 0.0f, 0.0f}, // in 45 degrees
  { 0.0f,   0.0f,   1.0f, 0.0f},
  { 0.0f,   0.0f,   0.0f, 1.0f}
};

GLfloat mattrans[][4] = {        // a translation matrix
  { 1.0f, 0.0f,  0.0f, 0.0f},    // it performs a translation along the
  { 0.0f, 1.0f,  0.0f, 0.0f},    // x-axis of 0.5 units and along
  { 0.0f, 0.0f,  1.0f, 0.0f},    // the z-axis of -1.5 units
  { 0.5f, 0.0f, -1.5f, 1.0f}
};

// this reprresents our maze
// 1 means a block will be rendered, 0 is empty
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

// Navigation variables - required for TASK 5
GLfloat navX = 0.0f;
GLfloat navZ = 9.5f; // we start 0.5 before the near wall
GLfloat navY = 0.0f; // wee need this for jumping

// Angle for cube rotation - required for TASK 6
GLfloat angleCube = 0.0f;        //angle for cube1

// Camera motion variables - required for HOMEOWRK HELPER
GLdouble angle_left_right = 0.0f;          // angle of rotation for the camera direction
GLdouble angle_up_down = 0.0f;
GLdouble lx = 0.0f, lz = -1.0f, ly = 0.0f; // actual vector representing the camera's
								// direction
GLdouble x = 0.0f, z = 5.0f, y = 0.0f;    // XZ position of the camera

time_t jump_start;
GLdouble angle_jump = 0.0f;

bool jump_up = false;
bool jump_down = false;

// we position the maze cubes with some offset so it's nice positioned
float maze_offset_x = -4.75f;
float maze_offset_y = 0.f;
float maze_offset_z = -9.5f;

float mouse_x = 0.f;
float mouse_y = 0.f;

/*
*/
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
/* Here we have an example to draw a bit nicer with our limited OpenGL
   knowledge. First filled objects are drawn in black, at a smaller size.
   Then the same object outlines are drawn in full size full size. Both
   will be compared in the depth buffer and the front outlines will
   remain. */
void drawObjectAlt(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPushMatrix(); //set where to start the current object transformations  
	glRotatef(angleCube, 0.0, 1.0, 0.0);
	glColor3f(0.0, 0.0, 0.0); //change cube1 to black
	glScalef(0.99, 0.99, 0.99);
	glutSolidCube(0.5);
	glTranslatef(0, 0.5, 0); //move cube1 to the right
	glScalef(0.99, 0.99, 0.99);
	glutSolidSphere(0.25f, 20, 20);
	glPopMatrix();

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPushMatrix(); //set where to start the current object transformations  
	glRotatef(angleCube, 0.0, 1.0, 0.0);
	glColor3f(0.0, 1.0, 0.0); //change cube1 to Green
	glutSolidCube(0.5);
	glTranslatef(0, 0.5, 0); //move cube1 to the right
	glutSolidSphere(0.25f, 20, 20);
	glPopMatrix();
}

float calculate_distance(float x, float z, float object_x, float object_z) {
	return sqrt(pow(x /*+ lx*/ - object_x, 2) + sqrt(pow(z /*+ lz*/ - object_z, 2)));
}

bool is_close(float x, float z, float object_x, float object_z) {
	float distance = calculate_distance(x, z, object_x, object_z);
	if (distance < 0) distance *= -1;

	return distance < DISTANCE_OFFSET_FOR_COLLISION;
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

void mouse(int xcoor, int ycoor) {

	mouse_move_left_or_right(xcoor);
	mouse_move_up_or_down(ycoor);

	if (LOG_MOUSE) {
		std::cout << "mouse coordinates: x (" << xcoor << "), y (" << ycoor << ")" << std::endl;
	}

	mouse_x = window_width / 2;
	mouse_y = window_height / 2;
	// reset the mouse position to the middle of our screen
	// without this, we would not be able to move endless in one direction,
	// because the mouse would just leave our window 
	glutWarpPointer(window_width / 2, window_height / 2);
}

void rotate_close_object(void) {
	for (int i = 0; i < objects_count; i++) {
		if (!is_close(navX, navZ, objects[i].position_world.x, objects[i].position_world.z))
			continue;

		time(&objects[i].rotation_begin);
	}

	for (int i = 0; i < custom_objects_count; i++) {
		if (!is_close(navX, navZ, custom_objects[i].position_world.x, custom_objects[i].position_world.z))
			continue;

		time(&custom_objects[i].rotation_begin);
	}
}

/* This is the keyboard function which is used to react on keyboard input.
   It has to be registered as a callback in glutKeyboardFunc. Once a key is
   pressed it will be invoked and the keycode as well as the current mouse
   coordinates relative to the window origin are passed.
   It acts on our FPS controls 'WASD' and the escape key. A simple output
   to the keypress is printed in the console in case of 'WASD'. In case of
   ESC the window is destroyed and the application is terminated. */
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
		/*else navZ -= 0.2f;*/
		break;
	case 'e':
		rotate_close_object();
		break;
	case 32: // space
		jump_up = true;
		jump_start = time(0); // get time of jump start trigger
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

/* This function should be called when the window is resized. It has to be
   registered as a callback in glutReshapeFunc. The function sets up the
   correct perspective projection. Don't worry about it we will not go into
   detail but we need it for correct perspective 3D rendering. */
void reshapeFunc(int xwidth, int yheight) {
	if (yheight == 0 || xwidth == 0) return;  // Nothing is visible, return

	glMatrixMode(GL_PROJECTION); // Set a new projection matrix
	glLoadIdentity();
	// Angle of view: 40 degrees
	// Near clipping plane distance: 0.5
	// Far clipping plane distance: 20.0
	gluPerspective(40.0f, (GLdouble)xwidth / (GLdouble)yheight, 0.5f, 100.0f);
	glViewport(0, 0, xwidth, yheight);  // Use the whole window for rendering

	window_width = xwidth;
	window_height = yheight;
}

/* This is our first display function it will be used for drawing a 2D
   triangle. The background is set to black and cleared, the current drawing
   colour is set and the vertices of the triangle are defined. At the end the
   buffers are flipped. */
void renderPrimitives(void) {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // set background colour to black 
	glClear(GL_COLOR_BUFFER_BIT);         // clear the colour buffer

	glColor3f(0.1f, 0.2f, 0.3f);        // set the colour to grey
	glBegin(GL_TRIANGLES);              // drawing using triangles
	glVertex3f(0.0f, 1.0f, 0.0f);      // top
	glVertex3f(-1.0f, -1.0f, 0.0f);    // bottom left
	glVertex3f(1.0f, -1.0f, 0.0f);    // bottom right
	glEnd();                            // finished drawing the triangle

	/* Example 1 - Slide 5 */
	glColor3f(1.0f, 0.0f, 0.0f);        // red
	glBegin(GL_QUADS);                  // drawing using quads
	glVertex2f(-0.5f, -0.5f);          // bottom left
	glVertex2f(0.5f, -0.5f);          // bottom right
	glVertex2f(0.5f, 0.5f);            // top right
	glVertex2f(-0.5f, 0.5f);          // top left
	glEnd();

	/* Example 2 - Slide 5 */
	glBegin(GL_QUADS);                  // drawing using quads
	glColor3f(1.0f, 0.0f, 0.0f);      // red
	glVertex2f(-0.5f, -0.5f);          // bottom left
	glColor3f(0.0f, 1.0f, 0.0f);      // green
	glVertex2f(0.5f, -0.5f);          // bottom right
	glColor3f(0.0f, 0.0f, 1.0f);      // blue
	glVertex2f(0.5f, 0.5f);            // top right
	glColor3f(1.0f, 1.0f, 0.0f);      // yellow
	glVertex2f(-0.5f, 0.5f);          // top left
	glEnd();

	// TASK 1:
	glColor3f(1.0f, 1.0f, 0.0f);  // yellow
	glBegin(GL_POLYGON);          // these vertices form a closed polygon
	glVertex2f(0.4f, 0.2f);
	glVertex2f(0.6f, 0.2f);
	glVertex2f(0.7f, 0.4f);
	glVertex2f(0.6f, 0.6f);
	glVertex2f(0.4f, 0.6f);
	glVertex2f(0.3f, 0.4f);
	glEnd();
	glutSwapBuffers();
}

/* This function will be used for composited objects and will be called from a
   display function. */
void draw_solid_object(void) { // TASK 4:
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPushMatrix(); //set where to start the current object transformations  
	glColor3f(0.0f, 1.0f, 0.0f);    // change cube1 to green
	glutSolidCube(0.5f);            // cube
	glTranslatef(0.0f, 0.5f, 0.0f); // move cube1 to the top
	glutSolidSphere(0.25f, 20, 20); // sphere
	glPopMatrix();
}

/* This function will replace the previous display function and will be used
   for scene setup. */
void render3DScene(void) {
	glMatrixMode(GL_MODELVIEW);    // set the ModelView Matrix for scene setup
	glClear(GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
	glColor3f(0.0f, 1.0f, 0.0f);  // green
	glTranslatef(0.0f, 0.0f, -1.5f);
	glRotatef(45, 1.0f, 0.0f, 0.0f);
	glutSolidCube(0.5f);

	// TASK 2:
	glutSolidSphere(0.1f, 20, 20);
	glutSolidTorus(0.6f, 1.4f, 20, 20);

	glutSwapBuffers();
}

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

void draw_object(object* object_to_draw) {
	if (!object_to_draw->is_visible)
		return;

	glPushMatrix();
	glTranslatef(object_to_draw->position_world.x, object_to_draw->position_world.y, object_to_draw->position_world.z);
	glRotatef(object_to_draw->rotation_x_angle, 1.0f, 0.f, 0.f);
	glRotatef(object_to_draw->rotation_y_angle, 0.0f, 1.f, 0.f);
	draw_solid_object();
	glPopMatrix();
}

void draw_objects(void)
{
	for (int i = 0; i < objects_count; i++) {
		draw_object(&objects[i]);
	}
	return;
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

void draw_custom_object(object* object_to_draw) {
	if (!object_to_draw->is_visible)
		return;

	glPushMatrix();
	glColor3f(0.0f, 0.0f, 1.0f);
	glTranslatef(object_to_draw->position_world.x, object_to_draw->position_world.y, object_to_draw->position_world.z);
	glRotatef(object_to_draw->rotation_x_angle, 1.0f, 0.f, 0.f);
	glRotatef(object_to_draw->rotation_y_angle, 0.0f, 1.f, 0.f);
	draw_x();
	glPopMatrix();
}

void draw_custom_objects(void)
{
	for (int i = 0; i < custom_objects_count; i++) {
		draw_custom_object(&custom_objects[i]);
	}
	
}

void draw_maze_cube(int i, int j)
{
	glPushMatrix();
	glColor3f(0.9f, 0.9f, 0.9f);// grey
	glTranslatef(i, 0.f, j);
	glutSolidCube(MAZE_CUBE_SIZE);
	glPopMatrix();
}

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

/* This function will replace the previous display function and will be used
   for rendering a cube and playing with transformations. */
void render(void) {
	glMatrixMode(GL_MODELVIEW);
	// glClear(GL_DEPTH_BUFFER_BIT); // Helper to be used with drawObjectAlt
	glClear(GL_COLOR_BUFFER_BIT);

	glLoadIdentity();

	gluLookAt(navX, navY, navZ,      // camera position
		navX + lx, y + ly, navZ + lz,      // target position (at)
		0.0f, 1.0f, 0.0f);     // up vector

	draw_walls();
	draw_objects();
	draw_maze();
	draw_custom_objects();
	glutSwapBuffers();
}



void process_jump_up(float jump_delta, float jump_height_max, float epsilon)
{
	navY = MIN(navY + jump_delta, jump_height_max);

	if ((jump_height_max - navY) < epsilon) { // we reached our maximum
		jump_up = false;
		jump_down = true;
	}
}

void process_jump_down(float jump_delta, float jump_origin, float epsilon)
{
	navY = MAX(navY - jump_delta, jump_origin);
	if (navY < epsilon) {
		jump_down = false;
		navY = jump_origin;
	}
}

void process_jump(void)
{
	float jump_delta = 0.03f; // we increase or decrease y every call by this value
	float jump_height_max = 1.0f;
	float epsilon = 0.005; // to compare floats
	float jump_origin = 0.0f; // our jumping starting point. we wan't back to this y value after the jump is finished

	if (jump_up) {
		process_jump_up(jump_delta, jump_height_max, epsilon);
	}
	else if (jump_down) {
		process_jump_down(jump_delta, jump_origin, epsilon);
	}
}

void rotate_object(object* object_to_rotate) {
	time_t rawtime = time(NULL);
	if (object_to_rotate->rotation_begin == NULL)
		return;

	double difference_seconds = difftime(rawtime, object_to_rotate->rotation_begin);
	if (difference_seconds < 5)
		object_to_rotate->rotation_y_angle += 10.f;
	else
		object_to_rotate->is_visible = false;
}


void rotate_objects(void) {
	for (int i = 0; i < objects_count; i++) {
		rotate_object(&objects[i]);
	}

	for (int i = 0; i < custom_objects_count; i++) {
		rotate_object(&custom_objects[i]);
	}
}


/* This function will registered as a callback with glutIdleFunc. Here it will
   be constantly called and perform updates to realise an animation. */
void idleFunc(void) {
	//angleCube += 0.1f; // TASK 6:
	rotate_objects();
	process_jump();
	glutPostRedisplay();
}

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

void init_object_data(void) {
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
	init_object_data();
	init_custom_object_data();
	

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	init_window();

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_CULL_FACE);

	init_callbacks();

	glutMainLoop();
}

int main(int argc, char** argv) {
	init(argc, argv);
	return 0;
}