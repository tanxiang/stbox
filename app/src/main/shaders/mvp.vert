#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform bufferVals {
    mat4 mvp;
} myBufferVals;
layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 0) out vec2 outUV;

void main() {
    outUV = inUV;
    gl_Position = myBufferVals.mvp * inPos;
}