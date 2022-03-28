#pragma once

void draw_objects();

void process_jump_up(float jump_delta, float jump_height_max, float epsilon);
void draw_custom_objects();
void draw_maze_cube(int i, int j);
void process_jump_down(float jump_delta, float jump_origin, float epsilon);
void process_jump();

void init_callbacks();

void init_window();

void init(int argc, char** argv);

void mouse_move_up_or_down(int ycoor);

void mouse_move_left_or_right(int xcoor);
