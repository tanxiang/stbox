#version 450

layout (triangles) in;

layout (binding = 0) uniform UBO
{
    mat4 MVP;
    mat4 model;
} ubo;
layout (location = 0) in vec3 inNormal[];

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;

void main(void)
{
    gl_Position = ubo.MVP * (
    	gl_TessCoord[0] * gl_in[0].gl_Position +
    	gl_TessCoord[1] * gl_in[1].gl_Position +
		gl_TessCoord[2] * gl_in[2].gl_Position
    );
	outWorldPos = vec3(
		ubo.model * (
			gl_TessCoord[0] * gl_in[0].gl_Position +
			gl_TessCoord[1] * gl_in[1].gl_Position +
			gl_TessCoord[2] * gl_in[2].gl_Position
		)
	);
    outNormal = vec3(
		ubo.model * vec4(
    		gl_TessCoord[0] * inNormal[0] +
    		gl_TessCoord[1] * inNormal[1] +
    		gl_TessCoord[2] * inNormal[2],0
		)
	);
    //outUV = gl_TessCoord[0]*inUV[0] + gl_TessCoord[1]*inUV[1] + gl_TessCoord[2]*inUV[2];
}