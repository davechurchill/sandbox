#version 130

// Adapted from "Mountain Peak" by Alexander Alekseev aka TDM, 2014.
// Original and adaptation licensed under CC BY-NC-SA 3.0 Unported:
// https://creativecommons.org/licenses/by-nc-sa/3.0/
// This adaptation replaces the original generated/raymarched geometry with
// the sandbox's externally supplied projected height texture.

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float u_time;
uniform float snowLine;
uniform float snowCoverage;
uniform float rockContrast;
uniform float sunAzimuth;
uniform float sunElevation;
uniform float hazeAmount;

float Hash(vec2 point)
{
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453);
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
    for (int octave = 0; octave < 5; ++octave)
    {
        value += Noise(point) * amplitude;
        point = mat2(1.67, 1.10, -1.10, 1.67) * point + vec2(13.4, 7.8);
        amplitude *= 0.5;
    }
    return value;
}

float TerrainHeight(vec2 uv, float fallback)
{
    float height = texture2D(currentTexture, clamp(uv, vec2(0.0), vec2(1.0))).r;
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
    vec2 gradient = vec2(rightHeight - leftHeight, downHeight - upHeight);
    vec3 normal = normalize(vec3(-gradient.x * 25.0, -gradient.y * 25.0, 1.0));

    vec2 wideStep = texelSize * 3.0;
    float farLeft = TerrainHeight(uv - vec2(wideStep.x, 0.0), height);
    float farRight = TerrainHeight(uv + vec2(wideStep.x, 0.0), height);
    float farUp = TerrainHeight(uv - vec2(0.0, wideStep.y), height);
    float farDown = TerrainHeight(uv + vec2(0.0, wideStep.y), height);
    vec2 wideGradient = vec2(farRight - farLeft, farDown - farUp);
    float slope = clamp(max(length(gradient) * 30.0, length(wideGradient) * 10.0), 0.0, 1.0);
    float curvature = leftHeight + rightHeight + upHeight + downHeight - height * 4.0;

    float azimuth = radians(sunAzimuth);
    float elevation = radians(sunElevation);
    vec3 sunDirection = normalize(vec3(
        cos(elevation) * cos(azimuth),
        cos(elevation) * sin(azimuth),
        sin(elevation)));
    vec3 viewDirection = vec3(0.0, 0.0, 1.0);
    vec3 halfDirection = normalize(sunDirection + viewDirection);
    float diffuse = max(dot(normal, sunDirection), 0.0);
    float specular = pow(max(dot(normal, halfDirection), 0.0), 38.0);

    vec2 pixel = uv / texelSize;
    vec2 rockWarp = vec2(
        Noise(pixel * 0.010 + vec2(4.3, 17.1)),
        Noise(pixel * 0.010 + vec2(29.7, 6.2))) - 0.5;
    float broadRock = Fbm(pixel * 0.020 + rockWarp * 1.8);
    float fineRock = Fbm(pixel * 0.075 + rockWarp * 3.2 + vec2(19.0, 7.0));
    float rockTone = clamp(0.22 + height * 0.44 + (broadRock - 0.5) * 0.48 + (fineRock - 0.5) * 0.17, 0.0, 1.0);
    rockTone = clamp((rockTone - 0.5) * rockContrast + 0.5, 0.0, 1.0);

    vec3 abyssRock = vec3(0.005, 0.010, 0.020);
    vec3 blueBlackRock = vec3(0.035, 0.055, 0.078);
    vec3 weatheredRock = vec3(0.145, 0.155, 0.165);
    vec3 rockColor = mix(abyssRock, blueBlackRock, smoothstep(0.0, 0.58, rockTone));
    rockColor = mix(rockColor, weatheredRock, smoothstep(0.52, 1.0, rockTone) * 0.78);

    vec3 coolSky = vec3(0.105, 0.175, 0.275);
    vec3 warmSun = vec3(1.00, 0.69, 0.42);
    float ambientOcclusion = 1.0 - smoothstep(0.001, 0.012, curvature);
    rockColor *= (0.27 + diffuse * 0.76) * mix(0.68, 1.0, ambientOcclusion);
    rockColor += coolSky * (0.065 + normal.z * 0.105);
    rockColor += warmSun * diffuse * (0.025 + rockTone * 0.025);
    rockColor += warmSun * specular * 0.095;

    float coverage = clamp(snowCoverage * 0.5, 0.0, 1.0);
    float effectiveSnowLine = snowLine - coverage * 0.10;
    float elevationSnow = smoothstep(effectiveSnowLine - 0.055, effectiveSnowLine + 0.075, height);
    float slopeLimit = mix(0.16, 0.72, coverage);
    float slopeRetention = 1.0 - smoothstep(slopeLimit, min(slopeLimit + 0.24, 1.0), slope);
    vec2 windDirection = normalize(vec2(cos(azimuth), sin(azimuth)));
    vec2 crossWind = vec2(-windDirection.y, windDirection.x);
    float windDeposit = Fbm(vec2(
        dot(pixel, windDirection) * 0.012,
        dot(pixel, crossWind) * 0.034) + vec2(3.2, 11.7));
    float snow = elevationSnow * slopeRetention * smoothstep(0.0, 0.08, snowCoverage);
    snow += (windDeposit - 0.54) * 0.30 * elevationSnow * coverage;
    snow = clamp(snow, 0.0, 1.0);

    vec3 shadowSnow = vec3(0.34, 0.50, 0.66);
    vec3 clearSnow = vec3(0.80, 0.90, 0.97);
    vec3 sunlightSnow = vec3(1.00, 0.91, 0.76);
    vec3 snowColor = mix(shadowSnow, clearSnow, 0.30 + diffuse * 0.60);
    snowColor = mix(snowColor, sunlightSnow, diffuse * 0.38);
    snowColor += coolSky * normal.z * 0.16;
    snowColor += warmSun * specular * 0.22;
    snowColor *= 0.90 + (fineRock - 0.5) * 0.08;

    vec3 color = mix(rockColor, snowColor, smoothstep(0.06, 0.92, snow));

    float alongWind = dot(pixel, windDirection);
    float acrossWind = dot(pixel, crossWind);
    vec2 wispCoordinates = vec2(
        alongWind * 0.006 - u_time * 0.10,
        acrossWind * 0.020);
    float broadHaze = Fbm(wispCoordinates + vec2(7.1, 21.4));
    float narrowHaze = Noise(wispCoordinates * vec2(1.8, 2.7) + vec2(-u_time * 0.035, 9.8));
    float wisps = smoothstep(0.48, 0.76, broadHaze * 0.72 + narrowHaze * 0.28);
    float valleyHaze = 1.0 - smoothstep(0.30, 0.76, height);
    float haze = clamp((valleyHaze * 0.23 + wisps * 0.52) * hazeAmount, 0.0, 0.68);
    vec3 hazeColor = mix(vec3(0.16, 0.25, 0.36), vec3(0.38, 0.42, 0.46), diffuse * 0.35);
    color = mix(color, hazeColor, haze);

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
