#version 420 core

in vec2 vTexCoord;
flat in int vTerrainAllowed;

uniform bool scatterShadowMode = false;
uniform bool has_color_texture;
uniform sampler2D color_texture;

void main()
{
    if (vTerrainAllowed == 0)
    {
        discard;
    }

    if (scatterShadowMode && has_color_texture)
    {
        vec4 texel = texture(color_texture, vTexCoord);
        if (texel.a < 0.35)
        {
            discard;
        }
    }
}
