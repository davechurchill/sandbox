#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float time;
uniform float lensingStrength;
uniform float starDensity;
uniform float nebulaIntensity;
uniform float ringIntensity;
uniform float driftSpeed;

float Hash(vec2 point)
{
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453123);
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
    for (int octave = 0; octave < 5; octave++)
    {
        value += Noise(point) * amplitude;
        point = point * 2.03 + vec2(17.3, 9.1);
        amplitude *= 0.5;
    }
    return value;
}

float StarLayer(vec2 coordinate, float density, float threshold)
{
    vec2 cellCoordinate = coordinate * density;
    vec2 cell = floor(cellCoordinate);
    vec2 local = fract(cellCoordinate) - 0.5;
    vec2 starPosition = vec2(
        Hash(cell + vec2(5.2, 1.3)),
        Hash(cell + vec2(8.7, 9.2))) - 0.5;
    float distanceToStar = length(local - starPosition * 0.76);
    float star = 1.0 - smoothstep(0.018, 0.090, distanceToStar);
    float existence = step(threshold, Hash(cell + vec2(41.0, 73.0)));
    float flicker = 0.78 + 0.22 * sin(
        time * (1.4 + Hash(cell) * 2.2) + Hash(cell + 3.7) * 6.28318);
    return star * existence * flicker;
}

float ValidHeight(vec2 uv, float fallback)
{
    float height = texture2D(currentTexture, clamp(uv, 0.0, 1.0)).r;
    return height > 0.001 && height < 0.999 ? height : fallback;
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

    float leftHeight = ValidHeight(uv - vec2(texelSize.x, 0.0), height);
    float rightHeight = ValidHeight(uv + vec2(texelSize.x, 0.0), height);
    float upHeight = ValidHeight(uv - vec2(0.0, texelSize.y), height);
    float downHeight = ValidHeight(uv + vec2(0.0, texelSize.y), height);
    vec2 gradient = vec2(leftHeight - rightHeight, upHeight - downHeight);
    float slope = clamp(length(gradient) * 20.0, 0.0, 1.0);
    float curvature = leftHeight + rightHeight + upHeight + downHeight - height * 4.0;

    vec2 textureDimensions = 1.0 / texelSize;
    float shortestDimension = max(min(textureDimensions.x, textureDimensions.y), 1.0);
    vec2 spaceCoordinate = uv * textureDimensions / shortestDimension;
    float gravityWell = smoothstep(0.38, 0.96, height);
    vec2 drift = vec2(time * driftSpeed * 0.004, -time * driftSpeed * 0.0025);
    vec2 lensedCoordinate = spaceCoordinate + drift
        + gradient * lensingStrength * (0.22 + gravityWell * 0.70);

    float nebulaShape = Fbm(lensedCoordinate * 4.2 + vec2(time * 0.018, -time * 0.011));
    float nebulaDetail = Fbm(lensedCoordinate * 9.5 - vec2(time * 0.009, time * 0.014));
    float nebulaMask = smoothstep(0.35, 0.82, nebulaShape)
        * (0.35 + nebulaDetail * 0.65)
        * (1.0 - gravityWell * 0.58);
    vec3 violetNebula = vec3(0.20, 0.025, 0.42);
    vec3 cyanNebula = vec3(0.015, 0.24, 0.39);
    vec3 nebula = mix(violetNebula, cyanNebula, nebulaDetail)
        * nebulaMask * nebulaIntensity;

    float stars = StarLayer(lensedCoordinate, starDensity, 0.87);
    float distantStars = StarLayer(
        lensedCoordinate * 0.73 + vec2(11.0, 7.0),
        starDensity * 0.58,
        0.78) * 0.42;
    float starTemperature = Hash(floor(lensedCoordinate * starDensity) + 19.0);
    vec3 starColor = mix(vec3(0.55, 0.72, 1.0), vec3(1.0, 0.72, 0.48), starTemperature);

    float ringCoordinate = fract(height * 14.0 - time * driftSpeed * 0.13);
    float ringLine = 1.0 - smoothstep(0.025, 0.095, abs(ringCoordinate - 0.5));
    float ringRegion = smoothstep(0.48, 0.82, height)
        * smoothstep(0.04, 0.55, slope + abs(curvature) * 25.0);
    float accretionRing = ringLine * ringRegion * ringIntensity;
    vec3 ringColor = mix(vec3(0.20, 0.72, 1.0), vec3(0.95, 0.30, 1.0), height);

    vec3 color = vec3(0.0025, 0.004, 0.014) + nebula;
    color += starColor * stars + vec3(0.58, 0.68, 1.0) * distantStars;
    color *= 1.0 - gravityWell * 0.58;
    color += ringColor * accretionRing;
    color += vec3(0.05, 0.14, 0.25) * slope * 0.45;
    color += vec3(0.32, 0.12, 0.52) * abs(curvature) * lensingStrength * 3.0;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
