#version 130

// Adapted from "Minecraft" by Reinder Nijhoff, 2013:
// https://www.shadertoy.com/view/4ds3WS
// The original combines Markus Persson's JavaScript Minecraft experiment
// with voxel-shader code by Inigo Quilez:
// https://twitter.com/notch/status/275331530040160256
// https://www.shadertoy.com/view/4dfGzs
// Original and adaptation licensed under CC BY-NC-SA 4.0 International:
// https://creativecommons.org/licenses/by-nc-sa/4.0/
// Terrain generation, camera raycasting, trees, clouds, and structures were
// removed; the sandbox's projected height texture supplies every voxel column.

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float time;
uniform float blockSize;
uniform float heightSteps;
uniform float blockRelief;
uniform float waterLevel;
uniform float snowLine;
uniform float aoStrength;

const int MaterialDirt = 0;
const int MaterialGrass = 1;
const int MaterialStone = 4;
const int MaterialWater = 9;
const int MaterialSnow = 10;
const int MaterialSand = 12;

float Hash(float value)
{
    return fract(sin(value) * 43758.5453);
}

float Hash(vec2 point)
{
    return Hash(dot(point, vec2(127.1, 311.7)));
}

bool IsValidHeight(float height)
{
    return height > 0.001 && height < 0.999;
}

float ReadHeight(vec2 uv, out bool valid)
{
    valid = uv.x >= 0.0 && uv.y >= 0.0 && uv.x <= 1.0 && uv.y <= 1.0;
    if (!valid)
    {
        return 0.0;
    }

    float height = texture2D(currentTexture, uv).r;
    valid = IsValidHeight(height);
    return height;
}

float ReadCellPoint(vec2 cell, vec2 pointWithinCell, out bool valid)
{
    vec2 textureDimensions = 1.0 / texelSize;
    vec2 cellMinimum = cell * blockSize;
    vec2 cellMaximum = min(cellMinimum + vec2(blockSize), textureDimensions);
    valid = cellMaximum.x > 0.0 && cellMaximum.y > 0.0
        && cellMinimum.x < textureDimensions.x && cellMinimum.y < textureDimensions.y
        && cellMaximum.x > cellMinimum.x && cellMaximum.y > cellMinimum.y;
    if (!valid)
    {
        return 0.0;
    }

    vec2 clippedMinimum = max(cellMinimum, vec2(0.0));
    vec2 samplePixel = clamp(
        mix(clippedMinimum, cellMaximum, pointWithinCell),
        vec2(0.0),
        textureDimensions - vec2(1.0));
    return ReadHeight((samplePixel + vec2(0.5)) * texelSize, valid);
}

float ReadCellCenter(vec2 cell, out bool valid)
{
    return ReadCellPoint(cell, vec2(0.5), valid);
}

float SampleCellHeight(vec2 cell, out bool valid)
{
    float height = ReadCellCenter(cell, valid);
    if (valid) { return height; }

    height = ReadCellPoint(cell, vec2(0.25, 0.25), valid);
    if (valid) { return height; }
    height = ReadCellPoint(cell, vec2(0.75, 0.25), valid);
    if (valid) { return height; }
    height = ReadCellPoint(cell, vec2(0.25, 0.75), valid);
    if (valid) { return height; }
    return ReadCellPoint(cell, vec2(0.75, 0.75), valid);
}

float QuantizeHeight(float height)
{
    float levels = max(heightSteps, 1.0);
    return floor(height * levels + 0.5) / levels;
}

float SurfaceHeight(float rawHeight)
{
    return rawHeight < waterLevel ? QuantizeHeight(waterLevel) : QuantizeHeight(rawHeight);
}

float NeighborSurface(vec2 cell, float fallback)
{
    bool valid;
    float rawHeight = SampleCellHeight(cell, valid);
    return valid ? SurfaceHeight(rawHeight) : fallback;
}

int TopMaterial(float rawHeight, float surfaceHeight, float maximumTerrainDifference, vec2 cell)
{
    if (rawHeight < waterLevel)
    {
        return MaterialWater;
    }

    float levelSize = 1.0 / max(heightSteps, 1.0);
    if (surfaceHeight <= waterLevel + levelSize * 2.2)
    {
        return MaterialSand;
    }
    if (surfaceHeight >= snowLine)
    {
        return MaterialSnow;
    }

    float mountainBand = smoothstep(snowLine - 0.16, snowLine, surfaceHeight);
    float exposed = maximumTerrainDifference * heightSteps;
    float stoneChance = Hash(cell * vec2(37.0, 91.0) + 17.0);
    if (mountainBand > 0.18 && (exposed > 1.55 || stoneChance < mountainBand * 0.62))
    {
        return MaterialStone;
    }

    return MaterialGrass;
}

float TextureBrightness(int material, vec2 texturePixel, vec2 cell, float layer)
{
    float seed = texturePixel.x + texturePixel.y * 347.0
        + float(material) * 4321.0
        + cell.x * 131.0 + cell.y * 197.0
        + floor(layer) * 7919.0;
    return 1.0 - Hash(seed) * (96.0 / 255.0);
}

vec3 MaterialColor(
    int topMaterial,
    vec2 texturePixel,
    vec2 cell,
    float rawHeight,
    float layerDepth,
    bool sideFace)
{
    int material = topMaterial;
    float layer = floor(max(layerDepth, 0.0));
    bool grassLip = false;
    bool snowLip = false;

    if (sideFace)
    {
        if (topMaterial == MaterialGrass)
        {
            material = layer < 3.0 ? MaterialDirt : MaterialStone;
            float grassEdge = 2.0 + mod(
                (texturePixel.x * texturePixel.x * 3.0 + texturePixel.x * 81.0) / 4.0,
                4.0);
            grassLip = layer < 1.0 && texturePixel.y < grassEdge;
        }
        else if (topMaterial == MaterialSand)
        {
            material = layer < 4.0 ? MaterialSand : MaterialStone;
        }
        else if (topMaterial == MaterialSnow)
        {
            material = MaterialStone;
            snowLip = layer < 1.0 && texturePixel.y < 2.0 + floor(Hash(cell) * 2.0);
        }
    }

    float brightness = TextureBrightness(material, texturePixel, cell, layer);
    vec3 color = vec3(150.0, 108.0, 74.0) / 255.0;

    if (material == MaterialGrass || grassLip)
    {
        color = vec3(106.0, 170.0, 64.0) / 255.0;
    }
    else if (material == MaterialStone)
    {
        color = vec3(127.0) / 255.0;
        float stoneMottle = Hash(texturePixel * vec2(17.0, 29.0) + cell * 7.0 + layer);
        brightness *= mix(0.86, 1.08, stoneMottle);
    }
    else if (material == MaterialWater)
    {
        float depth = clamp((waterLevel - rawHeight) / max(waterLevel, 0.001), 0.0, 1.0);
        vec3 shallowWater = vec3(64.0, 112.0, 255.0) / 255.0;
        vec3 deepWater = vec3(28.0, 48.0, 166.0) / 255.0;
        color = mix(shallowWater, deepWater, depth * 0.78);
        float frame = floor(time * 3.0);
        float waterPixel = Hash(texturePixel + cell * 16.0 + vec2(frame * 3.0, -frame * 2.0));
        brightness = mix(0.82, 1.08, waterPixel);
    }
    else if (material == MaterialSnow || snowLip)
    {
        color = vec3(0.88, 0.92, 0.96);
        brightness = mix(0.88, 1.04, brightness);
    }
    else if (material == MaterialSand)
    {
        color = vec3(0.82, 0.76, 0.51);
        brightness = mix(0.82, 1.04, brightness);
    }

    if (grassLip)
    {
        color = vec3(106.0, 170.0, 64.0) / 255.0;
    }
    if (snowLip)
    {
        color = vec3(0.88, 0.92, 0.96);
        brightness = max(brightness, 0.92);
    }

    return color * brightness;
}

float IsOccluder(float neighborHeight, float centerHeight)
{
    return step(centerHeight + 0.5 / max(heightSteps, 1.0), neighborHeight);
}

float CornerOcclusion(float firstEdge, float secondEdge, float diagonal)
{
    return firstEdge + secondEdge + diagonal * (1.0 - max(firstEdge, secondEdge));
}

float AmbientOcclusion(float occlusion)
{
    float base = clamp(1.0 - occlusion * 0.16 * aoStrength, 0.32, 1.0);
    return base * base;
}

vec3 FaceLighting(vec3 normal, float shadow)
{
    vec3 sunDirection = normalize(vec3(-0.5, 0.6, 0.7));
    vec3 backDirection = normalize(vec3(0.5, -0.6, 0.7));
    float diffuse = max(dot(normal, sunDirection), 0.0);
    float backLight = max(dot(normal, backDirection), 0.0);
    float sky = 0.5 + 0.5 * normal.z;

    vec3 lighting = vec3(0.16);
    lighting += diffuse * vec3(0.95, 0.92, 0.86) * (0.72 + shadow * 0.28);
    lighting += backLight * vec3(0.30, 0.19, 0.32) * 0.34;
    lighting += sky * vec3(0.60, 0.71, 0.75) * 0.56;
    return lighting;
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    bool fragmentValid;
    ReadHeight(uv, fragmentValid);
    if (!fragmentValid)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    vec2 pixel = uv / texelSize;
    vec2 cell = floor(pixel / blockSize);
    vec2 withinBlock = fract(pixel / blockSize);

    bool centerValid;
    float rawHeight = SampleCellHeight(cell, centerValid);
    if (!centerValid)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    float surfaceHeight = SurfaceHeight(rawHeight);
    float leftHeight = NeighborSurface(cell + vec2(-1.0, 0.0), surfaceHeight);
    float rightHeight = NeighborSurface(cell + vec2(1.0, 0.0), surfaceHeight);
    float upHeight = NeighborSurface(cell + vec2(0.0, -1.0), surfaceHeight);
    float downHeight = NeighborSurface(cell + vec2(0.0, 1.0), surfaceHeight);
    float upLeftHeight = NeighborSurface(cell + vec2(-1.0, -1.0), surfaceHeight);
    float upRightHeight = NeighborSurface(cell + vec2(1.0, -1.0), surfaceHeight);
    float downLeftHeight = NeighborSurface(cell + vec2(-1.0, 1.0), surfaceHeight);
    float downRightHeight = NeighborSurface(cell + vec2(1.0, 1.0), surfaceHeight);

    float maximumTerrainDifference = max(
        max(abs(surfaceHeight - leftHeight), abs(surfaceHeight - rightHeight)),
        max(abs(surfaceHeight - upHeight), abs(surfaceHeight - downHeight)));
    int topMaterial = TopMaterial(rawHeight, surfaceHeight, maximumTerrainDifference, cell);

    float leftOccluder = IsOccluder(leftHeight, surfaceHeight);
    float rightOccluder = IsOccluder(rightHeight, surfaceHeight);
    float upOccluder = IsOccluder(upHeight, surfaceHeight);
    float downOccluder = IsOccluder(downHeight, surfaceHeight);
    float topLeftOcclusion = CornerOcclusion(
        leftOccluder,
        upOccluder,
        IsOccluder(upLeftHeight, surfaceHeight));
    float topRightOcclusion = CornerOcclusion(
        rightOccluder,
        upOccluder,
        IsOccluder(upRightHeight, surfaceHeight));
    float bottomLeftOcclusion = CornerOcclusion(
        leftOccluder,
        downOccluder,
        IsOccluder(downLeftHeight, surfaceHeight));
    float bottomRightOcclusion = CornerOcclusion(
        rightOccluder,
        downOccluder,
        IsOccluder(downRightHeight, surfaceHeight));

    float rightDrop = max((surfaceHeight - rightHeight) * heightSteps, 0.0);
    float downDrop = max((surfaceHeight - downHeight) * heightSteps, 0.0);
    float rightWidth = min(0.48, rightDrop * blockRelief * 2.0 / max(blockSize, 1.0));
    float downWidth = min(0.48, downDrop * blockRelief * 2.0 / max(blockSize, 1.0));
    bool rightFaceVisible = rightDrop > 0.25 && withinBlock.x > 1.0 - rightWidth;
    bool downFaceVisible = downDrop > 0.25 && withinBlock.y > 1.0 - downWidth;
    float rightProgress = rightFaceVisible
        ? clamp((withinBlock.x - (1.0 - rightWidth)) / max(rightWidth, 0.0001), 0.0, 1.0)
        : 0.0;
    float downProgress = downFaceVisible
        ? clamp((withinBlock.y - (1.0 - downWidth)) / max(downWidth, 0.0001), 0.0, 1.0)
        : 0.0;
    bool useRightFace = rightFaceVisible && (!downFaceVisible || rightProgress >= downProgress);
    bool useDownFace = downFaceVisible && !useRightFace;

    vec2 texturePixel;
    float layerDepth = 0.0;
    float occlusion;
    float shadow;
    vec3 normal;
    bool sideFace = useRightFace || useDownFace;

    if (useRightFace)
    {
        layerDepth = rightProgress * rightDrop;
        float layerHeight = surfaceHeight - layerDepth / max(heightSteps, 1.0);
        texturePixel = floor(vec2(
            fract(withinBlock.y) * 16.0,
            fract(layerDepth) * 16.0));
        float upperSideOcclusion = IsOccluder(upHeight, layerHeight)
            + IsOccluder(upRightHeight, layerHeight);
        float lowerSideOcclusion = IsOccluder(downHeight, layerHeight)
            + IsOccluder(downRightHeight, layerHeight);
        occlusion = mix(upperSideOcclusion, lowerSideOcclusion, withinBlock.y)
            + IsOccluder(rightHeight, layerHeight) * 0.5;
        shadow = 1.0 - rightOccluder * 0.32 * aoStrength;
        normal = vec3(1.0, 0.0, 0.0);
    }
    else if (useDownFace)
    {
        layerDepth = downProgress * downDrop;
        float layerHeight = surfaceHeight - layerDepth / max(heightSteps, 1.0);
        texturePixel = floor(vec2(
            fract(withinBlock.x) * 16.0,
            fract(layerDepth) * 16.0));
        float leftSideOcclusion = IsOccluder(leftHeight, layerHeight)
            + IsOccluder(downLeftHeight, layerHeight);
        float rightSideOcclusion = IsOccluder(rightHeight, layerHeight)
            + IsOccluder(downRightHeight, layerHeight);
        occlusion = mix(leftSideOcclusion, rightSideOcclusion, withinBlock.x)
            + IsOccluder(downHeight, layerHeight) * 0.5;
        shadow = 1.0 - downOccluder * 0.32 * aoStrength;
        normal = vec3(0.0, 1.0, 0.0);
    }
    else
    {
        texturePixel = floor(withinBlock * 16.0);
        float upperOcclusion = mix(topLeftOcclusion, topRightOcclusion, withinBlock.x);
        float lowerOcclusion = mix(bottomLeftOcclusion, bottomRightOcclusion, withinBlock.x);
        occlusion = mix(upperOcclusion, lowerOcclusion, withinBlock.y);
        shadow = 1.0 - max(leftOccluder, downOccluder) * 0.32 * aoStrength;
        normal = vec3(0.0, 0.0, 1.0);
    }

    vec3 color = MaterialColor(
        topMaterial,
        texturePixel,
        cell,
        rawHeight,
        layerDepth,
        sideFace);
    color *= FaceLighting(normal, shadow);
    color *= AmbientOcclusion(occlusion);

    if (!sideFace)
    {
        float edgeDistance = min(
            min(withinBlock.x, 1.0 - withinBlock.x),
            min(withinBlock.y, 1.0 - withinBlock.y)) * blockSize;
        float seam = 1.0 - smoothstep(0.20, 0.85, edgeDistance);
        color *= 1.0 - seam * 0.075;
    }

    color = pow(clamp(color, 0.0, 1.0), vec3(0.45));
    color = color * 0.25 + 0.75 * color * color * (3.0 - 2.0 * color);
    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
