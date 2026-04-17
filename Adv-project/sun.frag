#version 420 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vWorldNormal;

uniform vec3 cameraPosWorld;
uniform vec3 sunCoreColor;
uniform vec3 sunHaloColor;
uniform float sunIntensityScale;

void main()
{
    // Calculate view direction from fragment to camera
    vec3 viewDir = normalize(cameraPosWorld - vWorldPos);
    vec3 normal = normalize(vWorldNormal);

    // Calculate fresnel-like effect based on view direction
    float facing = max(dot(normal, viewDir), 0.0);
    
    // Create multiple layers with different falloff rates
    float body = pow(facing, 14.0);      // Main sun body glow
    float core = pow(facing, 140.0);     // Bright core highlight
    float halo = pow(facing, 3.5);       // Soft outer halo

    // Combine color layers with different intensities
    vec3 radiance = sunHaloColor * halo * 0.35 + sunCoreColor * (body * 1.10 + core * 1.80);

    // Apply tone mapping to prevent overexposure
    vec3 color = 1.0 - exp(-radiance * sunIntensityScale);
    
    // Calculate alpha blend based on body and halo visibility
    float alpha = clamp(body * 1.10 + halo * 0.12, 0.0, 1.0);

    FragColor = vec4(color, alpha);
}