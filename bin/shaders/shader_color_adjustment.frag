#version 130

uniform sampler2D currentTexture;
uniform sampler2D terrainTexture;
uniform float windowHeight;
uniform vec2 projectionOrigin;
uniform vec2 projectionSize;
uniform float brightness;
uniform float contrast;
uniform float exposure;
uniform float saturation;
uniform float hue;
uniform float gamma;
uniform float temperature;

vec3 rotateHue(vec3 color, float angle)
{
    float luminance = dot(color, vec3(0.299, 0.587, 0.114));
    float inPhase = dot(color, vec3(0.596, -0.275, -0.321));
    float quadrature = dot(color, vec3(0.212, -0.523, 0.311));
    float chroma = length(vec2(inPhase, quadrature));
    float hueAngle = atan(quadrature, inPhase) + angle;
    inPhase = chroma * cos(hueAngle);
    quadrature = chroma * sin(hueAngle);
    return vec3(
        luminance + 0.956 * inPhase + 0.621 * quadrature,
        luminance - 0.272 * inPhase - 0.647 * quadrature,
        luminance - 1.106 * inPhase + 1.703 * quadrature);
}

void main()
{
    vec4 source = texture2D(currentTexture, gl_TexCoord[0].xy);
    vec2 screenPosition = vec2(
        gl_FragCoord.x,
        windowHeight - gl_FragCoord.y);
    vec2 terrainUv = (screenPosition - projectionOrigin) / projectionSize;
    float terrainHeight = texture2D(
        terrainTexture,
        clamp(terrainUv, vec2(0.0), vec2(1.0))).r;
    float terrainMask = step(0.001, terrainHeight)
        * (1.0 - step(0.999, terrainHeight));

    vec3 color = max(source.rgb, vec3(0.0));
    color *= exp2(exposure);
    color.r += max(temperature, 0.0) * 0.16;
    color.g += abs(temperature) * 0.015;
    color.b += max(-temperature, 0.0) * 0.16;
    color.r -= max(-temperature, 0.0) * 0.07;
    color.b -= max(temperature, 0.0) * 0.07;
    color = rotateHue(color, radians(hue));
    float luminance = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luminance), color, saturation);
    color = (color - vec3(0.5)) * contrast + vec3(0.5);
    color += vec3(brightness);
    color = pow(max(color, vec3(0.0)), vec3(1.0 / max(gamma, 0.001)));
    color = clamp(color, 0.0, 1.0);

    gl_FragColor = vec4(mix(source.rgb, color, terrainMask), source.a);
}
