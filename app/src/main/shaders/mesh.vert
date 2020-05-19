#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform bufferVals {
	mat4 mvp;
} myBufferVals;

layout (location = 0) in vec3 inPos;
//layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(inPos,1.0);
	gl_Position = myBufferVals.mvp * vec4(inPos,1.0);
}