#version 120
uniform sampler2D currentTexture;
uniform float time;

void main() {
    vec2 coord = gl_TexCoord[0].xy;   
    vec4 pixel_color = texture2D(currentTexture, coord);
    gl_FragColor = vec4( 1.0, pixel_color[1], pixel_color[2], pixel_color[3]);
}
