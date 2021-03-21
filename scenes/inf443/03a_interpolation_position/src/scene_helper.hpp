#pragma once


#include "vcl/vcl.hpp"

struct scene_environment
{
	vcl::camera_around_center camera;
	vcl::mat4 projection;
	vcl::mat4 projection_inverse;
	vcl::vec3 light;
};

struct picking_structure {
	bool active; 
	int index;
};

struct gui_parameters {
	bool display_frame = false;
	bool display_polygon = true;
	bool display_keyposition = true;
	bool display_trajectory = true;
	int trajectory_storage = 100;
};

struct user_interaction_parameters {
	vcl::vec2 mouse_prev;
	vcl::timer_fps fps_record;
	vcl::mesh_drawable global_frame;
	gui_parameters gui;
	bool cursor_on_gui;
	picking_structure picking;
};


void display_keypositions(vcl::mesh_drawable& sphere, vcl::buffer<vcl::vec3> const& key_positions, scene_environment const& scene, picking_structure const& picking);

void opengl_uniform(GLuint shader, scene_environment const& current_scene);

void picking_position(picking_structure& picking, vcl::buffer<vcl::vec3>& key_positions, vcl::glfw_state const& state, scene_environment const& scene, vcl::vec2 const& p);