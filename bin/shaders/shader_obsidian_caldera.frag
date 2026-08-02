#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float u_time;
uniform float lavaLevel;
uniform float crackIntensity;
uniform float crackScale;
uniform float flowSpeed;
uniform float cooling;
uniform float heatDistortion;

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

float Fbm(vec2 point)
{
    float value = 0.0;
    float amplitude = 0.5;
    for (int octave = 0; octave < 4; ++octave)
    {
        value += Noise(point) * amplitude;
        point = mat2(1.62, 1.18, -1.18, 1.62) * point + vec2(7.3, 4.1);
        amplitude *= 0.5;
    }
    return value;
}

float VoronoiEdge(vec2 point)
{
    vec2 cell = floor(point);
    vec2 local = fract(point);
    float nearest = 10.0;
    float secondNearest = 10.0;

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 feature = neighbor + 0.10 + Hash2(cell + neighbor) * 0.80;
            float distanceToFeature = length(feature - local);
            if (distanceToFeature < nearest)
            {
                secondNearest = nearest;
                nearest = distanceToFeature;
            }
            else if (distanceToFeature < secondNearest)
            {
                secondNearest = distanceToFeature;
            }
        }
    }

    return 1.0 - smoothstep(0.025, 0.115, secondNearest - nearest);
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
    float slope = length(gradient);
    vec2 downhill = slope > 0.00005 ? -gradient / slope : vec2(0.0);
    vec3 normal = normalize(vec3(-gradient.x * 22.0, -gradient.y * 22.0, 1.0));

    vec2 pixel = uv / texelSize;
    float shimmerTime = u_time * (0.25 + flowSpeed * 0.32);
    vec2 distortion = vec2(
        Noise(pixel * 0.010 + vec2(shimmerTime, 17.0)),
        Noise(pixel * 0.010 + vec2(41.0, -shimmerTime))) - 0.5;
    vec2 baseDomain = (pixel + distortion * heatDistortion * 8.0) / crackScale;
    vec2 flowDomain = baseDomain + downhill * u_time * flowSpeed * 0.16;

    float broadCrust = Fbm(baseDomain * 0.72 + vec2(5.0, 13.0));
    float fineCrust = Fbm(baseDomain * 3.25 + vec2(23.0, 2.0));
    float cracks = VoronoiEdge(flowDomain + (broadCrust - 0.5) * 0.55);
    float hairlineCracks = VoronoiEdge(flowDomain * 2.15 + vec2(31.7, 9.2));
    cracks = max(cracks, hairlineCracks * 0.42);
    cracks = clamp(cracks * crackIntensity, 0.0, 1.0);

    vec2 flowDirection = downhill + vec2(0.35, 0.15);
    flowDirection = length(flowDirection) > 0.0001
        ? normalize(flowDirection)
        : vec2(0.919, 0.394);
    float flowPulse = 0.62 + 0.38 * sin(
        dot(flowDomain, flowDirection) * 8.0
        - u_time * flowSpeed * 3.4
        + broadCrust * 6.2831);
    flowPulse = clamp(flowPulse, 0.0, 1.0);

    vec3 lightDirection = normalize(vec3(-0.46, -0.63, 0.72));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = pow(max(dot(reflect(-lightDirection, normal), vec3(0.0, 0.0, 1.0)), 0.0), 42.0);
    float glassSheen = pow(clamp(1.0 - normal.z, 0.0, 1.0), 2.0);

    vec3 blackGlass = mix(vec3(0.006, 0.008, 0.013), vec3(0.045, 0.035, 0.052), broadCrust);
    vec3 cooledPlate = mix(vec3(0.018, 0.020, 0.024), vec3(0.095, 0.068, 0.057), fineCrust);
    float plateBreakup = smoothstep(0.38, 0.73, broadCrust + fineCrust * 0.22);
    vec3 crustColor = mix(blackGlass, cooledPlate, plateBreakup * (0.28 + cooling * 0.48));
    crustColor *= 0.36 + diffuse * 0.58;
    crustColor += vec3(0.10, 0.075, 0.09) * specular + vec3(0.035, 0.018, 0.045) * glassSheen;

    float poolDepth = clamp((lavaLevel - height) / max(lavaLevel, 0.001), 0.0, 1.0);
    float pool = 1.0 - smoothstep(lavaLevel - 0.018, lavaLevel + 0.025, height);
    float hotNoise = Fbm(flowDomain * 1.45 + downhill * shimmerTime * 0.8);
    float heat = clamp(0.52 + poolDepth * 0.48 + hotNoise * 0.30 - cooling * 0.42, 0.0, 1.0);
    vec3 darkLava = vec3(0.34, 0.012, 0.002);
    vec3 orangeLava = vec3(1.00, 0.18, 0.006);
    vec3 yellowLava = vec3(1.00, 0.72, 0.08);
    vec3 lavaColor = mix(darkLava, orangeLava, smoothstep(0.12, 0.68, heat));
    lavaColor = mix(lavaColor, yellowLava, smoothstep(0.70, 1.0, heat) * flowPulse);

    float cooledIslands = smoothstep(0.42, 0.72, broadCrust + cooling * 0.24) * cooling;
    float liquidAmount = pool * (1.0 - cooledIslands * 0.76);
    vec3 color = mix(crustColor, lavaColor, liquidAmount);

    float buriedCracks = cracks * (1.0 - pool * 0.55);
    float crackHeat = clamp((1.0 - cooling * 0.68) * (0.65 + flowPulse * 0.50), 0.0, 1.0);
    vec3 crackColor = mix(vec3(0.52, 0.018, 0.002), vec3(1.0, 0.56, 0.035), crackHeat);
    color = mix(color, crackColor, buriedCracks);

    float hotBloom = max(liquidAmount * (0.18 + poolDepth * 0.25), buriedCracks * 0.20);
    color += vec3(0.42, 0.055, 0.002) * hotBloom * (1.0 - cooling * 0.55);
    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
