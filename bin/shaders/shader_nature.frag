#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform int terrainType;
uniform float waterLevel;
uniform float textureStrength;

float hash(vec2 point)
{
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453);
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

float terrainHeight(vec2 uv, float fallback)
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

    float leftHeight = terrainHeight(uv - vec2(texelSize.x, 0.0), height);
    float rightHeight = terrainHeight(uv + vec2(texelSize.x, 0.0), height);
    float upHeight = terrainHeight(uv - vec2(0.0, texelSize.y), height);
    float downHeight = terrainHeight(uv + vec2(0.0, texelSize.y), height);

    vec3 normal = normalize(vec3(
        (leftHeight - rightHeight) * 18.0,
        (upHeight - downHeight) * 18.0,
        1.0));
    vec3 sunlight = normalize(vec3(-0.45, -0.62, 0.78));
    float diffuse = max(dot(normal, sunlight), 0.0);
    float lighting = 0.43 + diffuse * 0.67;

    vec2 pixel = uv / texelSize;
    float broadVariation = noise(pixel * 0.035);
    float patchVariation = noise(pixel * 0.012 + vec2(19.0, 7.0));
    float fineVariation = noise(pixel * 0.22 + vec2(43.0, 29.0));
    float steepness = clamp(length(normal.xy), 0.0, 1.0);
    vec3 color;

    if (terrainType == 0)
    {
        float depth = clamp((waterLevel - height) / max(waterLevel, 0.001), 0.0, 1.0);
        vec3 shallowWater = vec3(0.080, 0.300, 0.390);
        vec3 deepWater = vec3(0.006, 0.035, 0.160);
        vec3 pondColor = mix(shallowWater, deepWater, smoothstep(0.0, 0.65, depth));
        pondColor *= mix(1.0, 0.94 + broadVariation * 0.06, textureStrength);

        float landHeight = clamp((height - waterLevel) / max(1.0 - waterLevel, 0.001), 0.0, 1.0);
        vec3 deepGrass = vec3(0.045, 0.155, 0.032);
        vec3 meadowGrass = vec3(0.155, 0.355, 0.075);
        vec3 hillGrass = vec3(0.355, 0.470, 0.145);
        vec3 grassColor = mix(deepGrass, meadowGrass, smoothstep(0.0, 0.42, landHeight));
        grassColor = mix(grassColor, hillGrass, smoothstep(0.38, 0.82, landHeight));

        vec3 dryGrass = vec3(0.48, 0.43, 0.19);
        float dryPatch = smoothstep(0.58, 0.82, patchVariation + height * 0.20) * textureStrength;
        grassColor = mix(grassColor, dryGrass, dryPatch * 0.24);

        float exposedGround = smoothstep(0.62, 0.94, steepness) * smoothstep(0.32, 0.85, height);
        vec3 earth = mix(vec3(0.18, 0.125, 0.070), vec3(0.39, 0.37, 0.31), height);
        color = mix(grassColor, earth, exposedGround * 0.78);

        float wetBank = 1.0 - smoothstep(waterLevel + 0.005, waterLevel + 0.060, height);
        color = mix(color, vec3(0.19, 0.225, 0.105), wetBank * 0.62);
        color *= mix(1.0, 0.88 + broadVariation * 0.20, textureStrength);
        color += (fineVariation - 0.5) * vec3(0.018, 0.030, 0.012) * textureStrength;

        float waterBlend = 1.0 - smoothstep(waterLevel - 0.018, waterLevel + 0.030, height);
        color = mix(color, pondColor, waterBlend);
        float surfaceLighting = mix(lighting, 0.96, waterBlend);
        gl_FragColor = vec4(clamp(color * surfaceLighting, 0.0, 1.0), 1.0);
        return;
    }
    else if (terrainType == 1)
    {
        vec3 lowGrass = mix(
            vec3(0.065, 0.175, 0.050),
            vec3(0.205, 0.285, 0.105),
            smoothstep(0.05, 0.58, height));
        vec3 grayRock = mix(
            vec3(0.285, 0.290, 0.280),
            vec3(0.610, 0.615, 0.600),
            smoothstep(0.18, 0.92, height));

        float heightRock = smoothstep(0.30, 0.82, height);
        float slopeRock = smoothstep(0.38, 0.88, steepness);
        float rockAmount = clamp(heightRock * 0.78 + slopeRock * 0.38, 0.0, 1.0);
        color = mix(lowGrass, grayRock, rockAmount);
        color *= mix(1.0, 0.92 + broadVariation * 0.10 + fineVariation * 0.035, textureStrength);
    }
    else if (terrainType == 2)
    {
        vec3 shadedSand = vec3(0.47, 0.285, 0.105);
        vec3 sunlitSand = vec3(0.90, 0.705, 0.355);
        color = mix(shadedSand, sunlitSand, smoothstep(0.04, 0.88, height));

        float duneRipple = sin(pixel.x * 0.075 + pixel.y * 0.025 + broadVariation * 5.0);
        color *= mix(1.0, 0.91 + duneRipple * 0.045 + patchVariation * 0.10, textureStrength);
        float desertRock = smoothstep(0.72, 0.96, steepness);
        color = mix(color, vec3(0.34, 0.205, 0.105), desertRock * 0.68);
    }
    else
    {
        vec3 alpineGrass = mix(
            vec3(0.075, 0.205, 0.070),
            vec3(0.285, 0.345, 0.145),
            smoothstep(0.05, 0.58, height));
        vec3 mountainRock = mix(
            vec3(0.245, 0.235, 0.225),
            vec3(0.555, 0.545, 0.535),
            height);
        float rockAmount = smoothstep(0.42, 0.68, height) + smoothstep(0.58, 0.90, steepness) * 0.45;
        color = mix(alpineGrass, mountainRock, clamp(rockAmount, 0.0, 1.0));

        float snow = smoothstep(0.67, 0.84, height)
            * (1.0 - smoothstep(0.62, 0.96, steepness));
        vec3 snowColor = mix(vec3(0.68, 0.76, 0.78), vec3(0.965, 0.975, 0.965), diffuse);
        color = mix(color, snowColor, snow);
        color *= mix(1.0, 0.90 + broadVariation * 0.16, textureStrength);
    }

    gl_FragColor = vec4(clamp(color * lighting, 0.0, 1.0), 1.0);
}
