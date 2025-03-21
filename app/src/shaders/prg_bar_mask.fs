#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;

out vec4 finalColor;

uniform float progress = 0.0;

void main()
{
  vec4 source = texture(texture0, fragTexCoord);
  
  if(fragTexCoord.x < progress) {
      finalColor = source;
  }
  else {
      finalColor = vec4(0.0, 0.0, 0.0, 0.0);
  }
}
