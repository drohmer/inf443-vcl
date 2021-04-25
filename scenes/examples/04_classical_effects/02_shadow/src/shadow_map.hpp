#pragma once

#include "vcl/vcl.hpp"


struct shadow_map_parameters 
{
	GLuint texture;
	int width;
	int height;
	GLuint fbo;
	GLuint shader;
};

struct scene_environment
{
	vcl::camera_spherical_coordinates camera;
	vcl::mat4 projection;

	vcl::camera_spherical_coordinates light;
	vcl::mat4 projection_light;

	int window_width, window_height;

	shadow_map_parameters depth_map;
};


shadow_map_parameters initialize_depth_map();
void draw_with_shadow(vcl::mesh_drawable const& drawable, scene_environment const& scene);
void draw_depth_map(vcl::mesh_drawable const& drawable, scene_environment const& current_scene);
void opengl_uniform(GLuint shader, scene_environment const& current_scene);