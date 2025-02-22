#ifndef Renderer_h
#define Renderer_h

#include "Camera.h"
#include "Shader.h"

class Renderer {
private:
    // std::vector<Object> buffer;
    Camera camera;
    Shader shader;
    
public:
    void PushObject();
    void ClearBuffer();
    void RenderFrame();
    void SetCamera();

    Renderer(Camera camera);
    ~Renderer();
};

#endif