#version 330
in vec3 position;
uniform mat4 transform;
out vec2 uvCoord;

void main() {
    gl_Position = transform * vec4( position, 1.0 );
    uvCoord = (position.xy + vec2(1,1))/2.0;
}