#version 410 core

layout(location = 0) in vec3 vPosition;

uniform mat4 model;
uniform mat4 lightSpaceTrMatrix;

void main()
{
    gl_Position = lightSpaceTrMatrix * model * vec4(vPosition, 1.0);
      // gl_Position = vec4(vPosition, 1.0);

}
