#version 450
#extension GL_ARB_separate_shader_objects : enable

struct GlyphInfo
{
    vec4 bbox;
    uvec4 cell_info;
};

layout (set = 0, binding = 0) buffer GlyphBuffer
{
	GlyphInfo glyphs[];
} glyph_buffer;

layout(location = 0) in vec4 in_rect;
layout(location = 1) in uint in_glyph_index;
layout(location = 2) in float in_sharpness;

layout(location = 0) out vec2 out_glyph_pos;
layout(location = 1) out uvec4 out_cell_info;
layout(location = 2) out float out_sharpness;
layout(location = 3) out vec2 out_cell_coord;

vec2 pos[4] = vec2[](
    vec2(-1, -1),
    vec2(-0.6, -1),
    vec2(-1, -0.75),
    vec2(-0.6, -0.75)
);

void main()
{
    GlyphInfo gi = glyph_buffer.glyphs[in_glyph_index];


    vec2 glyph_pos[4];
    glyph_pos[0] = vec2(gi.bbox.x, gi.bbox.y);
    glyph_pos[1] = vec2(gi.bbox.z, gi.bbox.y);
    glyph_pos[2] = vec2(gi.bbox.x, gi.bbox.w);
    glyph_pos[3] = vec2(gi.bbox.z, gi.bbox.w);

    vec2 cell_coord[4];
    cell_coord[0] = vec2(0,              0);
    cell_coord[1] = vec2(gi.cell_info.z, 0);
    cell_coord[2] = vec2(0,              gi.cell_info.w);
    cell_coord[3] = vec2(gi.cell_info.z, gi.cell_info.w);

        gl_Position = vec4(pos[gl_VertexIndex].x + gl_InstanceIndex / 5.0,pos[gl_VertexIndex].y + in_rect.x /5.0 , 0.0, 1.0);
    out_glyph_pos = glyph_pos[gl_VertexIndex];
    out_cell_info = gi.cell_info;
    out_sharpness = in_sharpness;
    out_cell_coord = cell_coord[gl_VertexIndex];
}
