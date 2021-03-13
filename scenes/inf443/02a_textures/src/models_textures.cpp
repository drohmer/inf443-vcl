
#include "models_textures.hpp"

using namespace vcl;

mesh tore_with_texture()
{
    float const a = 1.7f;
    float const b = 0.6f;

    // Number of samples of the terrain is N x N
    const unsigned int N = 50;

    mesh tore; // temporary terrain storage (CPU only)
    tore.position.resize(N*N);
    tore.uv.resize(N*N);

    // Fill terrain geometry
    for(unsigned int ku=0; ku<N; ++ku)
    {
        for(unsigned int kv=0; kv<N; ++kv)
        {
            // Compute local parametric coordinates (u,v) \in [0,1]
            const float u = ku/(N-1.0f);
            const float v = kv/(N-1.0f);

            // Compute the local surface function

            vec3 const p = {
					(a + b*std::cos(2*pi*u))*std::cos(2*pi*v),
					(a + b*std::cos(2*pi*u))*std::sin(2*pi*v),
					b*std::sin(2*pi*u)};
            

            // Store vertex coordinates
            tore.position[kv+N*ku] = p;
            // tore.uv[kv+N*ku] = {...,...};
        }
    }


    // Generate triangle organization
    for(size_t ku=0; ku<N-1; ++ku)
    {
        for(size_t kv=0; kv<N-1; ++kv)
        {
            const unsigned int idx = kv + N*ku;

            const uint3 triangle_1 = {idx, idx+1+N, idx+1};
            const uint3 triangle_2 = {idx, idx+N, idx+1+N};

            tore.connectivity.push_back(triangle_1);
            tore.connectivity.push_back(triangle_2);
        }
    }

	tore.fill_empty_field();
    return tore;
}


mesh cylinder_with_texture()
{
    float const r = 1.0f;
    float const h = 4.0f;

    // Number of samples of the terrain is N x N
    const unsigned int N = 20;

    mesh cylinder; // temporary terrain storage (CPU only)
    cylinder.position.resize(N*N);
    cylinder.uv.resize(N*N);

    // Fill terrain geometry
    for(unsigned int ku=0; ku<N; ++ku)
    {
        for(unsigned int kv=0; kv<N; ++kv)
        {
            // Compute local parametric coordinates (u,v) \in [0,1]
            const float u = ku/(N-1.0f);
            const float v = kv/(N-1.0f);

            // Compute the local surface function
            vec3 const p = {r*std::cos(2*pi*u), r*std::sin(2*pi*u), h*(v-0.5f)};
            vec2 const uv = {u,v};

            // Store vertex coordinates
            cylinder.position[kv+N*ku] = p;
            // cylinder.uv[kv+N*ku] = ...
        }
    }

    // Generate triangle organization
    for(size_t ku=0; ku<N-1; ++ku)
    {
        for(size_t kv=0; kv<N-1; ++kv)
        {
            const unsigned int idx = kv + N*ku;

            const uint3 triangle_1 = {idx, idx+1+N, idx+1};
            const uint3 triangle_2 = {idx, idx+N, idx+1+N};

            cylinder.connectivity.push_back(triangle_1);
            cylinder.connectivity.push_back(triangle_2);
        }
    }

	cylinder.fill_empty_field();
    return cylinder;
}


mesh disc_with_texture()
{
    float const r = 1.0f;

	mesh disc;
    size_t N = 20;

	for (size_t k = 0; k < size_t(N); ++k)
	{
		float const u = k/(N-1.0f);
		vec3 const p = r * vec3(std::cos(2*pi*u), std::sin(2*pi*u), 0.0f);
		disc.position.push_back(p);
        // disc.uv.push_back(...)
		
	}
	// middle point
    disc.position.push_back({0,0,0});
    // disc.uv.push_back(...)

	for (size_t k = 0; k < size_t(N-1); ++k)
		disc.connectivity.push_back( uint3{ unsigned(N), unsigned(k), unsigned(k+1)});

	disc.fill_empty_field();
    return disc;
}



