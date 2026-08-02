#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float time;
uniform float glowIntensity;
uniform float networkScale;
uniform float pulseSpeed;
uniform float sporeDensity;

float Hash(vec2 point)
{
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453123);
}

vec2 Hash2(vec2 point)
{
    return fract(sin(vec2(
        dot(point, vec2(127.1, 311.7)),
        dot(point, vec2(269.5, 183.3)))) * 43758.5453123);
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

float Fbm(vec2 point)
{
    float value = 0.0;
    float amplitude = 0.5;
    mat2 rotation = mat2(0.80, 0.60, -0.60, 0.80);
    for (int octave = 0; octave < 4; ++octave)
    {
        value += Noise(point) * amplitude;
        point = rotation * point * 2.03 + vec2(17.1, 9.2);
        amplitude *= 0.5;
    }
    return value;
}

vec3 Cellular(vec2 point)
{
    vec2 baseCell = floor(point);
    vec2 fraction = fract(point);
    float firstDistance = 8.0;
    float secondDistance = 8.0;
    float nearestIdentity = 0.0;

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 offset = vec2(float(x), float(y));
            vec2 cell = baseCell + offset;
            vec2 feature = offset + 0.15 + Hash2(cell) * 0.70 - fraction;
            float distanceSquared = dot(feature, feature);
            if (distanceSquared < firstDistance)
            {
                secondDistance = firstDistance;
                firstDistance = distanceSquared;
                nearestIdentity = Hash(cell + vec2(31.7, 57.3));
            }
            else if (distanceSquared < secondDistance)
            {
                secondDistance = distanceSquared;
            }
        }
    }

    return vec3(sqrt(firstDistance), sqrt(secondDistance), nearestIdentity);
}

float TerrainHeight(vec2 uv, float fallback)
{
    float height = texture2D(currentTexture, clamp(uv, 0.0, 1.0)).r;
    return height <= 0.001 || height >= 0.999 ? fallback : height;
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

    float leftHeight = TerrainHeight(uv - vec2(texelSize.x, 0.0), height);
    float rightHeight = TerrainHeight(uv + vec2(texelSize.x, 0.0), height);
    float upHeight = TerrainHeight(uv - vec2(0.0, texelSize.y), height);
    float downHeight = TerrainHeight(uv + vec2(0.0, texelSize.y), height);
    vec3 normal = normalize(vec3(
        (leftHeight - rightHeight) * 18.0,
        (upHeight - downHeight) * 18.0,
        1.0));

    vec2 textureDimensions = 1.0 / texelSize;
    float shortestDimension = max(min(textureDimensions.x, textureDimensions.y), 1.0);
    vec2 aspectPosition = uv * textureDimensions / shortestDimension;
    vec2 networkPosition = aspectPosition * networkScale;

    vec2 warp = vec2(
        Fbm(networkPosition * 0.48 + vec2(2.3, 7.1)),
        Fbm(networkPosition * 0.48 + vec2(11.7, 3.8))) - 0.5;
    vec2 warpedPosition = networkPosition + warp * 1.55;
    vec3 cellular = Cellular(warpedPosition);

    float edgeDistance = cellular.y - cellular.x;
    float broadVein = 1.0 - smoothstep(0.045, 0.185, edgeDistance);
    float brightCore = 1.0 - smoothstep(0.018, 0.075, edgeDistance);
    float filamentBreakup = 0.68 + 0.32 * Fbm(warpedPosition * 2.2 + vec2(5.0, 13.0));
    broadVein *= filamentBreakup;

    float flowCoordinate = dot(warpedPosition, vec2(0.73, 1.17));
    flowCoordinate += Fbm(warpedPosition * 0.72) * 5.0 + cellular.z * 6.28318;
    float pulseWave = 0.5 + 0.5 * sin(flowCoordinate * 2.2 - time * pulseSpeed * 4.0);
    float pulse = pow(pulseWave, 5.0);
    float livingNetwork = broadVein * (0.48 + 0.52 * pulse) + brightCore * 0.62;

    float soilVariation = Fbm(networkPosition * 1.3 + vec2(23.0, 41.0));
    vec3 lowSoil = vec3(0.008, 0.010, 0.019);
    vec3 highSoil = vec3(0.035, 0.055, 0.067);
    vec3 soil = mix(lowSoil, highSoil, smoothstep(0.08, 0.92, height));
    soil *= 0.72 + soilVariation * 0.40;

    vec3 lightDirection = normalize(vec3(-0.48, -0.61, 0.76));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float slopeLighting = 0.34 + diffuse * 0.72;
    vec3 color = soil * slopeLighting;

    vec3 cyanGlow = vec3(0.025, 0.92, 0.78);
    vec3 violetGlow = vec3(0.34, 0.10, 0.92);
    vec3 networkColor = mix(cyanGlow, violetGlow, cellular.z * 0.55 + pulse * 0.20);
    float heightLife = mix(0.68, 1.15, smoothstep(0.12, 0.88, height));
    color += networkColor * livingNetwork * glowIntensity * heightLife;

    vec2 sporePosition = aspectPosition * networkScale * 2.8;
    vec2 sporeCell = floor(sporePosition);
    vec2 sporeOffset = fract(sporePosition) - Hash2(sporeCell);
    float sporeGate = step(1.0 - sporeDensity * 0.72, Hash(sporeCell + vec2(71.0, 19.0)));
    float sporeShape = 1.0 - smoothstep(0.015, 0.105, length(sporeOffset));
    float peakAmount = smoothstep(0.48, 0.90, height);
    float twinkle = 0.35 + 0.65 * pow(
        0.5 + 0.5 * sin(time * (1.8 + pulseSpeed) + Hash(sporeCell) * 18.0),
        3.0);
    float spores = sporeGate * sporeShape * peakAmount * twinkle;
    color += mix(cyanGlow, vec3(0.62, 0.28, 1.0), Hash(sporeCell))
        * spores * glowIntensity * 1.8;

    float halo = broadVein * broadVein * 0.12 * glowIntensity;
    color += vec3(0.02, 0.16, 0.14) * halo;
    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
