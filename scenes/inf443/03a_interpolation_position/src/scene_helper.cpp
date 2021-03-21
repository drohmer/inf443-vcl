#include "scene_helper.hpp"

using namespace vcl;

void display_keypositions(mesh_drawable& sphere, buffer<vec3> const& key_positions, scene_environment const& scene, picking_structure const& picking)
{
	size_t const N = key_positions.size();
	for (size_t k = 0; k < N; ++k) {
		if(picking.active && picking.index==k)
			sphere.shading.color = {1,1,0};
		else
			sphere.shading.color = {0,0,1};
		sphere.transform.translate = key_positions[k];
		draw(sphere, scene);
	}
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", current_scene.camera.matrix_view());
	opengl_uniform(shader, "light", current_scene.light, false);
}

void picking_position(picking_structure& picking, buffer<vec3>& key_positions, glfw_state const& state, scene_environment const& scene, vec2 const& p)
{
	if(state.key_shift)
	{
		if (!state.mouse_click_left)
		{
			vec3 const ray_direction = camera_ray_direction(scene.camera.matrix_frame(),scene.projection_inverse, p);
			vec3 const ray_origin = scene.camera.position();

			int index=0;
			intersection_structure intersection = intersection_ray_spheres_closest(ray_origin, ray_direction, key_positions, 0.06f, &index);
			if (intersection.valid == true) {
				picking = {true, index};
			}
			else
				picking.active = false;
		}

		if (state.mouse_click_left && picking.active)
		{
			// Get vector orthogonal to camera orientation
			vec3 const n = scene.camera.front();
			// Compute intersection between current ray and the plane orthogonal to the view direction and passing by the selected object
			vec3 const ray_direction = camera_ray_direction(scene.camera.matrix_frame(),scene.projection_inverse, p);
			vec3 const ray_origin = scene.camera.position();
			intersection_structure intersection = intersection_ray_plane(ray_origin, ray_direction, key_positions[picking.index], n);
			key_positions[picking.index] = intersection.position;

		}
	}
	else
		picking.active = false;
}