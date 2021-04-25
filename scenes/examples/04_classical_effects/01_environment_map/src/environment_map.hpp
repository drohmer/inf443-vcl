#pragma once

#include "vcl/vcl.hpp"


GLuint cubemap_texture(std::string const& directory_path);

template <typename SCENE>
void draw_with_cubemap(vcl::mesh_drawable const& drawable, SCENE const& current_scene)
{
		// Setup shader
		assert_vcl(drawable.shader!=0, "Try to draw mesh_drawable without shader");
		assert_vcl(drawable.texture!=0, "Try to draw mesh_drawable without texture");
		glUseProgram(drawable.shader); opengl_check;

		// Send uniforms for this shader
		opengl_uniform(drawable.shader, current_scene);
		opengl_uniform(drawable.shader, drawable.shading, false);
		opengl_uniform(drawable.shader, "model", drawable.transform.matrix());

		// Set texture as a cubemap (different from the 2D texture using in the "standard" draw call)
		glActiveTexture(GL_TEXTURE0); opengl_check;
		glBindTexture(GL_TEXTURE_CUBE_MAP, drawable.texture); opengl_check;
		opengl_uniform(drawable.shader, "image_texture", 0);  opengl_check;

		
		// Call draw function
		assert_vcl(drawable.number_triangles>0, "Try to draw mesh_drawable with 0 triangles"); opengl_check;
		glBindVertexArray(drawable.vao);   opengl_check;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.vbo.at("index")); opengl_check;
		glDrawElements(GL_TRIANGLES, GLsizei(drawable.number_triangles*3), GL_UNSIGNED_INT, nullptr); opengl_check;

		// Clean buffers
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}