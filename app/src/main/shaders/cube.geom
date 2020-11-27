#version 450


layout(lines_adjacency)in;
layout(triangle_strip,max_vertices=14) out;

layout (binding = 0) uniform UBO
{
    mat4 MVP;
} ubo;

//layout (location = 0) in vec4 inColor[];

layout (location = 0) out vec4 outColor;

vec3 qtransform( vec4 q, vec3 v ){
    return v + 2.0*cross(q.xyz,cross(q.xyz,v) + q.w*v);
}

void main() {
    vec4 pos = gl_in[0].gl_Position;
    vec4 quat = gl_in[1].gl_Position;
    vec3 min = gl_in[2].gl_Position.xyz;
    vec3 max = gl_in[3].gl_Position.xyz;

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,vec3(min.x,max.y,min.z)),1.0));
    outColor = vec4(0.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,vec3(max.xy,min.z)),1.0));
    outColor = vec4(1.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position =  ubo.MVP*(pos + vec4(qtransform(quat,min),1.0));
    outColor = vec4(0.0, 0.0, 0.0,1.0);
    EmitVertex();

    gl_Position =  ubo.MVP*(pos + vec4(qtransform(quat,vec3(max.x,min.yz)),1.0));//ubo.MVP*vec4(max.x,min.yzw);
    outColor = vec4(1.0, 0.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,vec3(max.x,min.y,max.z)),1.0));//ubo.MVP*vec4(max.x,min.y,max.zw);
    outColor = vec4(1.0, 0.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,vec3(max.xy,min.z)),1.0));//ubo.MVP*vec4(max.xy,min.zw);
    outColor = vec4(1.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,max),1.0));//ubo.MVP*max;
    outColor = vec4(1.0, 1.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,vec3(min.x,max.y,min.z)),1.0));//ubo.MVP*vec4(min.x,max.y,min.zw);
    outColor = vec4(0.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,vec3(min.x,max.yz)),1.0));//ubo.MVP*vec4(min.x,max.yzw);
    outColor = vec4(0.0, 1.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,min),1.0));//ubo.MVP*min;
    outColor = vec4(0.0, 0.0, 0.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,vec3(min.xy,max.z)),1.0));//ubo.MVP*vec4(min.xy,max.zw);
    outColor = vec4(0.0, 0.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,vec3(max.x,min.y,max.z)),1.0));//ubo.MVP*vec4(max.x,min.y,max.zw);
    outColor = vec4(1.0, 0.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,vec3(min.x,max.yz)),1.0));//ubo.MVP*vec4(min.x,max.yzw);
    outColor = vec4(0.0, 1.0, 1.0,1.0);
    EmitVertex();

    gl_Position = ubo.MVP*(pos + vec4(qtransform(quat,max),1.0));//ubo.MVP*max;
    outColor = vec4(1.0, 1.0, 1.0,1.0);
    EmitVertex();

    EndPrimitive();
}
