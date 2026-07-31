#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float blockSize;

const float WaterLevel = 0.28;

float hash(vec2 point)
{
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453);
}

float readBlockHeight(vec2 cell)
{
    vec2 samplePixel = (cell + vec2(0.5)) * blockSize;
    vec2 sampleUv = clamp(
        samplePixel * texelSize,
        texelSize * 0.5,
        vec2(1.0) - texelSize * 0.5);
    return texture2D(currentTexture, sampleUv).r;
}

float quantizeHeight(float height)
{
    float heightLevels = clamp(192.0 / blockSize, 6.0, 32.0);
    return floor(height * heightLevels + 0.5) / heightLevels;
}

vec3 materialColor(float height, float variation)
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
    vec2 cell = floor(pixel / blockSize);
    vec2 withinBlock = fract(pixel / blockSize);

    float rawHeight = readBlockHeight(cell);
    if (rawHeight <= 0.001 || rawHeight >= 0.999)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    float height = quantizeHeight(rawHeight);
    float leftHeight = quantizeHeight(readBlockHeight(cell + vec2(-1.0, 0.0)));
    float rightHeight = quantizeHeight(readBlockHeight(cell + vec2(1.0, 0.0)));
    float upHeight = quantizeHeight(readBlockHeight(cell + vec2(0.0, -1.0)));
    float downHeight = quantizeHeight(readBlockHeight(cell + vec2(0.0, 1.0)));

    vec3 normal = normalize(vec3(
        (leftHeight - rightHeight) * 7.0,
        (upHeight - downHeight) * 7.0,
        1.0));
    vec3 lightDirection = normalize(vec3(-0.55, -0.70, 0.80));
    float lighting = 0.52 + max(dot(normal, lightDirection), 0.0) * 0.55;

    float variation = hash(cell * 13.0 + floor(withinBlock * blockSize * 0.35));
    vec3 color = materialColor(height, variation) * lighting;

    if (height >= WaterLevel)
    {
        float rightDrop = height - rightHeight;
        if (rightDrop > 0.01)
        {
            float faceWidth = min(0.34, rightDrop * 2.8);
            float face = smoothstep(1.0 - faceWidth, 1.0 - faceWidth + 0.035, withinBlock.x);
            vec3 sideMaterial = height < 0.66
                ? vec3(0.37, 0.23, 0.10)
                : vec3(0.30, 0.30, 0.29);
            color = mix(color, sideMaterial * (0.64 + variation * 0.08), face);
        }

        float downDrop = height - downHeight;
        if (downDrop > 0.01)
        {
            float faceWidth = min(0.34, downDrop * 2.8);
            float face = smoothstep(1.0 - faceWidth, 1.0 - faceWidth + 0.035, withinBlock.y);
            vec3 sideMaterial = height < 0.66
                ? vec3(0.42, 0.27, 0.12)
                : vec3(0.36, 0.36, 0.34);
            color = mix(color, sideMaterial * (0.72 + variation * 0.08), face);
        }
    }

    float edgeDistance = min(
        min(withinBlock.x, 1.0 - withinBlock.x),
        min(withinBlock.y, 1.0 - withinBlock.y)) * blockSize;
    float gridLine = 1.0 - smoothstep(0.25, 0.90, edgeDistance);
    color *= 1.0 - gridLine * 0.16;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
