#include "shadow_map.hpp"

using namespace vcl;

shadow_map_parameters initialize_depth_map()
{
	GLuint texture;
	GLuint fbo;
	int width = 1024;
	int height = 1024;

	// Initialize texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Initialize FBO, and attach the texture
	glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0); 

	// Indicate that no color will be rendered
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

	// Clear current FBO to avoid side effects
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// Read the shader used to compute the depth map
	GLuint shader = opengl_create_shader_program( read_text_file("shader/depth_map.vert.glsl"), read_text_file("shader/depth_map.frag.glsl"));

	return shadow_map_parameters{texture, width, height, fbo, shader};
}


void draw_with_shadow(mesh_drawable const& drawable, scene_environment const& scene)
{
	// Setup shader
	assert_vcl(drawable.shader!=0, "Try to draw mesh_drawable without shader");
	assert_vcl(drawable.texture!=0, "Try to draw mesh_drawable without texture");
	glUseProgram(drawable.shader); opengl_check;

	// Send uniforms for this shader
	opengl_uniform(drawable.shader, scene);
	opengl_uniform(drawable.shader, drawable.shading);
	opengl_uniform(drawable.shader, "model", drawable.transform.matrix());

	// Set texture
	glActiveTexture(GL_TEXTURE0); opengl_check;
	glBindTexture(GL_TEXTURE_2D, drawable.texture); opengl_check;
	opengl_uniform(drawable.shader, "image_texture", 0);  opengl_check;

	glActiveTexture(GL_TEXTURE1); opengl_check;
	glBindTexture(GL_TEXTURE_2D, scene.depth_map.texture); opengl_check;
	opengl_uniform(drawable.shader, "shadow_texture", 1);  opengl_check;
		
	// Call draw function
	assert_vcl(drawable.number_triangles>0, "Try to draw mesh_drawable with 0 triangles"); opengl_check;
	glBindVertexArray(drawable.vao);   opengl_check;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.vbo.at("index")); opengl_check;
	glDrawElements(GL_TRIANGLES, GLsizei(drawable.number_triangles*3), GL_UNSIGNED_INT, nullptr); opengl_check;

	// Clean buffers
	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0); 
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void draw_depth_map(mesh_drawable const& drawable, scene_environment const& current_scene)
{
	GLuint shader = current_scene.depth_map.shader;
	glUseProgram(shader); opengl_check;

	// Send uniforms for this shader
	opengl_uniform(shader, "projection", current_scene.projection_light);
	opengl_uniform(shader, "light", current_scene.light.matrix_view());
	opengl_uniform(shader, "model", drawable.transform.matrix());
		
	// Call draw function
	assert_vcl(drawable.number_triangles>0, "Try to draw mesh_drawable with 0 triangles"); opengl_check;
	glBindVertexArray(drawable.vao);   opengl_check;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.vbo.at("index")); opengl_check;
	glDrawElements(GL_TRIANGLES, GLsizei(drawable.number_triangles*3), GL_UNSIGNED_INT, nullptr); opengl_check;

	// Clean buffers
	glBindVertexArray(0);
}

void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", current_scene.camera.matrix_view());
	opengl_uniform(shader, "light", current_scene.light.matrix_view());
	opengl_uniform(shader, "projection_light", current_scene.projection_light);
}