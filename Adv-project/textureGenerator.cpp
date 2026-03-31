#include "textureGenerator.hpp"
#include <fstream>
#include <list>

std::vector<vec2> getNeighbours(TerrainSquare& square){
    std::vector<vec2> neighbors;
    vec2 offsets[8] = {
        vec2(-1, -1), vec2(0, -1), vec2(1, -1),
        vec2(-1, 0),              vec2(1, 0),
        vec2(-1, 1),  vec2(0, 1),  vec2(1, 1)
    };

    for (int i = 0; i < 8; ++i) {
        neighbors.push_back(square.center + offsets[i] * vec2(SQUARE_SIZE));
    }
    return neighbors;
}

bool heightmapExists(vec2 center){
    std::string filename = "heightmaps/" + std::to_string((int)center.x) + "_" + std::to_string((int)center.y) + ".png";
    std::ifstream file(filename);
    if (file.good()) {
        file.close();
        return true;
    }
    return false;
}

void generateHeightmap(const char* heightmapPath, TerrainSquare& square){
    // First we check if any of the 8 neighboring squares have a heightmap that we need to take into consideration when generating this one.

    // This map shows the 8 neighboring squares around the current square, with the current square in the middle (x):
    // 0 1 2
    // 3 x 4
    // 5 6 7

    std::vector<vec2> neighbors = getNeighbours(square);
    std::list<vec2> neighborsToConsider;

    for (const auto& neighbor : neighbors) {
        if (heightmapExists(neighbor)) {
            neighborsToConsider.push_back(neighbor);
        }
    }

    // Here we need some code that can look at the neighborsToConsider and generate a heightmap for the current square that takes into account the heightmaps of the neighbors.

    }