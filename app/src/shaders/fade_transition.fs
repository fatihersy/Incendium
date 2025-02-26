#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

uniform float process;

out vec4 out_color;

void main(){
    float lines = step(mod(process, 1.0), fract(fragTexCoord.x * 10.0));

    out_color = mix(
      vec4(1),
      vec4(0),
      lines
    );
}

