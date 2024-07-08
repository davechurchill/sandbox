#version 120
uniform sampler2D currentTexture;

void popsicle(float c);

void main() {
    vec2 coord = gl_TexCoord[0].xy;   
    vec4 pixel_color = texture2D(currentTexture, coord);
	float c = pixel_color[0];

	if (c < 0.02 || c > 0.99) {
	    gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0);
		return;
	}

	popsicle(c);
}

void popsicle(float c) {
	int dNormal = int((1.0 - c) * 1529.0);

	int i = int(dNormal / 255);
	int pR, pG, pB;
	switch (i)
        {
        case 0: { pR = 255; pG = dNormal; pB = dNormal; } break;
        case 1: { pR = 510 - dNormal; pG = 255; pB = 510 - dNormal; } break;
        case 2: { pR = dNormal - 510; pG = 765 - dNormal; pB = dNormal - 510; } break;
        case 3: { pR = 1020 - dNormal; pG = dNormal - 765; pB = 255; } break;
        case 4: { pR = 0; pG = 1275 - dNormal; pB = 255; } break;
        case 5: { pR = 0; pG = 0; pB = 1529 - dNormal; } break;
		default: { pR = 0; pG = 0; pB = 0; } break;
        }
	gl_FragColor = vec4( float(pR) / 255.0, float(pG) / 255.0, float(pB) / 255.0, 1.0);
}