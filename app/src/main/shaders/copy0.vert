#version 450


layout (location = 0) in vec4 inPos;
//layout (location = 1) in vec4 inColor;

//layout (location = 0) out vec4 outColor;

void main() {
	//outColor = vec4(inPos,1.0);
	gl_Position = inPos;
	//outWorldPos = vec3(gl_Position);
	//outColor = inColor;
}