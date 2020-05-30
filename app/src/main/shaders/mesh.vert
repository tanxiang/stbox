#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform bufferVals {
	mat4 mvp;
	mat4 model;
} myBufferVals;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNor;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
//layout (location = 0) out vec4 outColor;

void main() {
	//outColor = vec4(inPos,1.0);
	gl_Position = myBufferVals.mvp * vec4(inPos,1.0);
	outWorldPos = vec3(gl_Position);
	outNormal = vec3(myBufferVals.mvp * vec4(inNor,0.0));
}