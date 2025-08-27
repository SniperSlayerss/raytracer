#version 330 core

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Uniform variables
uniform float time;
uniform vec2 resolution;

// Output color
out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;
    
    finalColor = vec4(uv.x, uv.y, 0, 1.0);
}}