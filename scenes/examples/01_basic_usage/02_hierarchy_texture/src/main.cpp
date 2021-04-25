#include "vcl/vcl.hpp"
#include <iostream>
#include <list>


using namespace vcl;

struct gui_parameters {
	bool display_frame = true;
	bool wireframe = false;
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
void initialize_data();
void display_interface();


timer_basic timer;
hierarchy_mesh_drawable hierarchy;


void hierarchy_load()
{

	/** Examples of hierarchy with elements associated to different textures 
	* Note that the shapes can be procedural primitives, or even external mesh files (like the wings)
	*/

	mesh_drawable body = mesh_drawable(mesh_primitive_ellipsoid({1.0f, 1.0f, 1.5f}));
	body.texture = opengl_texture_to_gpu( image_load_png("assets/body.png") ); // associate a texture-image to each element

	mesh_drawable head = mesh_drawable(mesh_primitive_sphere(0.9f));
	head.texture = opengl_texture_to_gpu( image_load_png("assets/head.png") );

	mesh_drawable wing = mesh_drawable(mesh_load_file_obj("assets/wing.obj"));
	wing.texture = opengl_texture_to_gpu( image_load_png("assets/wing.png") );

	mesh_drawable wing2 = mesh_drawable(mesh_load_file_obj("assets/wing.obj"));
	wing2.texture = opengl_texture_to_gpu( image_load_png("assets/wing.png") );
	wing2.transform.rotate = rotation({0,0,1}, pi);

	mesh_drawable eye = mesh_drawable(mesh_primitive_sphere(0.2f));
	eye.shading.color = {0.2f,0.2f,0.2f};
	// Note: the eye is displayed with a single color and doesn't need a texture image (it has a default white image associated to it).


	hierarchy.add(body, "body");
	hierarchy.add(head, "head", "body", {0,0,1.8});
	hierarchy.add(eye, "eye_R", "head", {0.4f,0.3f,0.6f});
	hierarchy.add(eye, "eye_L", "head", {-0.4f,0.3f,0.6f});
	hierarchy.add(wing, "wing", "body", {0,1.03f,0});
	hierarchy.add(wing2, "wing2", "body", {0,1.03f,0});
	
}

void hierarchy_display()
{
	timer.update();
	float const t = timer.t;
	hierarchy["body" ].transform.translate = {0,0.2f*std::cos(5*t),0};
	hierarchy["wing" ].transform.rotate = rotation({0,0,1}, pi/8.0f*(1+std::cos(20*t)) );
	hierarchy["wing2"].transform.rotate = rotation({0,0,-1}, pi/8.0f*(1+std::cos(20*t)) );
	hierarchy["head" ].transform.translate = vec3{0,0,1.8}+vec3{0,0,0.1f*std::cos(2.5*t)};
	hierarchy["head" ].transform.rotate = rotation({0,1,0}, pi/8.0f*std::cos(5*t) );

	hierarchy.update_local_to_global_coordinates();
	draw(hierarchy, scene);
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
		hierarchy_display();


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
	scene.camera.distance_to_center = 10.0f;
	scene.camera.look_at({3,1,2}, {0,0,0.5}, {0,0,1});

	hierarchy_load();
}



void display_interface()
{
	ImGui::Checkbox("Frame", &user.gui.display_frame);
	ImGui::Checkbox("Wireframe", &user.gui.wireframe);
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



