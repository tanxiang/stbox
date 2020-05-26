#version 450

layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform PushConsts {
	layout(offset = 0) float r;
	layout(offset = 4) float g;
	layout(offset = 8) float b;
	layout(offset = 12) float kr;
	layout(offset = 16) float kg;
	layout(offset = 20) float kb;
} material;
void main()
{
	outFragColor = vec4(material.r,material.g,material.b,0.98);
	//outFragColor = vec4(inUV,1.0,0.5);
}