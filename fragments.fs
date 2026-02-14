#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform float saturation;

void main()
{
    vec4 texel = texture(texture0, fragTexCoord);

    // Convert to grayscale
    float gray = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    vec3 grayColor = vec3(gray);

    // Blend grayscale with original
    texel.rgb = mix(grayColor, texel.rgb, saturation);

    finalColor = texel * fragColor * 0.85;
}

