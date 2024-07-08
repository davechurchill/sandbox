
#version 120
uniform sampler2D currentTexture;
uniform float time;

void main()
{
    vec2 coord = gl_TexCoord[0].xy;

    float factorx=0.02*( 1 + cos(5.0*time));
    float factory=0.02*( 1 - cos(5.0*time));
    coord.x = (coord.x+factorx)/(1.0+2.0*factorx);
    coord.y = (coord.y+factory)/(1.0+2.0*factory);

    vec4 pixel_color = texture2D(currentTexture, coord);

    gl_FragColor = pixel_color;
}
