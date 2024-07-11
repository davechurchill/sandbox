#version 130
uniform sampler2D currentTexture;
uniform int shaderIndex;
uniform bool contour;
uniform int numberOfContourLines;

void popsicle(float c);
void red(vec4 pixel_color);
void terrain(float c);

void main() {

	vec2 coord = gl_TexCoord[0].xy;   
	vec4 pixel_color = texture2D(currentTexture, coord);
	float c = pixel_color[0];

	if(shaderIndex == 0)
	{
		if (c < 0.02 || c > 0.99) 
		{
			gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0);
			return;
		}
		popsicle(c);
	}

	if(shaderIndex == 1)
	{
		red(pixel_color);
	}

	if(shaderIndex == 2)
	{
		terrain(c);
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

		else 
		{
			if(shaderIndex == 0)
			{
				popsicle(c);
			}

			else if(shaderIndex == 1)
			{
				red(pixel_color);
			}

			else if(shaderIndex == 2)
			{
				terrain(c);
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

void red(vec4 pixel_color)
{
	gl_FragColor = vec4( 1.0, pixel_color[1], pixel_color[2], pixel_color[3]);
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
		// Blue color for lower depths (water)
		gl_FragColor = vec4( 0.0, 0.0, 3.3*c-0.1, 1.0); 
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
		gl_FragColor = vec4( 0.9-c/1.5, 0.6-c/1.5, 0.0, 1.0); 
	}

}