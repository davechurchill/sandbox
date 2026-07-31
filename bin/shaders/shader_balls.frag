#version 130

uniform sampler2D currentTexture;
uniform vec2 texelSize;

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
        (leftHeight - rightHeight) * 14.0,
        (upHeight - downHeight) * 14.0,
        1.0));
    vec3 lightDirection = normalize(vec3(-0.45, -0.60, 0.80));
    float lighting = 0.38 + max(dot(normal, lightDirection), 0.0) * 0.72;
    float gray = mix(0.16, 0.88, height) * lighting;

    gl_FragColor = vec4(vec3(clamp(gray, 0.0, 1.0)), 1.0);
}
