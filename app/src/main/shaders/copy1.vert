#version 450


layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 in1;

layout (location = 0) out vec4 out0;

void main() {
	//outColor = vec4(inPos,1.0);
	gl_Position = inPos;
	//outWorldPos = vec3(gl_Position);
	outo = in1;
}