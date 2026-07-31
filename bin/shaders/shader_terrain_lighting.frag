#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;
uniform float lightAzimuth;
uniform float lightElevation;
uniform float ambientLight;
uniform float shadowStrength;
uniform float heightStrength;
uniform int palette;

vec3 terrainPalette(float height)
{
    if (height < 0.25)
    {
        return mix(vec3(0.03, 0.16, 0.34), vec3(0.12, 0.42, 0.58), height / 0.25);
    }
    if (height < 0.50)
    {
        return mix(vec3(0.18, 0.48, 0.16), vec3(0.50, 0.44, 0.20), (height - 0.25) / 0.25);
    }
    if (height < 0.78)
    {
        return mix(vec3(0.50, 0.44, 0.20), vec3(0.46, 0.43, 0.40), (height - 0.50) / 0.28);
    }
    return mix(vec3(0.46, 0.43, 0.40), vec3(0.96, 0.98, 1.00), (height - 0.78) / 0.22);
}

vec3 paletteColor(float height)
{
    if (palette == 1)
    {
        return vec3(height);
    }
    if (palette == 2)
    {
        return mix(vec3(0.16, 0.05, 0.01), vec3(1.00, 0.78, 0.30), height);
    }
    if (palette == 3)
    {
        return mix(vec3(0.02, 0.12, 0.28), vec3(0.86, 0.98, 1.00), height);
    }
    return terrainPalette(height);
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    float height = texture2D(currentTexture, uv).r;
    if (height <= 0.001)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    float leftHeight = texture2D(currentTexture, clamp(uv - vec2(texelSize.x, 0.0), 0.0, 1.0)).r;
    float rightHeight = texture2D(currentTexture, clamp(uv + vec2(texelSize.x, 0.0), 0.0, 1.0)).r;
    float upHeight = texture2D(currentTexture, clamp(uv - vec2(0.0, texelSize.y), 0.0, 1.0)).r;
    float downHeight = texture2D(currentTexture, clamp(uv + vec2(0.0, texelSize.y), 0.0, 1.0)).r;

    vec3 normal = normalize(vec3(
        (leftHeight - rightHeight) * heightStrength,
        (upHeight - downHeight) * heightStrength,
        1.0));

    float azimuth = radians(lightAzimuth);
    float elevation = radians(lightElevation);
    vec3 lightDirection = normalize(vec3(
        cos(elevation) * cos(azimuth),
        cos(elevation) * sin(azimuth),
        sin(elevation)));

    float diffuse = max(dot(normal, lightDirection), 0.0);
    float lighting = clamp(ambientLight + diffuse * shadowStrength, 0.0, 1.0);
    vec3 halfVector = normalize(lightDirection + vec3(0.0, 0.0, 1.0));
    float highlight = pow(max(dot(normal, halfVector), 0.0), 32.0) * 0.16;
    vec3 color = paletteColor(height) * lighting + vec3(highlight);

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
