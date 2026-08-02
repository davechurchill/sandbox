#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float waterLevel;
uniform float rockLevel;
uniform float rockSlope;

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

float validHeight(vec2 uv, float fallback)
{
    float height = texture2D(currentTexture, clamp(uv, 0.0, 1.0)).r;
    return height > 0.001 && height < 0.999 ? height : fallback;
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec4 state = texture2D(currentTexture, uv);
    float height = state.r;
    float fuel = state.g;
    float initialFuel = state.a;
    if (height <= 0.001 || height >= 0.999)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    float leftHeight = validHeight(uv - vec2(texelSize.x, 0.0), height);
    float rightHeight = validHeight(uv + vec2(texelSize.x, 0.0), height);
    float upHeight = validHeight(uv - vec2(0.0, texelSize.y), height);
    float downHeight = validHeight(uv + vec2(0.0, texelSize.y), height);
    vec3 normal = normalize(vec3(
        (leftHeight - rightHeight) * 18.0,
        (upHeight - downHeight) * 18.0,
        1.0));
    vec3 sunlight = normalize(vec3(-0.45, -0.62, 0.78));
    float diffuse = max(dot(normal, sunlight), 0.0);
    float lighting = 0.43 + diffuse * 0.67;

    vec2 pixel = uv / texelSize;
    float patchVariation = noise(pixel * 0.012 + vec2(19.0, 7.0));
    float fineVariation = noise(pixel * 0.22 + vec2(43.0, 29.0));
    float steepness = clamp(length(normal.xy), 0.0, 1.0);

    float depth = clamp((waterLevel - height) / max(waterLevel, 0.001), 0.0, 1.0);
    vec3 shallowWater = vec3(0.080, 0.300, 0.390);
    vec3 deepWater = vec3(0.006, 0.035, 0.160);
    vec3 pondColor = mix(shallowWater, deepWater, smoothstep(0.0, 0.65, depth));

    float landHeight = clamp((height - waterLevel) / max(1.0 - waterLevel, 0.001), 0.0, 1.0);
    vec3 deepGrass = vec3(0.045, 0.155, 0.032);
    vec3 meadowGrass = vec3(0.155, 0.355, 0.075);
    vec3 hillGrass = vec3(0.355, 0.470, 0.145);
    vec3 grassColor = mix(deepGrass, meadowGrass, smoothstep(0.0, 0.42, landHeight));
    grassColor = mix(grassColor, hillGrass, smoothstep(0.38, 0.82, landHeight));

    float exposedGround = smoothstep(0.62, 0.94, steepness) * smoothstep(0.32, 0.85, height);
    vec3 earth = mix(vec3(0.18, 0.125, 0.070), vec3(0.39, 0.37, 0.31), height);
    vec3 color = mix(grassColor, earth, exposedGround * 0.78);

    float wetBank = 1.0 - smoothstep(waterLevel + 0.005, waterLevel + 0.060, height);
    color = mix(color, vec3(0.19, 0.225, 0.105), wetBank * 0.62);

    float waterBlend = 1.0 - smoothstep(waterLevel - 0.018, waterLevel + 0.030, height);
    color = mix(color, pondColor, waterBlend);
    float surfaceLighting = mix(lighting, 0.96, waterBlend);

    bool burnableTerrain = height > waterLevel
        && height < rockLevel
        && steepness < rockSlope;
    if (!burnableTerrain)
    {
        gl_FragColor = vec4(clamp(color * surfaceLighting, 0.0, 1.0), 1.0);
        return;
    }

    vec2 canopyCoordinate = pixel / 5.0;
    vec2 canopyCell = floor(canopyCoordinate);
    vec2 canopyOffset = vec2(
        hash(canopyCell + vec2(13.0, 37.0)),
        hash(canopyCell + vec2(71.0, 19.0))) - 0.5;
    float canopyDistance = length(
        fract(canopyCoordinate) - 0.5 - canopyOffset * 0.38);
    float canopy = 1.0 - smoothstep(0.22, 0.56, canopyDistance);

    vec3 deepForest = vec3(0.010, 0.070, 0.014);
    vec3 sunlitForest = vec3(0.105, 0.330, 0.060);
    vec3 forest = mix(deepForest, sunlitForest, patchVariation * 0.50 + diffuse * 0.50);
    forest *= 0.73 + fineVariation * 0.16 + canopy * 0.28;
    forest += canopy * vec3(0.012, 0.045, 0.006);
    float treeCoverage = smoothstep(0.005, 0.18, fuel);
    float treeBlend = treeCoverage * mix(0.58, 1.0, canopy);
    vec3 forestTint = color * vec3(0.58, 0.82, 0.58) + forest * 0.55;
    color = mix(color, forestTint, treeBlend);

    float burnedAmount = initialFuel > 0.001
        ? clamp((initialFuel - fuel) / initialFuel, 0.0, 1.0)
        : 0.0;
    vec3 ash = mix(vec3(0.055, 0.038, 0.025), vec3(0.16, 0.095, 0.045), fineVariation);
    vec3 burnedTerrain = color * vec3(0.38, 0.31, 0.25) + ash * 0.55;
    color = mix(color, burnedTerrain, burnedAmount * 0.84);
    color *= surfaceLighting;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
