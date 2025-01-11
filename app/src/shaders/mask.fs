#version 330 core

// Input from vertex shader
in vec2 TexCoords;

// Output to the framebuffer
out vec4 FragColor;

// Texture sampler
uniform sampler2D texture1;

// Percentage value (0.0 to 1.0) to mask the texture
uniform float progress;

void main()
{
    // Sample the texture
    vec4 texColor = texture(texture1, TexCoords);
    
    // Determine the alpha value based on the progress
    float maskValue = step(TexCoords.x, progress);
    
    // Apply the mask value to the texture alpha
    FragColor = vec4(texColor.rgb, texColor.a * maskValue);
}
