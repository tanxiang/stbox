#pragma once
struct Vertex {
    float posX, posY, posZ, posW;  // Position data
    float r, g, b, a;              // Color
};

struct VertexUV {
    float pos[4];  // Position data
    float uv[2];                    // texture u,v
};

