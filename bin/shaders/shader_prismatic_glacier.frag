#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float facetSize;
uniform float iceDepthAbsorption;
uniform float iridescence;
uniform float fractureIntensity;
uniform float causticSpeed;
uniform float u_time;

float Hash(vec2 point)
{
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453);
}

vec2 Hash2(vec2 point)
{
    return fract(sin(vec2(
        dot(point, vec2(127.1, 311.7)),
        dot(point, vec2(269.5, 183.3)))) * 43758.5453);
}

float Noise(vec2 point)
{
    vec2 cell = floor(point);
    vec2 fraction = fract(point);
    fraction = fraction * fraction * (3.0 - 2.0 * fraction);
    float a = Hash(cell);
    float b = Hash(cell + vec2(1.0, 0.0));
    float c = Hash(cell + vec2(0.0, 1.0));
    float d = Hash(cell + vec2(1.0, 1.0));
    return mix(mix(a, b, fraction.x), mix(c, d, fraction.x), fraction.y);
}

float TerrainHeight(vec2 uv, float fallback)
{
    float height = texture2D(currentTexture, clamp(uv, 0.0, 1.0)).r;
    return height <= 0.001 || height >= 0.999 ? fallback : height;
}

void FacetInformation(vec2 point, out vec2 center, out vec2 randomValue, out float edge)
{
    vec2 cell = floor(point);
    vec2 fraction = fract(point);
    float nearestDistance = 10.0;
    float secondDistance = 10.0;
    vec2 nearestCenter = cell;
    vec2 nearestRandom = vec2(0.5);

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 candidateRandom = Hash2(cell + neighbor);
            vec2 offset = neighbor + candidateRandom - fraction;
            float distanceToCenter = dot(offset, offset);
            if (distanceToCenter < nearestDistance)
            {
                secondDistance = nearestDistance;
                nearestDistance = distanceToCenter;
                nearestCenter = cell + neighbor + candidateRandom;
                nearestRandom = candidateRandom;
            }
            else if (distanceToCenter < secondDistance)
            {
                secondDistance = distanceToCenter;
            }
        }
    }

    center = nearestCenter;
    randomValue = nearestRandom;
    edge = sqrt(secondDistance) - sqrt(nearestDistance);
}

float CausticPattern(vec2 point, float time)
{
    vec2 cell = floor(point);
    vec2 fraction = fract(point);
    float nearestDistance = 10.0;
    float secondDistance = 10.0;

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 randomPoint = Hash2(cell + neighbor);
            randomPoint = 0.5 + 0.38 * sin(time * vec2(0.73, 0.59) + randomPoint * 6.2831853);
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

    float ridge = 1.0 - smoothstep(0.025, 0.135, secondDistance - nearestDistance);
    return ridge * ridge;
}

float FractureVeins(vec2 pixel, float time)
{
    vec2 broad = pixel * 0.017;
    vec2 warp = vec2(
        Noise(broad + vec2(time * 0.013, 7.2)),
        Noise(broad + vec2(13.8, -time * 0.011))) - 0.5;
    vec2 firstCoordinates = pixel * 0.050 + warp * 2.7;
    vec2 rotatedPixel = vec2(
        pixel.x * 0.74 + pixel.y * 0.67,
        -pixel.x * 0.67 + pixel.y * 0.74);
    vec2 secondCoordinates = rotatedPixel * 0.076 - warp * 3.1;
    float firstVein = 1.0 - smoothstep(0.012, 0.060, abs(Noise(firstCoordinates) - 0.5));
    float secondVein = 1.0 - smoothstep(0.010, 0.047, abs(Noise(secondCoordinates + 21.0) - 0.5));
    float pulse = 0.78 + 0.22 * sin(time * 0.75 + Noise(pixel * 0.011) * 6.2831853);
    return max(firstVein, secondVein * 0.72) * pulse;
}

vec3 Spectrum(float phase)
{
    return 0.56 + 0.44 * cos(6.2831853 * (phase + vec3(0.00, 0.33, 0.67)));
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    float rawHeight = texture2D(currentTexture, uv).r;
    if (rawHeight <= 0.001 || rawHeight >= 0.999)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    vec2 pixel = uv / texelSize;
    float safeFacetSize = max(facetSize, 1.0);
    vec2 facetCenter;
    vec2 facetRandom;
    float facetEdgeDistance;
    FacetInformation(pixel / safeFacetSize, facetCenter, facetRandom, facetEdgeDistance);

    vec2 facetUv = clamp(facetCenter * safeFacetSize * texelSize, texelSize, vec2(1.0) - texelSize);
    vec2 slopeStep = texelSize * max(safeFacetSize * 0.16, 1.0);
    float facetHeight = TerrainHeight(facetUv, rawHeight);
    float leftHeight = TerrainHeight(facetUv - vec2(slopeStep.x, 0.0), facetHeight);
    float rightHeight = TerrainHeight(facetUv + vec2(slopeStep.x, 0.0), facetHeight);
    float upHeight = TerrainHeight(facetUv - vec2(0.0, slopeStep.y), facetHeight);
    float downHeight = TerrainHeight(facetUv + vec2(0.0, slopeStep.y), facetHeight);
    vec2 facetTilt = (facetRandom - 0.5) * 0.42;
    vec3 normal = normalize(vec3(
        (leftHeight - rightHeight) * 16.0 + facetTilt.x,
        (upHeight - downHeight) * 16.0 + facetTilt.y,
        1.0));

    float animationTime = u_time * causticSpeed;
    vec2 flow = vec2(
        Noise(pixel * 0.025 + vec2(animationTime * 0.045, 3.7)),
        Noise(pixel * 0.025 + vec2(8.1, -animationTime * 0.038))) - 0.5;
    vec2 refractionOffset = (normal.xy * 1.7 + flow * 0.9) * texelSize * (2.0 + iceDepthAbsorption * 1.2);
    float refractedHeight = TerrainHeight(uv + refractionOffset, rawHeight);
    float depth = clamp((1.0 - refractedHeight) * iceDepthAbsorption, 0.0, 2.5);

    vec3 clearIce = vec3(0.49, 0.88, 0.98);
    vec3 blueIce = vec3(0.055, 0.43, 0.68);
    vec3 deepIce = vec3(0.010, 0.075, 0.24);
    float middleAbsorption = 1.0 - exp(-depth * 1.35);
    float deepAbsorption = 1.0 - exp(-max(depth - 0.55, 0.0) * 1.65);
    vec3 color = mix(clearIce, blueIce, middleAbsorption);
    color = mix(color, deepIce, deepAbsorption * 0.88);

    vec3 lightDirection = normalize(vec3(-0.42, -0.58, 0.79));
    vec3 viewDirection = vec3(0.0, 0.0, 1.0);
    vec3 halfDirection = normalize(lightDirection + viewDirection);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float slopeLighting = 0.48 + diffuse * 0.64;
    float specular = pow(max(dot(normal, halfDirection), 0.0), 42.0) * 0.48;
    color = color * slopeLighting + vec3(0.76, 0.94, 1.0) * specular;

    float facetEdge = 1.0 - smoothstep(0.025, 0.115, facetEdgeDistance);
    color *= 1.0 - facetEdge * 0.16;

    float fresnel = pow(1.0 - clamp(dot(normal, viewDirection), 0.0, 1.0), 2.3);
    float prismAmount = clamp((fresnel * 0.82 + facetEdge * 0.62) * iridescence, 0.0, 1.5);
    vec3 prism = Spectrum(rawHeight * 1.7 + Hash(floor(facetCenter)) + u_time * 0.025);
    color += prism * prismAmount * 0.30;

    float fracture = FractureVeins(pixel, u_time);
    float fractureAmount = clamp(fracture * fractureIntensity, 0.0, 1.5);
    color = mix(color, vec3(0.43, 0.84, 0.98), min(fractureAmount * 0.52, 0.80));
    color += vec3(0.30, 0.64, 0.82) * fractureAmount * (0.18 + depth * 0.10);

    float caustic = CausticPattern(pixel * 0.045 + flow * 1.8, animationTime);
    float causticDepth = smoothstep(0.04, 0.28, depth) * (1.0 - smoothstep(1.25, 2.35, depth));
    color += vec3(0.24, 0.52, 0.62) * caustic * causticDepth * 0.44;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
