#pragma once

#include "../Core/Shader.h"

class IDrawable
{
public:
    virtual void Draw(Shader& shader) = 0;
};
