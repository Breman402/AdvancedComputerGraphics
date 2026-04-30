#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <random>
#include <utility>

enum class TerrainType
{
    water,
    sand,
    gras,
    rock,
    snow
};

using namespace std;

struct ScatterInstance
{
    glm::vec2 worldPos;
    float scale;
    float rotation;
};

float random_between(const pair<const float, const float>& range) {
    static random_device rd;
    static mt19937 gen(rd());

    uniform_real_distribution<float> dist(range.first, range.second);
    return dist(gen);
}

class ScatterObject
{
    public:
  
    ScatterObject(pair<float, float> scale, float offset, pair<TerrainType, TerrainType> terrainRange, const string& modelPath){
        this->scale = scale;
        this->offset = offset;
        this->terrainRange = terrainRange;
        this->modelPath = modelPath;
    }

    void render(const GLuint& shaderProgram){

    }

    vector<ScatterInstance> generateInstances(){
        const float instanceScale = random_between(scale);
        const float instanceRotation = random_between({0.0f, 360.0f});
        const glm::vec2 instancePosition = glm::vec2(0.0f, 0.0f);

        ScatterInstance instance = {instancePosition, instanceScale, instanceRotation};
    }

    private:

    pair<float, float> scale; // Lower and upper bound scale
    float offset;
    string modelPath;
    float freq = 1.0f; // 100% freq per gridsize for a scatter obj to spawn in every gridsquare
    float grid = 1.0f; // The size of each grid cell
    pair<TerrainType, TerrainType> terrainRange; // The lowest and highest terrain that the scatter obj. will spawn at
};
