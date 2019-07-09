#version 120
// Input vertex attributes
attribute vec3 vertexPosition;

// Input uniform values
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

void main(void) {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
    //gl_Position = vec4(vertexPosition, 1.0);
    gl_PointSize = 1.0;
}

