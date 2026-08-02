#version 130
uniform sampler2D currentTexture;
uniform int shaderIndex;
uniform float u_time;

void main()
{
	vec2 coord = gl_TexCoord[0].xy;   
	vec4 pixel_color = texture2D(currentTexture, coord);
	float c = pixel_color[0];

	if (c < 0.5)
	{
		gl_FragColor = vec4(0.0, 0.0, 1.0 - 2*c, 1.0);
	}
	else if (abs(c-0.5) < 0.01)
	{
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else
	{
		gl_FragColor = vec4(2*(c-0.5), 0.0, 0.0, 1.0);
	}
}
