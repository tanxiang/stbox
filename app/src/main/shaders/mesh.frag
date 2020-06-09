#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;


layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform Material {
	layout(offset = 0) vec3 ambient;
	layout(offset = 16) vec3 diffuse;
	layout(offset = 32) vec3 specular;
	layout(offset = 48) vec3 lightcolor;
} material;


void main()
{
	vec3 ambient = material.lightcolor * material.ambient;
	vec3 lightDir = normalize(vec3(3,15,-5) - inWorldPos);
	vec3 norm = normalize(inNormal);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = material.lightcolor * (diff * material.diffuse);

	vec3 viewDir = normalize(vec3(0,0,-4) - inWorldPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 0.5);
	vec3 specular = material.lightcolor * (spec * material.specular);

	vec3 result = ambient + diffuse + specular;
	outFragColor = vec4(result,1.0);
	//outFragColor = vec4(inUV,1.0,0.5);
}