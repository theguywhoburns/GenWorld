#include <GenWorld/Utils/Time.h>

namespace Utils {
float Time::deltaTime = 0.0f;
float Time::time = 0.0f;
float Time::frameRate = 0.0f;
int Time::frameCount = 0;
float Time::lastTime = 0.0f;
float Time::lastFrameRateTime = 0.0f;

void Time::GetTime() { time = static_cast<float>(glfwGetTime()); }

void Time::CalculateDeltaTime() {
  deltaTime = time - lastTime;
  lastTime = time;
}

void Time::CalculateFrameRate() {
  frameCount++;
  if (time - lastFrameRateTime >= 1.0f) {
    frameRate = static_cast<float>(frameCount) / (time - lastFrameRateTime);
    lastFrameRateTime = time;
    frameCount = 0;
  }
}
} // namespace Utils