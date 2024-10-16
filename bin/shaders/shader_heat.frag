#version 130
uniform sampler2D currentTexture;
uniform int shaderIndex;
uniform bool contour;
uniform int numberOfContourLines;
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
	
	
	if (contour)
	{
		ivec2 textureSize = textureSize(currentTexture, 0);

		// Calculate the texel size
		vec2 texelSize = 1.0 / textureSize;

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
