#version 130

uniform sampler2D currentTexture;
uniform vec3 waterColor;
uniform float waterOpacity;
uniform bool overlayOnly;

vec3 terrainColor(float height)
{
    if (height < 0.28)
    {
        return mix(vec3(0.10, 0.16, 0.08), vec3(0.16, 0.34, 0.10), height / 0.28);
    }
    if (height < 0.58)
    {
        return mix(vec3(0.16, 0.34, 0.10), vec3(0.48, 0.38, 0.18), (height - 0.28) / 0.30);
    }
    if (height < 0.82)
    {
        return mix(vec3(0.48, 0.38, 0.18), vec3(0.52, 0.48, 0.42), (height - 0.58) / 0.24);
    }
    return mix(vec3(0.52, 0.48, 0.42), vec3(0.94, 0.97, 1.00), (height - 0.82) / 0.18);
}

void main()
{
    vec4 encoded = texture2D(currentTexture, gl_TexCoord[0].xy);
    float height = encoded.r;
    float trail = encoded.g;
    float water = encoded.b;

    if (height <= 0.001)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    vec3 terrain = terrainColor(height);
    vec3 deepWater = mix(waterColor, vec3(0.01, 0.08, 0.24), clamp(water, 0.0, 1.0));
    float coverage = clamp(max(water, trail * 0.55) * waterOpacity, 0.0, 1.0);
    float highlight = smoothstep(0.55, 1.0, water) * 0.22;

    if (overlayOnly)
    {
        vec3 overlayColor = clamp(deepWater + vec3(highlight), 0.0, 1.0);
        gl_FragColor = vec4(overlayColor, coverage);
        return;
    }

    vec3 result = mix(terrain, deepWater, coverage) + vec3(highlight);

    gl_FragColor = vec4(clamp(result, 0.0, 1.0), 1.0);
}
