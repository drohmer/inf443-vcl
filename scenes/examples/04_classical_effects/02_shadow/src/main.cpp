#include "vcl/vcl.hpp"

#include <iostream>
#include <list>

#include "shadow_map.hpp"

using namespace vcl;

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	mesh_drawable global_frame;
	bool cursor_on_gui;
	bool display_frame = true;
};
user_interaction_parameters user;



void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);
void initialize_data();
void display_interface();


scene_environment scene;

timer_basic timer;
mesh_drawable sphere;
mesh_drawable ground;
mesh_drawable cube;
mesh_drawable quad;



void initialize_data()
{
	//Set the shader that will display a mesh with the shadow (set it as default shader)
	GLuint const shader_mesh_with_shadow = opengl_create_shader_program(read_text_file("shader/mesh_with_shadow.vert.glsl"), read_text_file("shader/mesh_with_shadow.frag.glsl"));
	mesh_drawable::default_shader = shader_mesh_with_shadow;
	mesh_drawable::default_texture = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});

	// Initialize the light position and viewpoint
	scene.projection_light = projection_orthographic(-10,10,-10,10,0,30); // orthographic projection for simplicity
	scene.light.distance_to_center = 5.0f;
	scene.light.manipulator_rotate_spherical_coordinates(pi/4.0f, pi/4.0f);

	// Initialize the camera position and viewpoint
	scene.camera.distance_to_center = 5.0f;
	scene.camera.manipulator_rotate_spherical_coordinates(pi/6.0f, pi/4.0f);

	// Create the shapes
	user.global_frame = mesh_drawable(mesh_primitive_frame());
	ground = mesh_drawable( mesh_primitive_quadrangle({-10,-0.5f,-10}, {-10, -0.5f,10}, { 10, -0.5f,10}, { 10,-0.5f,-10}) );
	sphere = mesh_drawable( mesh_primitive_sphere() );
	cube = mesh_drawable(mesh_primitive_cube());
	cube.transform.translate = {1,2,0};

	// Initialize the FBO and texture used to handle the depth map
	scene.depth_map = initialize_depth_map();
}





void display_frame()
{
	timer.update();
	float const t = timer.t;
	sphere.transform.translate = {2.0f*std::cos(t), 1.0f, -2.0f};
	cube.transform.rotate = rotation({0,1,0}, t);

	// First pass: Draw all shapes that cast shadows
	{
		glViewport(0, 0, scene.depth_map.width, scene.depth_map.height);
		glBindFramebuffer(GL_FRAMEBUFFER, scene.depth_map.fbo); opengl_check;
		glClear(GL_DEPTH_BUFFER_BIT);

		draw_depth_map(sphere, scene);
		if(user.display_frame)
			draw_depth_map(user.global_frame, scene);
		draw_depth_map(cube, scene);

		glBindFramebuffer(GL_FRAMEBUFFER, 0); opengl_check;
		
	}

	// Second pass: Draw all shapes that receives shadows
	{
		glViewport(0, 0, scene.window_width, scene.window_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if(user.display_frame)
			draw_with_shadow(user.global_frame, scene);
		draw_with_shadow(ground, scene);
		draw_with_shadow(sphere, scene);
		draw_with_shadow(cube, scene);
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
		//scene.light = scene.camera.matrix_view();
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



void display_interface()
{
	ImGui::Checkbox("Frame", &user.display_frame);

}


void window_size_callback(GLFWwindow* , int width, int height)
{
	scene.window_width = width;
	scene.window_height = height;
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




