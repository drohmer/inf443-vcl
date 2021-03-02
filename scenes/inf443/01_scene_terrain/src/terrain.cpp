
#include "terrain.hpp"

using namespace vcl;

// Evaluate 3D position of the terrain for any (u,v) \in [0,1]
vec3 evaluate_terrain(float u, float v)
{
    float const x = 20*(u-0.5f);
    float const y = 20*(v-0.5f);

    vec2 const u0 = {0.5f, 0.5f};
    float const h0 = 2.0f;
    float const sigma0 = 0.15f;

    float const d = norm(vec2(u,v)-u0)/sigma0;

    float const z = h0*std::exp(-d*d);

    return {x,y,z};
}

mesh create_terrain()
{
    // Number of samples of the terrain is N x N
    const unsigned int N = 100;

    mesh terrain; // temporary terrain storage (CPU only)
    terrain.position.resize(N*N);

    // Fill terrain geometry
    for(unsigned int ku=0; ku<N; ++ku)
    {
        for(unsigned int kv=0; kv<N; ++kv)
        {
            // Compute local parametric coordinates (u,v) \in [0,1]
            const float u = ku/(N-1.0f);
            const float v = kv/(N-1.0f);

            // Compute the local surface function
            vec3 const p = evaluate_terrain(u,v);

            // Store vertex coordinates
            terrain.position[kv+N*ku] = p;
        }
    }

    // Generate triangle organization
    //  Parametric surface with uniform grid sampling: generate 2 triangles for each grid cell
    for(size_t ku=0; ku<N-1; ++ku)
    {
        for(size_t kv=0; kv<N-1; ++kv)
        {
            const unsigned int idx = kv + N*ku; // current vertex offset

            const uint3 triangle_1 = {idx, idx+1+N, idx+1};
            const uint3 triangle_2 = {idx, idx+N, idx+1+N};

            terrain.connectivity.push_back(triangle_1);
            terrain.connectivity.push_back(triangle_2);
        }
    }

	terrain.fill_empty_field(); // need to call this function to fill the other buffer with default values (normal, color, etc)
    return terrain;
}

