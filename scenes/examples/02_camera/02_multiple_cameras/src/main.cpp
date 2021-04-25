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
};
user_interaction_parameters user;

struct scene_environment
{
	camera_around_center camera;
	mat4 projection;
	vec3 light;
};


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);
void initialize_data();
void display_interface();

// Create two scene (each scene has its own camera)
scene_environment scene;
scene_environment scene_orthographic;

timer_basic timer;
mesh_drawable quad;
mesh_drawable cube;


void init_meshes_and_scene()
{
	// Set the second scene with an orthographic project
	scene_orthographic.projection = projection_orthographic(-1,1,-1,1,0,1);
	scene_orthographic.camera.distance_to_center = 0.5f;
	scene_orthographic.light = {0,0,1};

	// Create two basic objects
	quad = mesh_drawable( mesh_primitive_quadrangle({-0.5f,0.7f,0},{0.5f,0.7f,0},{0.5f,1.0f,0},{-0.5f,1.0f,0}) );
	quad.texture = opengl_texture_to_gpu( image_load_png("assets/text.png") );

	cube = mesh_drawable(mesh_primitive_cube({0,0,0},0.2f));
	cube.transform.translate = {0.75f,0.8f,0.0f};
	cube.texture = opengl_texture_to_gpu( image_load_png("assets/wood.png") );
}

void mesh_display()
{
	timer.update();
	cube.transform.rotate = rotation({1,0,0},1.1f*pi/2.0f)*rotation({0,0,1}, timer.t);

	// Display the two objects using the two scenes (using different camera and projection)

	// Scene has the default user-controled camera and a perspective projection
	draw(quad, scene);
	draw(cube, scene);

	// Scene_orthographic has a fixed camera and an orthographic projection
	draw(quad, scene_orthographic);
	draw(cube, scene_orthographic);
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
	scene.camera.distance_to_center = 10.0f;
	scene.camera.look_at({3,1,2}, {0,0,0.5}, {0,0,1});

	init_meshes_and_scene();
}


void display_interface()
{
	ImGui::Checkbox("Frame", &user.display_frame);
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

void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.1f, 100.0f);
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", current_scene.camera.matrix_view());
	opengl_uniform(shader, "light", current_scene.light, false);
}



