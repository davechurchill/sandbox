#version 130
uniform sampler2D currentTexture;
uniform int shaderIndex;
uniform bool contour;
uniform int numberOfContourLines;
uniform float u_time;


void popsicle(float c);
void red(float c);
void terrain(float c);
void animate(float b);

void main() {

	vec2 coord = gl_TexCoord[0].xy;   
	vec4 pixel_color = texture2D(currentTexture, coord);
	float c = pixel_color[0];

	if (c < 0.02 || c > 0.99) 
	{
		gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0);
		return;
	}

	switch (shaderIndex)
	{
		case 0: popsicle(c); break;
		case 1: red(c); break;
		case 2: terrain(c); break;
		default: gl_FragColor = pixel_color; break;
	}
	
	if(contour)
	{
		ivec2 textureSize = textureSize(currentTexture, 0);

		// Calculate the texel size
		vec2 texelSize = 1.0 / textureSize;

		// Get the coordinates of the current cell
		//vec2 coord = gl_TexCoord[0].xy;

		// Get the coordinates of the adjacent cells
		vec2 coord_left = coord + vec2(-texelSize.x, 0.0);
		vec2 coord_right = coord + vec2(texelSize.x, 0.0);
		vec2 coord_up = coord + vec2(0.0, -texelSize.y);
		vec2 coord_down = coord + vec2(0.0, texelSize.y);

		//get the pixel color
		vec4 current_pixel_color = texture2D(currentTexture, coord);
		float left_pixel_color = texture2D(currentTexture, coord_left)[0];
		float right_pixel_color = texture2D(currentTexture, coord_right)[0];
		float up_pixel_color = texture2D(currentTexture, coord_up)[0];
		float down_pixel_color = texture2D(currentTexture, coord_down)[0];

		//draw contour lines based on the number of contours selected
		float step = 1.0 / (float(numberOfContourLines) + 1.0);
		for(float contourLevel = 0.0; contourLevel <= 1.0; contourLevel+= step)
		{
			if(current_pixel_color[0]> contourLevel)
			{
				if(left_pixel_color <= contourLevel || right_pixel_color <= contourLevel || 
				up_pixel_color <= contourLevel   || down_pixel_color <= contourLevel) 
				{
					gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
					break;
				}
			}
		}
	} 
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

void red(float c)
{
	gl_FragColor = vec4(c, c, 1.0, 1.0 );
}


void animate(float b)
{
			float time = u_time * .5+23.0;

		// uv should be the 0-1 uv of texture...
		vec2 uv = gl_TexCoord[0].xy;
		
		vec2 p = mod(uv*6.28318530718, 6.28318530718)-250.0;

		vec2 i = vec2(p);
		float c = 1.0;
		float inten = .005;

		for (int n = 0; n < 10; n++) 
		{
			float t = time * (1.0 - (3.5 / float(n+1)));
			i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
			c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
		}
		c /= float(7);
		c = 1.17-pow(c, 1.4);
		vec3 colour = vec3(pow(abs(c), 8.0));
		colour = clamp(colour + vec3(0.0, 0.35, 0.5), 0.0, 1.0);
		
		gl_FragColor = vec4(colour, 1.0); 

}
void terrain(float c)
{
	if (c < 0.02 || c > 0.99) 
	{
		// eliminate extremely low or high values (these are thresholded or error values)
		gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0); 
	} 
	else if (c < 0.3) 
	{
		animate(0.35);
	}

	else if (c < 0.5) 
	{
		// green (surface level)
		gl_FragColor = vec4( 0.0, 1-c, 0.0, 1.0); 
	} 
	else if (c < 0.7) 
	{
		// Transition from green to mountain color
		gl_FragColor = vec4( (c - 0.5) * 5.0, 0.5 + (c - 0.5) * 0.5, 0.0, 1.0); 
	} 
	
	else 
	{
		// Mountain color for higher elevations
        gl_FragColor = vec4( c+0.2, c*c, c*c*c*c*c, 1.0); 
	}
}