#version 330 core

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Uniform matrices
uniform mat4 mvp;

// Output to fragment shader
out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
    // Pass texture coordinates to fragment shader
    fragTexCoord = vertexTexCoord;
    
    // Pass vertex color to fragment shader
    fragColor = vertexColor;
    
    // Transform vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
