#version 130

uniform sampler2D currentTexture;
uniform int numberOfContourLines;
uniform vec3 lineColor;
uniform float lineOpacity;

void main()
{
    vec2 coordinate = gl_TexCoord[0].xy;
    float height = texture2D(currentTexture, coordinate).r;

    if (height < 0.02 || height > 0.99 || numberOfContourLines <= 0)
    {
        discard;
    }

    vec2 texelSize = 1.0 / vec2(textureSize(currentTexture, 0));
    float leftHeight = texture2D(currentTexture, coordinate - vec2(texelSize.x, 0.0)).r;
    float rightHeight = texture2D(currentTexture, coordinate + vec2(texelSize.x, 0.0)).r;
    float upHeight = texture2D(currentTexture, coordinate - vec2(0.0, texelSize.y)).r;
    float downHeight = texture2D(currentTexture, coordinate + vec2(0.0, texelSize.y)).r;

    // The original loop drew on the high side of a contour whenever any
    // threshold satisfied neighborHeight <= threshold < height. Find that
    // integer threshold interval directly instead of testing every line.
    float contourScale = float(numberOfContourLines) + 1.0;
    float lowestNeighbor = min(min(leftHeight, rightHeight), min(upHeight, downHeight));
    float firstPossibleLine = max(1.0, ceil(lowestNeighbor * contourScale));
    float lastPossibleLine = min(float(numberOfContourLines), ceil(height * contourScale) - 1.0);

    if (firstPossibleLine > lastPossibleLine)
    {
        discard;
    }

    gl_FragColor = vec4(lineColor, lineOpacity);
}
