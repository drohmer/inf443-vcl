#include "vcl/vcl.hpp"
#include <iostream>
#include <list>


using namespace vcl;

struct keyboard_state_parameters{
	bool left  = false; 
	bool right = false; 
	bool up    = false; 
	bool down  = false;
};

struct user_interaction_parameters {
	timer_fps fps_record;
	mesh_drawable global_frame;
	bool cursor_on_gui;
	bool display_frame = true;
	keyboard_state_parameters keyboard_state;
	float speed = 1.0f;
};
user_interaction_parameters user;

struct scene_environment
{
	camera_head camera;
	mat4 projection;
	vec3 light;
};



void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void window_size_callback(GLFWwindow* window, int width, int height);
void initialize_data();
void display_interface();

scene_environment scene;

timer_basic timer;
mesh_drawable terrain;


void mesh_display()
{
	draw(terrain, scene);

	// Handle camera fly-through
	float dt = timer.update();
	scene.camera.position_camera += user.speed*0.1f*dt*scene.camera.front();
	if(user.keyboard_state.up)
		scene.camera.manipulator_rotate_roll_pitch_yaw(0,-0.5f*dt,0);
	if(user.keyboard_state.down)
		scene.camera.manipulator_rotate_roll_pitch_yaw(0, 0.5f*dt,0);
	if(user.keyboard_state.right)
		scene.camera.manipulator_rotate_roll_pitch_yaw(0.7f*dt,0,0);
	if(user.keyboard_state.left)
		scene.camera.manipulator_rotate_roll_pitch_yaw(-0.7f*dt,0,0);
}

// Store keyboard state
// left-right / up-down key
void keyboard_callback(GLFWwindow* , int key, int , int action, int )
{
	if(key == GLFW_KEY_UP){
		if(action == GLFW_PRESS) user.keyboard_state.up = true;
		if(action == GLFW_RELEASE) user.keyboard_state.up = false;
	}

	if(key == GLFW_KEY_DOWN){
		if(action == GLFW_PRESS) user.keyboard_state.down = true;
		if(action == GLFW_RELEASE) user.keyboard_state.down = false;
	}

	if(key == GLFW_KEY_LEFT){
		if(action == GLFW_PRESS) user.keyboard_state.left = true;
		if(action == GLFW_RELEASE) user.keyboard_state.left = false;
	}

	if(key == GLFW_KEY_RIGHT){
		if(action == GLFW_PRESS) user.keyboard_state.right = true;
		if(action == GLFW_RELEASE) user.keyboard_state.right = false;
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
	glfwSetKeyCallback(window, keyboard_callback);
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
	scene.camera.position_camera = {0.5f, 0.5f, -2.0f};
	scene.camera.manipulator_rotate_roll_pitch_yaw(0,0,pi/2.0f);

	terrain = mesh_drawable( mesh_load_file_obj("assets/mountain.obj") );
	terrain.texture = opengl_texture_to_gpu( image_load_png("assets/mountain.png") );
}


void display_interface()
{
	ImGui::Checkbox("Frame", &user.display_frame);
	ImGui::SliderFloat("Speed", &user.speed, 0.2f, 5.0f);
}



void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.005f, 100.0f);
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", current_scene.camera.matrix_view());
	opengl_uniform(shader, "light", current_scene.light, false);
}



