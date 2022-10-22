#pragma once 

#include <random>
#include <glm/glm.hpp>

inline float randomFloat() {
    // Returns a random real in [0, 1).
    static std::default_random_engine e;
    static std::uniform_real_distribution<float> u(0.0f, 1.0f);

    return u(e);
}

inline float randomFloat(float minVal, float maxVal) {
    // Returns a random real in [min,max).
    return minVal + (maxVal - minVal) * randomFloat();
}

inline glm::vec3 randomVec3() {
    return glm::vec3(randomFloat(), randomFloat(), randomFloat());
}

inline glm::vec3 randomVec3(float minVal, float maxVal) {
    return glm::vec3(
        randomFloat(minVal, maxVal),
        randomFloat(minVal, maxVal),
        randomFloat(minVal, maxVal));
}