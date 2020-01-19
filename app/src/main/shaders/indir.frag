#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
//layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = inColor;
    //outFragColor = vec4(inUV,1.0,0.5);
}