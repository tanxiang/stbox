#version 450

layout (location = 0) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
	vec3 lightpos;
	vec3 lightcolor;
} material;


void main()
{
	outFragColor = vec4(material.ambient,0.98);
	//outFragColor = vec4(inUV,1.0,0.5);
}