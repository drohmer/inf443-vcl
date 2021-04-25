#version 330 core

in struct fragment_data
{
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
	vec4 fraglight;
	vec3 eye;
} fragment;

layout(location=0) out vec4 FragColor;

uniform sampler2D image_texture;
uniform sampler2D shadow_texture;

uniform mat4 light;

uniform vec3 color = vec3(1.0, 1.0, 1.0); 
uniform float alpha = 1.0f;
uniform float Ka = 0.4;
uniform float Kd = 0.8; 
uniform float Ks = 0.4f;
uniform float specular_exp = 64.0;


float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz/fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float currentDepth = projCoords.z;

    float bias = 0.005;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadow_texture, 0);
    for(int x = -2; x <= 2; ++x)
    {
        for(int y = -2; y <= 2; ++y)
        {
            float closestDepth = texture(shadow_texture, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth-bias > closestDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;

    if(projCoords.z > 1.0)
       shadow = 0.0;

    return shadow;
}


void main()
{
	vec3 N = normalize(fragment.normal);
	if (gl_FrontFacing == false) {
		N = -N;
	}

    vec3 light_position = vec3(inverse(light)*vec4(0,0,0,1.0));
	vec3 L = normalize(light_position-fragment.position);

	float diffuse = max(dot(N,L),0.0);
	float specular = 0.0;
	if(diffuse>0.0){
		vec3 R = reflect(-L,N);
		vec3 V = normalize(fragment.eye-fragment.position);
		specular = pow( max(dot(R,V),0.0), specular_exp );
	}

	float shadow = ShadowCalculation(fragment.fraglight);

	vec4 color_image_texture = texture(image_texture, vec2(fragment.uv.x,1.0-fragment.uv.y) );

	vec3 color_object  = fragment.color * color * color_image_texture.rgb;
	vec3 color_shading = (Ka + Kd * (1.0-shadow) * diffuse) * color_object + (1.0-shadow) * Ks * specular * vec3(1.0, 1.0, 1.0);



	FragColor = vec4(color_shading, alpha * color_image_texture.a);
}
