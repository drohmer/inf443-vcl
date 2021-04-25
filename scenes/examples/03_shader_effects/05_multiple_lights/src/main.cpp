#include "vcl/vcl.hpp"
#include <iostream>
#include <list>


using namespace vcl;

struct gui_parameters {
	bool display_frame = true;
};

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	mesh_drawable global_frame;
	gui_parameters gui;
	bool cursor_on_gui;
};
user_interaction_parameters user;


struct scene_environment
{
	camera_around_center camera;
	mat4 projection;

	// Consider a set of spotlight defined by their position and color
	std::array<vec3,5> spotlight_position;
	std::array<vec3,5> spotlight_color;
	float spotlight_falloff = 0.5;
	float fog_falloff = 0.005f;
};
scene_environment scene;



void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);
void initialize_data();
void display_interface();



timer_basic timer;
mesh_drawable cube;
mesh_drawable sphere_spotlight;
mesh_drawable ground;


void initialize_data()
{
	// Load a new custom shader that take into account spotlights (note the new shader file in shader/ directory)
	GLuint const shader_mesh = opengl_create_shader_program(read_text_file("shader/mesh_lights.vert.glsl"),read_text_file("shader/mesh_lights.frag.glsl"));

	// set it as default shader
	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});

	user.global_frame = mesh_drawable(mesh_primitive_frame());
	scene.camera.distance_to_center = 10.0f;
	scene.camera.look_at({3,1,2}, {0,0,0.5}, {0,0,1});

	//initialize the meshes
	cube = mesh_drawable( mesh_primitive_cube() );
	cube.transform.translate = {-2,0,0.5f};
	cube.transform.rotate = rotation({0,0,1},pi/4.0f);

	
	mesh sphere_spotlight_mesh = mesh_primitive_sphere(0.05f);
	sphere_spotlight_mesh.flip_connectivity();
	sphere_spotlight = mesh_drawable( sphere_spotlight_mesh );

	ground = mesh_drawable(mesh_primitive_quadrangle({-30,-30,0},{-30,30,0},{30,30,0},{30,-30,0}));
}


void mesh_display()
{
	timer.update();
	float t = timer.t;
	
	// set the values for the spotlights (possibly varying in time)
	scene.spotlight_color[0] = {1.0f, 0.0f, 0.0f};
	scene.spotlight_position[0] = {std::cos(t), std::sin(t), 0.5+0.2*std::cos(3*t)};

	scene.spotlight_color[1] = {0.0f, 1.0f, 0.0f};
	scene.spotlight_position[1] = {std::cos(0.5*t+pi/2), std::sin(0.5*t+pi/2), 0.5+0.2*std::cos(2*t)};
	
	scene.spotlight_position[2] = {0,0,1.05f};
	scene.spotlight_color[2] = 2*(std::cos(t)+1.0f)/2.0*vec3(1,1,1);

	scene.spotlight_position[3] = {3*std::cos(t), 3*std::sin(t), 0.5+0.2*std::cos(3*t)};
	scene.spotlight_color[3] = vec3( (std::cos(t)+1)/2,0,1);

	scene.spotlight_position[4] = {-3.0f,-3.0f,0.05f};
	scene.spotlight_color[4] = {1.0f, 0.9f, 0.5f};

	// display the spotlights as small spheres
	for (size_t k = 0; k < scene.spotlight_position.size(); ++k)
	{
		sphere_spotlight.transform.translate = scene.spotlight_position[k];
		sphere_spotlight.shading.color = scene.spotlight_color[k];
		draw(sphere_spotlight, scene);
	}
	
	// display elements
	draw(cube, scene);
	draw(ground, scene);
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", current_scene.camera.matrix_view());

	// Adapt the uniform values send to the shader
	int const N_spotlight = current_scene.spotlight_color.size();
	GLint const location_color    = glGetUniformLocation(shader, "spotlight_color");
	GLint const location_position = glGetUniformLocation(shader, "spotlight_position");
	glUniform3fv(location_color, N_spotlight, ptr(current_scene.spotlight_color[0]));
	glUniform3fv(location_position, N_spotlight, ptr(current_scene.spotlight_position[0]));

	/** Note: Here we use the raw OpenGL call to glUniform3fv allowing us to pass a vector of data (here an array of 5 positions and 5 colors) */

	opengl_uniform(shader, "spotlight_falloff", current_scene.spotlight_falloff);
	opengl_uniform(shader, "fog_falloff", current_scene.fog_falloff);
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
		user.fps_record.update();
		
		glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		imgui_create_frame();
		if(user.fps_record.event) {
			std::string const title = "VCL Display - "+str(user.fps_record.fps)+" fps";
			glfwSetWindowTitle(window, title.c_str());
		}

		ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);
		user.cursor_on_gui = ImGui::GetIO().WantCaptureMouse;

		if(user.gui.display_frame) draw(user.global_frame, scene);

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





void display_interface()
{
	ImGui::Checkbox("Frame", &user.gui.display_frame);
	ImGui::SliderFloat("Light falloff", &scene.spotlight_falloff, 0, 1.0f, "%0.4f", 2.0f);
	ImGui::SliderFloat("Fog falloff", &scene.fog_falloff, 0, 0.05f, "%0.5f", 2.0f);
}


void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.1f, 100.0f);
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




