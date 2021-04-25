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

uniform vec3 light = vec3(1.0, 1.0, 1.0);

uniform vec3 color = vec3(1.0, 1.0, 1.0); // Unifor color of the object
uniform float alpha = 1.0f; // alpha coefficient
uniform float Ka = 0.4; // Ambient coefficient
uniform float Kd = 0.8; // Diffuse coefficient
uniform float Ks = 0.4f;// Specular coefficient
uniform float specular_exp = 64.0; // Specular exponent
uniform bool use_texture = true;
uniform bool texture_inverse_y = false;

uniform vec3 spotlight_color[5];
uniform vec3 spotlight_position[5];
uniform float spotlight_falloff;
uniform float fog_falloff;

void main()
{
	vec3 N = normalize(fragment.normal);
	if (gl_FrontFacing == false) {
		N = -N;
	}
	vec2 uv_image = vec2(fragment.uv.x, 1.0-fragment.uv.y);
	if(texture_inverse_y) {
		uv_image.y = 1.0-uv_image.y;
	}
	vec4 color_image_texture = texture(image_texture, uv_image);
	if(use_texture==false) {
		color_image_texture=vec4(1.0,1.0,1.0,1.0);
	}

	vec3 color_object  = fragment.color * color * color_image_texture.rgb;
	vec3 color_shading = Ka * color_object;

	for(int k_light=0; k_light<5; k_light++)
	{
		vec3 v = spotlight_position[k_light]-fragment.position;
		float dist = length(v);
		vec3 L = normalize(v);
		float diffuse = max(dot(N,L),0.0);
		float specular = 0.0;
		if(diffuse>0.0){
			vec3 R = reflect(-L,N);
			vec3 V = normalize(fragment.eye-fragment.position);
			specular = pow( max(dot(R,V),0.0), specular_exp );
		}
		
		// spotlight color
		color_shading += (Kd*diffuse*color_object + Ks * specular)*spotlight_color[k_light]*exp(-spotlight_falloff*dist*dist);
	}

    //fog effect
	float depth = length(fragment.eye-fragment.position);
	float w_depth = exp(-fog_falloff*depth*depth);
	vec3 color_with_fog = w_depth*color_shading+(1-w_depth)*vec3(0.7,0.7,0.7);

	FragColor = vec4( color_with_fog, alpha * color_image_texture.a);
}
