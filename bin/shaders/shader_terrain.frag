#version 120
uniform sampler2D currentTexture;
uniform float time;

void main() {
    vec2 coord = gl_TexCoord[0].xy;   
    vec4 pixel_color = texture2D(currentTexture, coord);
	
	float waterLevel = 0.8;
	float c = pixel_color[0];
	
	if (c < 0.02 || c > 0.99) {
	    gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0);
	} else if (c < 0.2) {
		gl_FragColor = vec4( 1.0-5*c, 1.0, 1.0-5*c, 1.0);
	} else if (c < 0.6) {
	    gl_FragColor = vec4( 0.0, 1-c, 0.0, 1.0);
	} else {
		gl_FragColor = vec4( 0, 0, c, 1.0);
	}
}
