#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float process;

out vec4 out_color;

void main(){
    float lines = process;  //step(process, fract(fragTexCoord.x * 10.0));

    out_color = mix(
      vec4(0, 0, 0, 1),
      vec4(0, 0, 0, 0),
      lines
    );
}

