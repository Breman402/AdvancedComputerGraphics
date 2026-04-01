#include "materials.hpp"

Material waterMaterial(
    glm::vec3(0.0f, 0.3f, 0.5f), // color
    0.964f, // metalness
    0.00f, // fresnel
    256.0f, // shininess
    glm::vec3(0.1f, 0.1f, 0.2f) // emission
);
Material sandMaterial(
    glm::vec3(0.76f, 0.70f, 0.50f), // color
    0.516f, // metalness
    0.013f, // fresnel
    43.202f, // shininess
    glm::vec3(0.0f) // emission
);
Material grassMaterial(
    glm::vec3(0.1f, 0.6f, 0.1f), // color
    0.0f, // metalness
    0.0f, // fresnel
    32.0f, // shininess
    glm::vec3(0.0f) // emission
);
Material rockMaterial(
    glm::vec3(0.5f, 0.5f, 0.5f), // color
    0.0f, // metalness
    0.0f, // fresnel
    8.24f, // shininess
    glm::vec3(0.0f) // emission
);
Material snowMaterial(
    glm::vec3(0.9f, 0.9f, 0.9f), // color
    0.0f, // metalness
    0.369f, // fresnel
    1.0f, // shininess
    glm::vec3(0.0f) // emission
);

std::vector<Material*> materials = { &waterMaterial, &sandMaterial, &grassMaterial, &rockMaterial, &snowMaterial };
