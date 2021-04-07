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
	vec3 light;
};
scene_environment scene;



void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);
void display_segment(vec3 const& a, vec3 const& b);

void initialize_data();
void display_interface();
void display_frame();



mesh_drawable sphere;
mesh_drawable disc;
timer_event_periodic timer(0.6f);

segments_drawable segments;

vec3 pA, vA;
vec3 pB, vB;
float L0;
float L0_array;


/** Compute spring force applied on particle pi from particle pj */
vec3 spring_force(vec3 const& p_i, vec3 const& p_j, float L_0, float K)
{
	// TO DO: correct the computation of this force value
    return {0,0,0};
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
	segments_drawable::default_shader = shader_single_color;
	segments = segments_drawable( {{0,0,0},{1,0,0}} );

	user.global_frame = mesh_drawable(mesh_primitive_frame());
	scene.camera.distance_to_center = 10.0f;
	scene.camera.look_at({3,1,2}, {0,0,0.5}, {0,0,1});

	sphere = mesh_drawable( mesh_primitive_sphere(0.05f));

    // Initial position and speed of particles
    // ******************************************* //
    pA = {0,0,0};     // Initial position of particle A
    vB = {0,0,0};     // Initial speed of particle A

    pB = {0.0f,0.45f,0.0f};  // Initial position of particle B
    vB = {0,0,0};     // Initial speed of particle B

    L0 = 0.4f; // Rest length between A and B
	

}


void display_frame()
{

	 // Simulation time step (dt)
    float const dt = timer.scale*0.01f;

    // Simulation parameters
    float const m  = 0.01f;        // particle mass
    float const K  = 5.0f;        // spring stiffness
    float const mu = 0.01f;       // damping coefficient

    vec3 const g   = {0,0,-9.81f}; // gravity

    // Forces
    vec3 const fB_spring  = spring_force(pB, pA, L0, K);
    vec3 const fB_weight  =  m * g;
	vec3 const fB_damping =  -mu*vB;
    vec3 const fB = fB_spring + fB_weight + fB_damping;

	// Numerical Integration (Verlet)
    {
        // Only particle B should be updated
        vB = (1-mu)*vB + dt * fB / m;
		pB = pB + dt * vB;
    }

	// Display of the result

    // particle pa
    sphere.transform.translate = pA;
    sphere.shading.color = {0,0,0};
    draw(sphere, scene);

    // particle pb
    sphere.transform.translate = pB;
    sphere.shading.color = {1,0,0};
    draw(sphere, scene);

	display_segment(pA, pB);

}


void display_segment(vec3 const& a, vec3 const& b)
{
	segments.update({a,b});
	draw(segments, scene);
}


void display_interface()
{
	ImGui::Checkbox("Frame", &user.gui.display_frame);
	ImGui::SliderFloat("Scale", &timer.scale, 0.0f, 3.0f, "%.3f", 2.0f);
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

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", current_scene.camera.matrix_view());
	opengl_uniform(shader, "light", current_scene.light, false);
}



