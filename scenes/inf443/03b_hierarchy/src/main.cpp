#include "vcl/vcl.hpp"
#include <iostream>


using namespace vcl;

struct scene_environment
{
	camera_around_center camera;
	mat4 projection;
	vec3 light;
};
scene_environment scene;

struct gui_parameters {
	bool display_frame = false;
	bool display_surface = true;
	bool display_wireframe = false;
};

struct user_interaction_parameters {
	vec2 mouse_prev;
	timer_fps fps_record;
	mesh_drawable global_frame;
	gui_parameters gui;
	bool cursor_on_gui;
};
user_interaction_parameters user;


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);


void initialize_data();
void display_interface();
void display_frame();

timer_interval timer;
hierarchy_mesh_drawable hierarchy;

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
	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});

	user.global_frame = mesh_drawable(mesh_primitive_frame());
	user.gui.display_frame = false;
	scene.camera.distance_to_center = 2.5f;
	scene.camera.look_at({-0.5f,2.5f,1}, {0,0,0}, {0,0,1});
	

	// Definition of the elements of the hierarchy
	// ------------------------------------------- //

	float const radius_body = 0.25f;
    float const radius_arm = 0.05f;
    float const length_arm = 0.2f;

    // The geometry of the body is a sphere
    mesh_drawable body = mesh_drawable( mesh_primitive_sphere(radius_body, {0,0,0}, 40, 40));

	// Geometry of the eyes: black spheres
	mesh_drawable eye = mesh_drawable(mesh_primitive_sphere(0.05f, {0,0,0}, 20, 20));
    eye.shading.color = {0,0,0};

	// Shoulder part and arm are displayed as cylinder
    mesh_drawable shoulder_left = mesh_drawable(mesh_primitive_cylinder(radius_arm, {0,0,0}, {-length_arm,0,0}));
    mesh_drawable arm_left = mesh_drawable(mesh_primitive_cylinder(radius_arm, {0,0,0}, {-length_arm/1.5f,0,-length_arm/1.0f}));

	mesh_drawable shoulder_right = mesh_drawable(mesh_primitive_cylinder(radius_arm, {0,0,0}, {length_arm,0,0}));
    mesh_drawable arm_right = mesh_drawable(mesh_primitive_cylinder(radius_arm, {0,0,0}, {length_arm/1.5f,0,-length_arm/1.0f}));

	// An elbow displayed as a sphere
    mesh_drawable elbow = mesh_drawable(mesh_primitive_sphere(0.055f));


	// Build the hierarchy:
	// ------------------------------------------- //
    // Syntax to add element
    //   hierarchy.add(visual_element, element_name, parent_name, (opt)[translation, rotation])

	// The root of the hierarchy is the body
	hierarchy.add(body, "body");

	// Eyes positions are set with respect to some ratio of the body
	hierarchy.add(eye, "eye_left", "body" , radius_body * vec3( 1/3.0f, 1/2.0f, 1/1.5f));
	hierarchy.add(eye, "eye_right", "body", radius_body * vec3(-1/3.0f, 1/2.0f, 1/1.5f));

	// Set the left part of the body arm: shoulder-elbow-arm
    hierarchy.add(shoulder_left, "shoulder_left", "body", {-radius_body+0.05f,0,0}); // extremity of the spherical body
    hierarchy.add(elbow, "elbow_left", "shoulder_left", {-length_arm,0,0});          // place the elbow the extremity of the "shoulder cylinder"
    hierarchy.add(arm_left, "arm_bottom_left", "elbow_left");                        // the arm start at the center of the elbow

	// Set the right part of the body arm: similar to the left part with a symmetry in x direction
    hierarchy.add(shoulder_right, "shoulder_right", "body", {radius_body-0.05f,0,0});
    hierarchy.add(elbow, "elbow_right", "shoulder_right", {length_arm,0,0});
    hierarchy.add(arm_right, "arm_bottom_right", "elbow_right");
}



void display_frame()
{
	// Update the current time
	timer.update();
	float const t = timer.t;


	/** *************************************************************  **/
    /** Compute the (animated) transformations applied to the elements **/
    /** *************************************************************  **/

    // The body oscillate along the z direction
    hierarchy["body"].transform.translate = {0,0.2f*(1+std::sin(2*3.14f*t)),0};

	// Rotation of the shoulder-left around the y axis
    hierarchy["shoulder_left"].transform.rotate   = rotation({0,0,1}, std::sin(2*3.14f*(t-0.4f)) );
	// Rotation of the arm-left around the y axis (delayed with respect to the shoulder)
    hierarchy["arm_bottom_left"].transform.rotate = rotation({0,0,1}, std::sin(2*3.14f*(t-0.6f)) );
		
	// Rotation of the shoulder-right around the y axis
    hierarchy["shoulder_right"].transform.rotate = rotation({0,0,-1}, std::sin(2*3.14f*(t-0.4f)) );
    // Rotation of the arm-right around the y axis (delayed with respect to the shoulder)
    hierarchy["arm_bottom_right"].transform.rotate = rotation({0,0,-1}, std::sin(2*3.14f*(t-0.6f)) );
	
	// update the global coordinates
	hierarchy.update_local_to_global_coordinates();

	// display the hierarchy
	if(user.gui.display_surface)
		draw(hierarchy, scene);
	if(user.gui.display_wireframe)
		draw_wireframe(hierarchy, scene);

}


void display_interface()
{
	ImGui::SliderFloat("Time", &timer.t, timer.t_min, timer.t_max);
	ImGui::SliderFloat("Time scale", &timer.scale, 0.0f, 2.0f);
	ImGui::Checkbox("Frame", &user.gui.display_frame);
	ImGui::Checkbox("Surface", &user.gui.display_surface);
	ImGui::Checkbox("Wireframe", &user.gui.display_wireframe);
}


void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	float const fov = 50.0f * pi /180.0f;
	float const z_min = 0.1f;
	float const z_max = 100.0f;
	scene.projection = projection_perspective(fov, aspect, z_min, z_max);
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





