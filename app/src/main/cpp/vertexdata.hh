#ifndef STBOX_CUBE_DATA_HH
#define STBOX_CUBE_DATA_HH
struct Vertex {
    float posX, posY, posZ, posW;  // Position data
    float r, g, b, a;              // Color
};

struct VertexUV {
    float pos[4];  // Position data
    float uv[2];                    // texture u,v
};

#endif //STBOX_CUBE_DATA_HH
