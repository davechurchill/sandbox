#version 130

uniform sampler2D currentTexture;
uniform int numberOfContourLines;
uniform vec3 lineColor;
uniform float lineOpacity;

void main()
{
    vec2 coordinate = gl_TexCoord[0].xy;
    float height = texture2D(currentTexture, coordinate).r;
    gl_FragColor = vec4(0.0);

    if (height < 0.02 || height > 0.99 || numberOfContourLines <= 0)
    {
        return;
    }

    vec2 texelSize = 1.0 / vec2(textureSize(currentTexture, 0));
    float leftHeight = texture2D(currentTexture, coordinate - vec2(texelSize.x, 0.0)).r;
    float rightHeight = texture2D(currentTexture, coordinate + vec2(texelSize.x, 0.0)).r;
    float upHeight = texture2D(currentTexture, coordinate - vec2(0.0, texelSize.y)).r;
    float downHeight = texture2D(currentTexture, coordinate + vec2(0.0, texelSize.y)).r;

    float contourStep = 1.0 / (float(numberOfContourLines) + 1.0);
    for (int line = 1; line <= numberOfContourLines; ++line)
    {
        float contourHeight = contourStep * float(line);
        if (height > contourHeight
            && (leftHeight <= contourHeight || rightHeight <= contourHeight
                || upHeight <= contourHeight || downHeight <= contourHeight))
        {
            gl_FragColor = vec4(lineColor, lineOpacity);
            return;
        }
    }
}
