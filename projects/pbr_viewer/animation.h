#pragma once

#include <limits>
#include <string>
#include <vector>

#include "node.h"

struct Animation {
    struct Channel {
        enum class PathType {
            Translation,
            Rotation,
            Scale
        };

        PathType path;
        Node* node;
        uint32_t samplerIndex;
    };

    struct Sampler {
        enum class InterpolationType {
            LINEAR,
            STEP,
            CUBICSPLINE
        };
        InterpolationType interpolation;
        std::vector<float> inputs;
        std::vector<glm::vec4> outputs;
    };

    std::string name;
    std::vector<Sampler> samplers;
    std::vector<Channel> channles;
    float start = std::numeric_limits<float>::max();
    float end = std::numeric_limits<float>::min();
};