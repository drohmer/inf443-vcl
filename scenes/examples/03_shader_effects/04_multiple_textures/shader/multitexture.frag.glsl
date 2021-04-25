#version 330 core

in struct fragment_data
{
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;

	vec3 eye;
} fragment;

layout(location=0) out vec4 FragColor;

uniform sampler2D image_texture;
uniform sampler2D image_texture_2; // the second texture

uniform vec3 light = vec3(1.0, 1.0, 1.0);

uniform vec3 color = vec3(1.0, 1.0, 1.0); 
uniform float alpha = 1.0f;
uniform float Ka = 0.4;
uniform float Kd = 0.8; 
uniform float Ks = 0.4f;
uniform float specular_exp = 64.0;



void main()
{
	vec3 N = normalize(fragment.normal);
	if (gl_FrontFacing == false) {
		N = -N;
	}
	vec3 L = normalize(light-fragment.position);

	float diffuse = max(dot(N,L),0.0);
	float specular = 0.0;
	if(diffuse>0.0){
		vec3 R = reflect(-L,N);
		vec3 V = normalize(fragment.eye-fragment.position);
		specular = pow( max(dot(R,V),0.0), specular_exp );
	}


	vec2 uv_image = vec2(fragment.uv.x, 1.0-fragment.uv.y);
	vec4 color_image_texture_1 = texture(image_texture, uv_image);
	vec4 color_image_texture_2 = texture(image_texture_2, uv_image);

	// Blend the second texture depending on the uv parameter
	float alpha_blend = fragment.uv.y;
	color_image_texture_2 = alpha_blend*vec4(1,1,1,1) + (1-alpha_blend)*color_image_texture_2;
	// Finally multiply the color of the two textures
	vec4 color_image_texture = color_image_texture_1 * color_image_texture_2;

	vec3 color_object  = fragment.color * color * color_image_texture.rgb;
	vec3 color_shading = (Ka + Kd * diffuse) * color_object + Ks * specular * vec3(1.0, 1.0, 1.0);
	



	FragColor = vec4(color_shading, alpha * color_image_texture.a);
}
