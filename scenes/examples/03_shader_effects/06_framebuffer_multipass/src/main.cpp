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

mesh_drawable quad;
mesh_drawable camel;

GLuint fbo;                // Frame buffer for multi-pass render
GLuint depth_buffer;       // Depth buffer used when rendered in the frame buffer
GLuint intermediate_image; // Texture of the rendered color image


void draw_screen(mesh_drawable const& drawable);

void initialize_fbo()
{
	// Allocate texture to store the rendered image
	glGenTextures(1, &intermediate_image);
	glBindTexture(GL_TEXTURE_2D, intermediate_image);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  

	// Allocate a depth buffer when using the frame buffer
	glGenRenderbuffers(1, &depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1280, 1024);

	// Create frame buffer
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);  
	// associate the texture to the frame buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, intermediate_image, 0);   
	// associate the depth-buffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
}

void initialize_mesh()
{
	// the object displayed in the first pass
	camel = mesh_drawable( mesh_load_file_obj("assets/camel.obj") );
	camel.transform.scale = 0.5f;
	camel.transform.translate = {-0.5f,0.5f,0.25f};

	// Shader computing an image filter in the second pass
	GLuint const shader_screen_render = opengl_create_shader_program( read_text_file("shader/screen_render.vert.glsl"), read_text_file("shader/screen_render.frag.glsl"));
	
	// The result of the second pass is displayed on a quad
	quad = mesh_drawable( mesh_primitive_quadrangle({-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0}) );
	quad.texture = intermediate_image;
	quad.shader = shader_screen_render;
}


void render_first_pass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Display the camel and render it in the texture associated to the FBO
	draw(camel, scene);
	
	if(user.display_frame) 
		draw(user.global_frame, scene);
}

void render_second_pass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
	glClear(GL_COLOR_BUFFER_BIT);

	// Display the quad using the FBO texture, and use the shader applying the filter
	draw_screen(quad);
}


// Special draw call for the quad
//  - Similar to the standard draw associated to mesh_drawable, but with less uniform parameters
void draw_screen(mesh_drawable const& drawable)
{
	// Setup shader
	assert_vcl(drawable.shader!=0, "Try to draw mesh_drawable without shader");
	assert_vcl(drawable.texture!=0, "Try to draw mesh_drawable without texture");
	glUseProgram(drawable.shader); opengl_check;

	// Set texture
	glActiveTexture(GL_TEXTURE0); opengl_check;
	glBindTexture(GL_TEXTURE_2D, drawable.texture); opengl_check;
	opengl_uniform(drawable.shader, "image_texture", 0, false);  opengl_check;
		
	// Call draw function
	assert_vcl(drawable.number_triangles>0, "Try to draw mesh_drawable with 0 triangles"); opengl_check;
	glBindVertexArray(drawable.vao);   opengl_check;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.vbo.at("index")); opengl_check;
	glDrawElements(GL_TRIANGLES, GLsizei(drawable.number_triangles*3), GL_UNSIGNED_INT, nullptr); opengl_check;

	// Clean buffers
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
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

		display_interface();

		render_first_pass();
		render_second_pass();


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
	scene.camera.look_at({4,1,1}, {0,0,0}, {0,0,1});

	initialize_fbo();
	initialize_mesh();
}


void display_interface()
{
	ImGui::Checkbox("Frame", &user.display_frame);
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

