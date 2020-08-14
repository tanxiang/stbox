#version 450


layout(triangles)in;
layout(triangle_strip,max_vertices=14) out;

layout (binding = 0) uniform UBO
{
    mat4 MVP;
} ubo;
//layout (location = 0) in vec4 inColor[];

layout (location = 0) out vec4 outColor;

vec3 qtransform( vec4 q, vec3 v ){
    return v + 2.0*cross(cross(v, -q.xyz ) + q.w*v, -q.xyz);
}

void main() {
    vec4 min = gl_in[0].gl_Position;
    vec4 max = gl_in[1].gl_Position;
    vec4 quat = gl_in[2].gl_Position;

    gl_Position = ubo.MVP*vec4(min.x,max.y,min.zw);
    outColor = vec4(0.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*vec4(max.xy,min.zw);
    outColor = vec4(1.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*min;
    outColor = vec4(0.0, 0.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*vec4(max.x,min.yzw);
    outColor = vec4(1.0, 0.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*vec4(max.x,min.y,max.zw);
    outColor = vec4(1.0, 0.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*vec4(max.xy,min.zw);
    outColor = vec4(1.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*max;
    outColor = vec4(1.0, 1.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*vec4(min.x,max.y,min.zw);
    outColor = vec4(0.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*vec4(min.x,max.yzw);
    outColor = vec4(0.0, 1.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*min;
    outColor = vec4(0.0, 0.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*vec4(min.xy,max.zw);
    outColor = vec4(0.0, 0.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*vec4(max.x,min.y,max.zw);
    outColor = vec4(1.0, 0.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*vec4(min.x,max.yzw);
    outColor = vec4(0.0, 1.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*max;
    outColor = vec4(1.0, 1.0, 1.0,1.0);
    EmitVertex();

    EndPrimitive();
}
