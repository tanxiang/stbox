#version 450


layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNor;

layout (location = 0) out vec3 outNormal;

void main() {
	//outColor = vec4(inPos,1.0);
	gl_Position = vec4(inPos,1.0);
	//outWorldPos = vec3(gl_Position);
	outNormal = inNor;
}