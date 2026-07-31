#version 130

uniform vec2 ballCenter;
uniform float ballRadius;
uniform vec4 baseColor;
uniform float rotation;
uniform vec2 movementDirection;
uniform float movementAmount;
uniform int lavaMode;
uniform float u_time;

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

void main()
{
    vec2 point = (gl_FragCoord.xy - ballCenter) / max(ballRadius, 1.0);
    float radiusSquared = dot(point, point);
    if (radiusSquared > 1.0)
    {
        discard;
    }

    float angle = radians(rotation);
    mat2 rotationMatrix = mat2(
        cos(angle), -sin(angle),
        sin(angle), cos(angle));
    vec2 materialPoint = rotationMatrix * point;

    float sphereHeight = sqrt(max(0.0, 1.0 - radiusSquared));
    float surface = noise(materialPoint * 4.2 + vec2(9.1, 3.7));
    float surfaceX = noise((materialPoint + vec2(0.018, 0.0)) * 4.2 + vec2(9.1, 3.7)) - surface;
    float surfaceY = noise((materialPoint + vec2(0.0, 0.018)) * 4.2 + vec2(9.1, 3.7)) - surface;
    vec3 normal = normalize(vec3(
        point.x - surfaceX * 0.42,
        point.y - surfaceY * 0.42,
        sphereHeight));

    vec3 lightDirection = normalize(vec3(-0.48, 0.58, 0.78));
    vec3 viewDirection = vec3(0.0, 0.0, 1.0);
    vec3 halfDirection = normalize(lightDirection + viewDirection);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float highlight = pow(max(dot(normal, halfDirection), 0.0), 64.0);
    float broadHighlight = pow(max(dot(normal, halfDirection), 0.0), 14.0);
    float edge = pow(1.0 - sphereHeight, 2.2);

    vec3 color;
    if (lavaMode != 0)
    {
        float rockNoise = noise(materialPoint * 5.2 + vec2(4.3, 12.7));
        rockNoise += noise(materialPoint * 10.4 + vec2(17.1, 2.6)) * 0.35;
        float crackDistance = abs(fract(rockNoise * 1.45) - 0.5);
        float cracks = 1.0 - smoothstep(0.025, 0.095, crackDistance);
        float flicker = 0.88 + sin(u_time * 4.2 + rockNoise * 8.0) * 0.12;
        vec3 rock = mix(vec3(0.055, 0.020, 0.014), vec3(0.23, 0.055, 0.018), rockNoise);
        rock *= 0.34 + diffuse * 0.72;
        vec3 molten = mix(vec3(1.0, 0.095, 0.006), vec3(1.0, 0.82, 0.12), cracks);
        color = mix(rock, molten * flicker, cracks);
        color += molten * cracks * 0.55;
        color += vec3(1.0, 0.22, 0.02) * broadHighlight * 0.18;
    }
    else
    {
        float materialVariation = noise(materialPoint * 3.4 + vec2(2.8, 15.6));
        float softBand = sin(
            materialPoint.x * 8.0
            + materialPoint.y * 2.2
            + materialVariation * 2.4) * 0.5 + 0.5;
        vec3 enamel = baseColor.rgb * (0.90 + materialVariation * 0.10 + softBand * 0.035);
        color = enamel * (0.24 + diffuse * 0.82);

        vec2 direction = normalize(movementDirection);
        float alongDirection = dot(point, direction);
        float acrossDirection = abs(point.x * direction.y - point.y * direction.x);
        float directionLine = (1.0 - smoothstep(0.045, 0.105, acrossDirection))
            * smoothstep(-0.015, 0.045, alongDirection)
            * (1.0 - smoothstep(0.76, 0.90, alongDirection))
            * movementAmount;
        color = mix(color, baseColor.rgb * 0.075, directionLine * 0.92);

        float polishedHighlight = highlight * (1.0 - directionLine * 0.78);
        color += vec3(1.0, 0.96, 0.90)
            * (polishedHighlight * 0.88 + broadHighlight * 0.12);
        color = mix(color, color * vec3(0.36, 0.48, 0.58), edge * 0.62);
        color += vec3(0.16, 0.28, 0.42) * edge * 0.16;
    }

    float antialias = 1.0 - smoothstep(0.965, 1.0, radiusSquared);
    gl_FragColor = vec4(clamp(color, 0.0, 1.0), antialias);
}
