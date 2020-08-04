#version 450


layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = inColor;
    //outFragColor = vec4(inUV,1.0,0.5);
}