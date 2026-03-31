#version 410 core

// ----------------------------------------------------------------------------
// Varyings from vertex shader
// ----------------------------------------------------------------------------
in vec2 vTexcoord;
in vec3 vWorldPos;
in vec3 vWorldNormal;

// ----------------------------------------------------------------------------
// Output color
// ----------------------------------------------------------------------------
out vec4 FragColor;

// ----------------------------------------------------------------------------
// Uniforms: Textures
// ----------------------------------------------------------------------------
uniform sampler2D earthTex;  // Color texture (daytime map)

// ----------------------------------------------------------------------------
// Uniforms: Lighting
// ----------------------------------------------------------------------------
uniform vec3 lightDirWorld;   // Direction *toward* the light source (sun)
uniform vec3 cameraPosWorld;  // Camera origin in world space
uniform float lightIntensity;


void main()
{
    // ============================================================================
    // BASE COLOR SAMPLE (NOT SHADING)
    // ============================================================================
    vec3 baseColor = texture(earthTex, vTexcoord).rgb;

    // ============================================================================
    // LIGHTING / SHADING COMPUTATION
    // ============================================================================

    // Normalize shading vectors
    vec3 N = normalize(vWorldNormal);            // Surface normal
    vec3 L = normalize(lightDirWorld);           // Light direction
    vec3 V = normalize(cameraPosWorld - vWorldPos); // View direction
    vec3 H = normalize(L + V);                   // Blinn-Phong half vector

    // Diffuse term (Lambertian)
    float NdotL = max(dot(N, L), 0.0);

    // Ambient light contribution
    float ambient = 0.10;

    // Diffuse contribution (sunlight on the day side)
    float diffuse = NdotL * lightIntensity;

    // Specular contribution (small highlight on shiny areas)
    float specular = 0.0;
    if (NdotL > 0.0)
    {
        float NdotH = max(dot(N, H), 0.0);
        float shininess = 32.0;
        specular = pow(NdotH, shininess);
    }

    // Final lighting result
    vec3 lighting =
        baseColor * (ambient + diffuse) +     // Ambient + diffuse color
        vec3(1.0) * specular * 0.2 * lightIntensity;           // Small white specular

    // ----------------------------------------------------------------------------
    // Output
    // ----------------------------------------------------------------------------
    FragColor = vec4(lighting, 1.0);
}
