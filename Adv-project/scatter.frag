#version 420 core

in vec3 vWorldNormal;
in vec2 vTexCoord;
flat in int vTerrainAllowed;

out vec4 fragmentColor;

uniform bool has_color_texture;
uniform sampler2D color_texture;
uniform vec3 material_color;

void main()
{
    if (vTerrainAllowed == 0)
    {
        discard;
    }

    vec3 baseColor = material_color;
    float alpha = 1.0;
    if (has_color_texture)
    {
        vec4 texel = texture(color_texture, vTexCoord);
        if (texel.a < 0.35)
        {
            discard;
        }
        baseColor *= texel.rgb;
        alpha = texel.a;
    }

    vec3 normal = normalize(vWorldNormal);
    vec3 lightDir = normalize(vec3(-0.4, 1.0, -0.3));
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 color = baseColor * (0.25 + 0.75 * diffuse);

    fragmentColor = vec4(color, alpha);
}
