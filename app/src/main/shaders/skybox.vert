#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform bufferVals {
	mat4 mvp;
} myBufferVals;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = inPos;
	outUVW.y *= -1.0;
	outUVW.x *= -1.0;
	gl_Position = myBufferVals.mvp * vec4(inPos.xyz, 1.0);
}
