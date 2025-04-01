#pragma once

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void init() = 0;
    virtual void shutdown() = 0;

    virtual void preRender() = 0;
    virtual void render() = 0;
    virtual void postRender() = 0;
};