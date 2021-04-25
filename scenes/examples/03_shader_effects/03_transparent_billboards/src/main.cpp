#include "vcl/vcl.hpp"

#include <iostream>
#include <list>



using namespace vcl;



struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	mesh_drawable global_frame;
	bool cursor_on_gui;
	bool display_frame = true;
	bool display_pines = true;
};
user_interaction_parameters user;


struct scene_environment
{
	camera_around_center camera;
	mat4 projection;
	vec3 light;
};
scene_environment scene;


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);
void initialize_data();
void display_interface();


mesh_drawable trunk;
mesh_drawable branches;
mesh_drawable foliage;


void load_quads()
{
	/** Load a shader that makes fully transparent fragments when alpha-channel of the texture is small */
	GLuint const shader_with_transparency = opengl_create_shader_program( read_text_file("shader/transparency.vert.glsl"), read_text_file("shader/transparency.frag.glsl"));

	trunk = mesh_drawable( mesh_load_file_obj("assets/trunk.obj") );
	trunk.texture = opengl_texture_to_gpu( image_load_png("assets/trunk.png") );

	branches = mesh_drawable( mesh_load_file_obj("assets/branches.obj") );
	branches.shading.color = {0.45f, 0.41f, 0.34f}; // branches do not have textures

	foliage = mesh_drawable( mesh_load_file_obj("assets/foliage.obj") );
	foliage.texture = opengl_texture_to_gpu( image_load_png("assets/pine.png") );
	foliage.shader = shader_with_transparency; // set the shader handling transparency for the foliage
	foliage.shading.phong = {0.4f, 0.6f, 0, 1};     // remove specular effect for the billboard

}


void mesh_display()
{
	draw(trunk, scene);
	draw(branches, scene);
	
	if(user.display_pines)
		draw(foliage, scene);
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

		if(user.display_frame) draw(user.global_frame, scene);

		display_interface();
		mesh_display();


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
	scene.camera.look_at({2,2,4}, {0,1.5,0}, {0,1,0});

	load_quads();
}


void display_interface()
{
	ImGui::Checkbox("Frame", &user.display_frame);
	ImGui::Checkbox("Display Pines", &user.display_pines);
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
			scene.camera.manipulator_rotate_trackball(p0, p1);
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

