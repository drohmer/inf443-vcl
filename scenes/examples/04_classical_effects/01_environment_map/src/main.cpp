#include "vcl/vcl.hpp"

#include <iostream>
#include <list>

#include "environment_map.hpp"

using namespace vcl;

enum shapes_enum {display_sphere, display_cylinder, display_torus, display_camel};

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	mesh_drawable global_frame;
	bool cursor_on_gui;
	bool display_frame = true;
	shapes_enum shape;
};
user_interaction_parameters user;

struct scene_environment
{
	camera_spherical_coordinates camera;
	mat4 projection;
	vec3 light;
};
scene_environment scene;

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);
void initialize_data();
void display_interface();

mesh_drawable cube;


mesh_drawable sphere;
mesh_drawable cylinder;
mesh_drawable torus;
mesh_drawable camel;


void initialize_mesh()
{
	// Read shaders
	GLuint const shader_skybox = opengl_create_shader_program( read_text_file("shader/skybox.vert.glsl"), read_text_file("shader/skybox.frag.glsl"));
	GLuint const shader_environment_map = opengl_create_shader_program( read_text_file("shader/environment_map.vert.glsl"), read_text_file("shader/environment_map.frag.glsl"));

	// Read cubemap texture
	GLuint texture_cubemap = cubemap_texture("assets/skybox/");

	// Cube used to display the skybox
	cube = mesh_drawable( mesh_primitive_cube({0,0,0},2.0f), shader_skybox, texture_cubemap);


	// Possible shapes displayed with the environment map
	sphere = mesh_drawable( mesh_primitive_sphere(), shader_environment_map, texture_cubemap);
	cylinder = mesh_drawable( mesh_primitive_cylinder(0.8f,{-2,0,0},{2,0,0},20,40, true), shader_environment_map, texture_cubemap);
	torus = mesh_drawable(mesh_primitive_torus(1.5f, 0.5f, {0,0,0}, {0,1,0}, 80,25), shader_environment_map, texture_cubemap);
	camel = mesh_drawable( mesh_load_file_obj("assets/camel.obj"), shader_environment_map, texture_cubemap );
	camel.transform.rotate = rotation({0,1,0},pi/2.0f)*rotation({1,0,0},-pi/2.0f);

	sphere.shading.phong = {1,0,0.6f};
	cylinder.shading.phong = {1,0,0.6f};
	torus.shading.phong = {1,0,0.6f};
	camel.shading.phong = {1,0,0.6f};
}




void display_frame()
{
	glDepthMask(GL_FALSE);
	draw_with_cubemap(cube, scene);
	glDepthMask(GL_TRUE);

	if(user.display_frame)
			draw(user.global_frame, scene);

	switch (user.shape) {
	case display_sphere: 
		draw_with_cubemap(sphere, scene); break;
	case display_cylinder: 
		draw_with_cubemap(cylinder, scene); break;
	case display_torus:
		draw_with_cubemap(torus, scene); break;
	case display_camel: 
		draw_with_cubemap(camel, scene); break;
	}

}




int main(int, char* argv[])
{
	std::cout << "Run " << argv[0] << std::endl;

	int const width = 1280, height = 1024;
	GLFWwindow* window = create_window(width, height);
	window_size_callback(window, width, height);
	std::cout << opengl_info_display() << std::endl;;

	imgui_init(window);
	glfwSetCursorPosCallback(window, mouse_move_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);
	
	std::cout<<"Initialize data ..."<<std::endl;
	initialize_data();

	std::cout<<"Start animation loop ..."<<std::endl;
	user.fps_record.start();
	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		scene.light = scene.camera.position();
		user.fps_record.update();
		
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		imgui_create_frame();
		if(user.fps_record.event) {
			std::string const title = "VCL Display - "+str(user.fps_record.fps)+" fps";
			glfwSetWindowTitle(window, title.c_str());
		}

		ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);
		user.cursor_on_gui = ImGui::GetIO().WantCaptureMouse;

		

		display_interface();
		display_frame();


		ImGui::End();
		imgui_render_frame(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	imgui_cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}


void initialize_data()
{
	GLuint const shader_mesh = opengl_create_shader_program(opengl_shader_preset("mesh_vertex"), opengl_shader_preset("mesh_fragment"));
	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});

	user.global_frame = mesh_drawable(mesh_primitive_frame());

	initialize_mesh();
}


void display_interface()
{
	ImGui::Checkbox("Frame", &user.display_frame);

	ImGui::Text("Surface to display:");
	int* ptr_shape_display  = reinterpret_cast<int*>(&user.shape);
	ImGui::RadioButton("Sphere", ptr_shape_display, display_sphere); ImGui::SameLine();
	ImGui::RadioButton("Cylinder", ptr_shape_display, display_cylinder); ImGui::SameLine();
	ImGui::RadioButton("Torus", ptr_shape_display, display_torus); ImGui::SameLine();
	ImGui::RadioButton("Camel", ptr_shape_display, display_camel);

}


void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.01f, 100.0f);
}


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	vec2 const  p1 = glfw_get_mouse_cursor(window, xpos, ypos);
	vec2 const& p0 = user.mouse_prev;
	glfw_state state = glfw_current_state(window);

	auto& camera = scene.camera;
	if(!user.cursor_on_gui){
		if(state.mouse_click_left && !state.key_ctrl)
			scene.camera.manipulator_rotate_spherical_coordinates((p1-p0).x, -(p1-p0).y);
		if(state.mouse_click_left && state.key_ctrl)
			camera.manipulator_translate_in_plane(p1-p0);
		if(state.mouse_click_right)
			camera.manipulator_scale_distance_to_center( (p1-p0).y );
	}

	user.mouse_prev = p1;
}


void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", current_scene.camera.matrix_view());
	opengl_uniform(shader, "light", current_scene.light, false);
}

