#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float u_time;

float terrainHeight(vec2 uv, float fallback)
{
    float height = texture2D(currentTexture, clamp(uv, 0.0, 1.0)).r;
    return height <= 0.001 || height >= 0.999 ? fallback : height;
}

float hash(vec2 point)
{
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453);
}

vec2 hash2(vec2 point)
{
    return fract(sin(vec2(
        dot(point, vec2(127.1, 311.7)),
        dot(point, vec2(269.5, 183.3)))) * 43758.5453);
}

float noise(vec2 point)
{
    vec2 cell = floor(point);
    vec2 fraction = fract(point);
    fraction = fraction * fraction * (3.0 - 2.0 * fraction);

    float a = hash(cell);
    float b = hash(cell + vec2(1.0, 0.0));
    float c = hash(cell + vec2(0.0, 1.0));
    float d = hash(cell + vec2(1.0, 1.0));
    return mix(mix(a, b, fraction.x), mix(c, d, fraction.x), fraction.y);
}

float waterSurface(vec2 pixel, float time)
{
    vec2 broadCoordinates = pixel * 0.018;
    float warpX = noise(broadCoordinates + vec2(time * 0.035, -time * 0.021));
    float warpY = noise(broadCoordinates + vec2(17.2, 4.6)
        + vec2(-time * 0.027, time * 0.031));
    vec2 warp = (vec2(warpX, warpY) - 0.5) * 2.0;

    vec2 broadFlow = pixel * 0.033 + warp * 0.85
        + vec2(time * 0.080, time * 0.025);
    vec2 rotatedPixel = vec2(
        pixel.x * 0.72 + pixel.y * 0.69,
        -pixel.x * 0.69 + pixel.y * 0.72);
    vec2 fineFlow = rotatedPixel * 0.071 + warp * 1.20
        + vec2(-time * 0.110, time * 0.070);
    return noise(broadFlow) * 0.68 + noise(fineFlow) * 0.32;
}

vec2 waterSlope(vec2 pixel, float time)
{
    const float sampleDistance = 1.5;
    float left = waterSurface(pixel - vec2(sampleDistance, 0.0), time);
    float right = waterSurface(pixel + vec2(sampleDistance, 0.0), time);
    float up = waterSurface(pixel - vec2(0.0, sampleDistance), time);
    float down = waterSurface(pixel + vec2(0.0, sampleDistance), time);
    return vec2(right - left, down - up) * 1.15;
}

float causticPattern(vec2 pixel, float time)
{
    vec2 point = pixel * 0.048;
    vec2 cell = floor(point);
    vec2 fraction = fract(point);
    float nearestDistance = 10.0;
    float secondDistance = 10.0;

    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 randomPoint = hash2(cell + neighbor);
            randomPoint = 0.5 + 0.38 * sin(
                time * vec2(0.43, 0.36) + randomPoint * 6.2831853);
            float distanceToPoint = length(neighbor + randomPoint - fraction);
            if (distanceToPoint < nearestDistance)
            {
                secondDistance = nearestDistance;
                nearestDistance = distanceToPoint;
            }
            else if (distanceToPoint < secondDistance)
            {
                secondDistance = distanceToPoint;
            }
        }
    }

    float cellEdge = 1.0 - smoothstep(
        0.025,
        0.120,
        secondDistance - nearestDistance);
    return cellEdge * cellEdge;
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    float height = texture2D(currentTexture, uv).r;
    if (height <= 0.001 || height >= 0.999)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    vec2 pixel = uv / texelSize;
    vec2 slope = waterSlope(pixel, u_time);
    vec2 refractedUv = clamp(uv + slope * texelSize * 2.6, 0.0, 1.0);
    float refractedHeight = terrainHeight(refractedUv, height);
    float depth = clamp(1.0 - refractedHeight, 0.0, 1.0);
    vec3 shallowWater = vec3(0.105, 0.560, 0.625);
    vec3 middleWater = vec3(0.020, 0.275, 0.440);
    vec3 deepWater = vec3(0.004, 0.045, 0.145);
    vec3 color = mix(shallowWater, middleWater, smoothstep(0.03, 0.46, depth));
    color = mix(color, deepWater, smoothstep(0.40, 0.94, depth));

    float leftHeight = terrainHeight(refractedUv - vec2(texelSize.x, 0.0), refractedHeight);
    float rightHeight = terrainHeight(refractedUv + vec2(texelSize.x, 0.0), refractedHeight);
    float upHeight = terrainHeight(refractedUv - vec2(0.0, texelSize.y), refractedHeight);
    float downHeight = terrainHeight(refractedUv + vec2(0.0, texelSize.y), refractedHeight);
    vec3 bottomNormal = normalize(vec3(
        (leftHeight - rightHeight) * 12.0,
        (upHeight - downHeight) * 12.0,
        1.0));
    vec3 lightDirection = normalize(vec3(-0.36, -0.48, 0.80));
    float bottomLighting = 0.84 + max(dot(bottomNormal, lightDirection), 0.0) * 0.16;
    color *= bottomLighting;

    float causticDepth = smoothstep(0.015, 0.08, depth)
        * (1.0 - smoothstep(0.48, 0.88, depth));
    float caustic = causticPattern(pixel + slope * 8.0, u_time);
    color += vec3(0.070, 0.115, 0.120) * caustic * causticDepth;

    vec3 waterNormal = normalize(vec3(-slope.x, -slope.y, 1.0));
    vec3 viewDirection = vec3(0.0, 0.0, 1.0);
    vec3 halfDirection = normalize(lightDirection + viewDirection);
    float diffuse = 0.94 + max(dot(waterNormal, lightDirection), 0.0) * 0.06;
    float specular = pow(max(dot(waterNormal, halfDirection), 0.0), 56.0) * 0.34;
    float reflection = pow(1.0 - max(dot(waterNormal, viewDirection), 0.0), 2.0);
    color = color * diffuse + vec3(0.80, 0.93, 1.00) * specular;
    color = mix(color, vec3(0.20, 0.42, 0.58), reflection * 0.45);

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
