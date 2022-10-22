#pragma once

#include "sphere.h"
#include "triangle.h"
#include "material.h"

struct Primitive {
public:
    enum class Type {
        Sphere,
        Triangle
    };
public:
    Type type = Type::Sphere; // 0: sphere 1: triangle
    int shapeIdx = 0;         // used in shader
    int materialIdx = 0;
    union {
        Sphere* sphere;
        Triangle* triangle;
    };

public:
    Primitive() : sphere(nullptr) {}
    
    Primitive(Type type, int shapeIdx, int materialIdx, Sphere* ptr) :
        type(type),
        shapeIdx(shapeIdx),
        materialIdx(materialIdx),
        sphere(ptr) {}

    Primitive(Type type, int shapeIdx, int materialIdx, Triangle* ptr) :
        type(type), 
        shapeIdx(shapeIdx), 
        materialIdx(materialIdx),
        triangle(ptr) {}

    static constexpr int getTexDataComponent() noexcept {
        return 3;
    }
};

struct ShaderPrimitive {
    int shapeType;
    int shapeIdx;
    int materialIdx;
};