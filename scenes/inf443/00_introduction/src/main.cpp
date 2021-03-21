/**
	Introduction to use the VCL library
*/

#include "vcl/vcl.hpp"
#include <iostream>

// Add vcl namespace within the current one - Allows to use function from vcl library without explicitely preceeding their name with vcl::
using namespace vcl;


// ****************************************** //
// General structures declaration
// ****************************************** //

// Structure storing user-related interaction data and GUI parameter
struct user_interaction_parameters {
	vec2 mouse_prev;      // Current position of the mouse
	bool cursor_on_gui;   // Indicate if the cursor is on the GUI widget
	timer_fps fps_record; // Handle the FPS display
	bool display_frame;   // Boolean indicating wheather the global frame is displayed
};
user_interaction_parameters user;   // (declaration of user as a global variable)

// Structure storing the global variable of the 3D scene
//   can be use to send uniform parameter when displaying a shape
struct scene_environment
{
	camera_around_center camera; // A camera looking at, and rotating around, a specific center position
	mat4 projection;             // The perspective projection matrix
	vec3 light;                  // Position of the light in the scene
};
scene_environment scene;   // (declaration of scene as a global variable)



// ****************************************** //
// Functions signatures
// ****************************************** //

// Callback functions
//   Functions called when a corresponding event is received by GLFW (mouse move, keyboard, etc).
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);

// Specific functions:
//    Initialize the data of this scene - executed once before the start of the animation loop.
void initialize_data();
//    Display calls - executed a each frame
void display_scene();


// ****************************************** //
// Declaration of Global variables
// ****************************************** //
mesh_drawable frame_visual;
mesh_drawable sphere;
mesh_drawable plane;



// Main function with creation of the scene and animation loop
int main(int, char* argv[])
{

	std::cout << "Run " << argv[0] << std::endl;

	// create GLFW window and initialize OpenGL
	int const width = 1280, height = 1024;
	GLFWwindow* window = create_window(width, height);
	window_size_callback(window, width, height);
	std::cout << opengl_info_display() << std::endl;;

	imgui_init(window);  // Initialize GUI library

	// Set GLFW callback functions
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
		
		// Clear screen
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Create GUI interface for the current frame
		imgui_create_frame();
		if(user.fps_record.event) {
			std::string const title = "VCL Display - "+str(user.fps_record.fps)+" fps";
			glfwSetWindowTitle(window, title.c_str());
		}
		ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);
		user.cursor_on_gui = ImGui::IsAnyWindowFocused();

		// Update the gui variable
		ImGui::Checkbox("Display Frame", &user.display_frame);

		// Display the shapes of the scenes:
		display_scene();

		// Display GUI
		ImGui::End();
		imgui_render_frame(window);
		// Swap buffer and handle GLFW events
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
	// Load and set the common shaders
	// *************************************** //

	//   - Shader used to display meshes
	GLuint const shader_mesh = opengl_create_shader_program(opengl_shader_preset("mesh_vertex"), opengl_shader_preset("mesh_fragment"));
	//   - Shader used to display constant color (ex. for curves)
	GLuint const shader_uniform_color = opengl_create_shader_program(opengl_shader_preset("single_color_vertex"), opengl_shader_preset("single_color_fragment"));
	//   - Default white texture
	GLuint const texture_white = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});

	// Set default shader and texture to drawable mesh
	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = texture_white;


	// Set the initial position of the camera
	// *************************************** //
	vec3 const camera_position = {2.0f, -3.5f, 2.0f}; // position of the camera in space
	vec3 const camera_target_position = {0,0,0};      // position the camera is looking at / point around which the camera rotates
	vec3 const up = {0,0,1};                          // approximated "up" vector of the camera
	scene.camera.look_at(camera_position, camera_target_position, up); 


	// Prepare the objects visible in the scene
	// *************************************** //

	// Create a visual frame representing the coordinate system
	frame_visual  = mesh_drawable(mesh_primitive_frame());

	// Initialize a sphere (centered at the origin with prescribed radius)
	sphere = mesh_drawable(mesh_primitive_sphere(0.1f)); 
	sphere.transform.translate = {0.0f, 0.5f, 0.5f}; // translate the position of the sphere
	sphere.shading.color = {1.0f, 0.6f, 0.6f};       // set the color of the sphere

	// Initialize a quadrangle
	plane  = mesh_drawable(mesh_primitive_quadrangle());
	plane.shading.color = {1.0f, 1.0f, 0.8f};        // set the color of the plane


}



void display_scene()
{
	// the general syntax to display a mesh_drawable is:
    //   draw(objectDrawableName, scene);
	//     Note: scene is used to set the uniform parameters associated to the camera, light, etc. to the shader

	if(user.display_frame==true)
		draw(frame_visual, scene); // only dislpay the frame when the variable user.display_frame is true
	draw(sphere, scene);
	draw(plane, scene);

	draw_wireframe(plane, scene, {0,0,1}); // display the triangulation of the quadrangle (in blue)

}


// Function called every time the screen is resized
void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);
	scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.1f, 100.0f);
}


// Function called every time the mouse is moved
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

// Uniform data used when displaying an object in this scene
void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", current_scene.camera.matrix_view());
	opengl_uniform(shader, "light", current_scene.light, false);
}



