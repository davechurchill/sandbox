#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float hexagonSize;
uniform float heightSteps;
uniform float prismRelief;

const float SqrtThree = 1.73205080757;
const float HalfSqrtThree = 0.86602540378;
const float WaterLevel = 0.28;

float Hash(vec2 point)
{
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453);
}

bool IsValidHeight(float height)
{
    return height > 0.001 && height < 0.999;
}

float ReadPixelHeight(vec2 pixel, out bool valid)
{
    vec2 textureDimensions = 1.0 / texelSize;
    valid = pixel.x >= 0.0 && pixel.y >= 0.0
        && pixel.x < textureDimensions.x && pixel.y < textureDimensions.y;
    if (!valid)
    {
        return 0.0;
    }

    float height = texture2D(currentTexture, (pixel + vec2(0.5)) * texelSize).r;
    valid = IsValidHeight(height);
    return height;
}

vec2 RoundAxial(vec2 axial)
{
    float cubeX = axial.x;
    float cubeZ = axial.y;
    float cubeY = -cubeX - cubeZ;
    float roundedX = floor(cubeX + 0.5);
    float roundedY = floor(cubeY + 0.5);
    float roundedZ = floor(cubeZ + 0.5);
    float differenceX = abs(roundedX - cubeX);
    float differenceY = abs(roundedY - cubeY);
    float differenceZ = abs(roundedZ - cubeZ);

    if (differenceX > differenceY && differenceX > differenceZ)
    {
        roundedX = -roundedY - roundedZ;
    }
    else if (differenceY > differenceZ)
    {
        roundedY = -roundedX - roundedZ;
    }
    else
    {
        roundedZ = -roundedX - roundedY;
    }
    return vec2(roundedX, roundedZ);
}

vec2 HexagonId(vec2 pixel)
{
    float radius = max(hexagonSize, 1.0);
    vec2 origin = vec2(HalfSqrtThree * radius, radius);
    vec2 point = pixel - origin;
    vec2 axial = vec2(
        (point.x / SqrtThree - point.y / 3.0) / radius,
        (2.0 * point.y / 3.0) / radius);
    return RoundAxial(axial);
}

vec2 HexagonCenter(vec2 id)
{
    float radius = max(hexagonSize, 1.0);
    vec2 origin = vec2(HalfSqrtThree * radius, radius);
    return origin + vec2(
        SqrtThree * radius * (id.x + id.y * 0.5),
        1.5 * radius * id.y);
}

float SampleHexagonHeight(vec2 id, out bool valid)
{
    float radius = max(hexagonSize, 1.0);
    vec2 center = HexagonCenter(id);
    float height = ReadPixelHeight(center, valid);
    if (valid) { return height; }

    float probeRadius = radius * 0.52;
    height = ReadPixelHeight(center + vec2(0.0, -probeRadius), valid);
    if (valid) { return height; }
    height = ReadPixelHeight(center + vec2(HalfSqrtThree, -0.5) * probeRadius, valid);
    if (valid) { return height; }
    height = ReadPixelHeight(center + vec2(HalfSqrtThree, 0.5) * probeRadius, valid);
    if (valid) { return height; }
    height = ReadPixelHeight(center + vec2(0.0, probeRadius), valid);
    if (valid) { return height; }
    height = ReadPixelHeight(center + vec2(-HalfSqrtThree, 0.5) * probeRadius, valid);
    if (valid) { return height; }
    return ReadPixelHeight(center + vec2(-HalfSqrtThree, -0.5) * probeRadius, valid);
}

float QuantizeHeight(float height)
{
    float levels = max(heightSteps, 1.0);
    return floor(height * levels + 0.5) / levels;
}

float NeighborHeight(vec2 id, float fallback)
{
    bool valid;
    float height = SampleHexagonHeight(id, valid);
    return valid ? QuantizeHeight(height) : fallback;
}

float HexagonMetric(vec2 localPosition)
{
    float radius = max(hexagonSize, 1.0);
    vec2 point = abs(localPosition);
    float verticalSide = point.x / max(HalfSqrtThree * radius, 0.0001);
    float diagonalSide = (point.y + point.x / SqrtThree) / radius;
    return max(verticalSide, diagonalSide);
}

float SideProgress(float edgeDistance, float dropInLevels)
{
    float radius = max(hexagonSize, 1.0);
    float width = min(radius * 0.44, dropInLevels * prismRelief * 1.55);
    return width > 0.0001 ? clamp(1.0 - edgeDistance / width, 0.0, 1.0) : 0.0;
}

vec3 MaterialColor(float height, float variation)
{
    if (height < WaterLevel)
    {
        float depth = clamp((WaterLevel - height) / WaterLevel, 0.0, 1.0);
        return mix(vec3(0.08, 0.42, 0.72), vec3(0.015, 0.10, 0.32), depth);
    }
    if (height < 0.34)
    {
        return mix(vec3(0.72, 0.62, 0.32), vec3(0.88, 0.79, 0.47), variation);
    }
    if (height < 0.66)
    {
        return mix(vec3(0.20, 0.47, 0.10), vec3(0.34, 0.63, 0.16), variation);
    }
    if (height < 0.88)
    {
        return mix(vec3(0.35, 0.35, 0.34), vec3(0.56, 0.56, 0.54), variation);
    }
    return mix(vec3(0.78, 0.82, 0.82), vec3(0.98, 0.99, 1.00), variation);
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec2 pixel = uv / texelSize;
    bool fragmentValid;
    ReadPixelHeight(pixel, fragmentValid);
    if (!fragmentValid)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    vec2 id = HexagonId(pixel);
    vec2 center = HexagonCenter(id);
    vec2 localPosition = pixel - center;
    bool centerValid;
    float rawHeight = SampleHexagonHeight(id, centerValid);
    if (!centerValid)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    float centerHeight = QuantizeHeight(rawHeight);
    float rightHeight = NeighborHeight(id + vec2(1.0, 0.0), centerHeight);
    float downRightHeight = NeighborHeight(id + vec2(0.0, 1.0), centerHeight);
    float downLeftHeight = NeighborHeight(id + vec2(-1.0, 1.0), centerHeight);
    float leftHeight = NeighborHeight(id + vec2(-1.0, 0.0), centerHeight);
    float upLeftHeight = NeighborHeight(id + vec2(0.0, -1.0), centerHeight);
    float upRightHeight = NeighborHeight(id + vec2(1.0, -1.0), centerHeight);

    const vec2 RightDirection = vec2(1.0, 0.0);
    const vec2 DownRightDirection = vec2(0.5, HalfSqrtThree);
    const vec2 DownLeftDirection = vec2(-0.5, HalfSqrtThree);

    float radius = max(hexagonSize, 1.0);
    float edgePlane = HalfSqrtThree * radius;
    float rightDrop = max((centerHeight - rightHeight) * heightSteps, 0.0);
    float downRightDrop = max((centerHeight - downRightHeight) * heightSteps, 0.0);
    float downLeftDrop = max((centerHeight - downLeftHeight) * heightSteps, 0.0);
    float rightProgress = SideProgress(edgePlane - dot(localPosition, RightDirection), rightDrop);
    float downRightProgress = SideProgress(
        edgePlane - dot(localPosition, DownRightDirection),
        downRightDrop);
    float downLeftProgress = SideProgress(
        edgePlane - dot(localPosition, DownLeftDirection),
        downLeftDrop);

    float sideProgress = rightProgress;
    float sideIndex = 0.0;
    if (downRightProgress > sideProgress)
    {
        sideProgress = downRightProgress;
        sideIndex = 1.0;
    }
    if (downLeftProgress > sideProgress)
    {
        sideProgress = downLeftProgress;
        sideIndex = 2.0;
    }

    float leftAverage = (leftHeight + upLeftHeight + downLeftHeight) / 3.0;
    float rightAverage = (rightHeight + upRightHeight + downRightHeight) / 3.0;
    float upAverage = (upLeftHeight + upRightHeight) * 0.5;
    float downAverage = (downLeftHeight + downRightHeight) * 0.5;
    vec3 normal = normalize(vec3(
        (leftAverage - rightAverage) * 7.0,
        (upAverage - downAverage) * 7.0,
        1.0));
    vec3 lightDirection = normalize(vec3(-0.55, -0.70, 0.80));
    float lighting = 0.52 + max(dot(normal, lightDirection), 0.0) * 0.55;

    float variation = Hash(id * 13.0 + floor((localPosition + vec2(radius)) * 0.35));
    vec3 color = MaterialColor(centerHeight, variation) * lighting;

    bool sideFace = centerHeight >= WaterLevel && sideProgress > 0.0001;
    if (sideFace)
    {
        bool dirtSide = centerHeight < 0.66;
        if (sideIndex > 0.5 && sideIndex < 1.5)
        {
            vec3 sideMaterial = dirtSide
                ? vec3(0.42, 0.27, 0.12)
                : vec3(0.36, 0.36, 0.34);
            color = sideMaterial * (0.72 + variation * 0.08);
        }
        else
        {
            vec3 sideMaterial = dirtSide
                ? vec3(0.37, 0.23, 0.10)
                : vec3(0.30, 0.30, 0.29);
            color = sideMaterial * (0.64 + variation * 0.08);
        }
    }

    float edgeDistance = max(0.0, (1.0 - HexagonMetric(localPosition)) * radius * 0.84);
    float gridLine = 1.0 - smoothstep(0.25, 0.90, edgeDistance);
    color *= 1.0 - gridLine * 0.16;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
