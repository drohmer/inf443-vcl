#include "vcl/vcl.hpp"
#include <iostream>
#include "interpolation.hpp"
#include "scene_helper.hpp"

using namespace vcl;

user_interaction_parameters user;
scene_environment scene;

buffer<vec3> key_positions;
buffer<float> key_times;
timer_interval timer;


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);


void initialize_data();
void display_interface();
void display_frame();

mesh_drawable sphere_current;    // sphere used to display the interpolated value
mesh_drawable sphere_keyframe;   // sphere used to display the key positions
curve_drawable polygon_keyframe; // Display the segment between key positions
trajectory_drawable trajectory;  // Temporary storage and display of the interpolated trajectory


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
		user.cursor_on_gui = ImGui::IsAnyWindowFocused();

		if(user.gui.display_frame) draw(user.global_frame, scene);

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
	// Basic setups of shaders and camera
	GLuint const shader_mesh = opengl_create_shader_program(opengl_shader_preset("mesh_vertex"), opengl_shader_preset("mesh_fragment"));
	GLuint const shader_single_color = opengl_create_shader_program(opengl_shader_preset("single_color_vertex"), opengl_shader_preset("single_color_fragment"));
	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});
	curve_drawable::default_shader = shader_single_color;

	user.global_frame = mesh_drawable(mesh_primitive_frame());
	user.gui.display_frame = false;
	scene.camera.distance_to_center = 2.5f;
	scene.camera.look_at({-3,1,2}, {0,0,0.5}, {0,0,1});


	// Definition of the initial data
	//--------------------------------------//
    // Key positions
	key_positions = { {-1,1,0}, {0,1,0}, {1,1,0}, {1,2,0}, {2,2,0}, {2,2,1}, {2,0,1.5}, {1.5,-1,1}, {1.5,-1,0}, {1,-1,0}, {0,-0.5,0}, {-1,-0.5,0}};
	// Key times
	key_times = {0.0f, 1.0f, 2.0f, 2.5f, 3.0f, 3.5f, 3.75f, 4.5f, 5.0f, 6.0f, 7.0f, 8.0f};


	// Set timer bounds
    //  You should adapt these extremal values to the type of interpolation
	size_t const N = key_times.size();
	timer.t_min = key_times[0];    // Start the timer at the first time of the keyframe
    timer.t_max = key_times[N-1];  // Ends the timer at the last time of the keyframe
    timer.t = timer.t_min;
	

	// Initialize drawable structures
	sphere_keyframe = mesh_drawable( mesh_primitive_sphere(0.05f) );
	sphere_current  = mesh_drawable( mesh_primitive_sphere(0.06f) );
	sphere_keyframe.shading.color = {0,0,1};
	sphere_current.shading.color  = {1,1,0};

	polygon_keyframe = curve_drawable( key_positions );
	polygon_keyframe.color = {0,0,0};
}





void display_frame()
{
	// Sanity check
	assert_vcl( key_times.size()==key_positions.size(), "key_time and key_positions should have the same size");

	// Update the current time
	timer.update();
	float const t = timer.t;
	if( t<timer.t_min+0.1f ) // clear trajectory when the timer restart
        trajectory.clear();

	// Display the key positions
	if(user.gui.display_keyposition)
		display_keypositions(sphere_keyframe, key_positions, scene, user.picking);

	// Display the polygon linking the key positions
	if(user.picking.active) 
		polygon_keyframe.update(key_positions); // update the polygon if needed
	if(user.gui.display_polygon)
		draw(polygon_keyframe, scene);

	// Compute the interpolated position
	vec3 const p = interpolation(t, key_positions, key_times);

	// Display the interpolated position
	sphere_current.transform.translate = p;
	draw(sphere_current, scene);

	// Display the trajectory
	trajectory.visual.color = {1,0,0};
	trajectory.add(p, t);
	draw(trajectory, scene);

}


void display_interface()
{
	ImGui::SliderFloat("Time", &timer.t, timer.t_min, timer.t_max);
	ImGui::SliderFloat("Time scale", &timer.scale, 0.0f, 2.0f);

	ImGui::Checkbox("Frame", &user.gui.display_frame);
	ImGui::Checkbox("Display key positions", &user.gui.display_keyposition);
	ImGui::Checkbox("Display polygon", &user.gui.display_polygon);
	ImGui::Checkbox("Display trajectory", &user.gui.display_trajectory);
	bool new_size = ImGui::SliderInt("Trajectory size", &user.gui.trajectory_storage, 2, 500);

	if(new_size) {
		trajectory.clear();
		trajectory = trajectory_drawable(user.gui.trajectory_storage);
	}


}


void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	float const fov = 50.0f * pi /180.0f;
	float const z_min = 0.1f;
	float const z_max = 100.0f;
	scene.projection = projection_perspective(fov, aspect, z_min, z_max);
	scene.projection_inverse = projection_perspective_inverse(fov, aspect, z_min, z_max);
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	
	vec2 const  p1 = glfw_get_mouse_cursor(window, xpos, ypos);
	vec2 const& p0 = user.mouse_prev;
	glfw_state state = glfw_current_state(window);

	auto& camera = scene.camera;
	if(!user.cursor_on_gui && !state.key_shift){
		if(state.mouse_click_left && !state.key_ctrl)
			scene.camera.manipulator_rotate_trackball(p0, p1);
		if(state.mouse_click_left && state.key_ctrl)
			camera.manipulator_translate_in_plane(p1-p0);
		if(state.mouse_click_right)
			camera.manipulator_scale_distance_to_center( (p1-p0).y );
	}

	// Handle mouse picking
	picking_position(user.picking, key_positions, state, scene, p1);

	user.mouse_prev = p1;
}




