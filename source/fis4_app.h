#pragma once
#include "fis4_interpolator.h"
#include "demo_scene.h"
#include <chrono>
#include <deque>

namespace FIS4 {

struct FrameStats {
    double instantFPS = 0;
    double avgFPS = 0;
    double frameTimeMs = 0;
    uint64_t totalFrames = 0;
    uint64_t nativeFrames = 0;
    uint64_t interpolatedFrames = 0;
};

class App {
public:
    App(int w=1920,int h=1080);
    void init();

    // Called from main loop
    void handleInput(uint64_t buttons, float stickX=0,float stickY=0); // PS4 pad bits
    void updateNative(float dt, double nowMs); // tick at base FPS
    DemoScene::RenderState getRenderState(float interpT); // for drawing (may be interpolated)

    // Mode switching
    void toggleFIS4() { interpolator_.setEnabled(!interpolator_.isEnabled()); }
    void cycleBaseFPS();
    void cycleInterpMode();
    bool isFIS4Enabled() const { return interpolator_.isEnabled(); }
    BaseFPS getBaseFPS() const { return interpolator_.getBaseFPS(); }
    InterpMode getInterpMode() const { return interpolator_.getMode(); }
    double getEffectiveFPS() const { return interpolator_.getEffectiveFPS(); }

    // Scene
    void nextScene() { scene_.nextScene(); }
    void prevScene() { scene_.prevScene(); }
    const char* getSceneName() const { return scene_.getSceneName(); }
    SceneID getSceneID() const { return scene_.getScene(); }

    // Pausing
    void setPaused(bool p){ paused_=p; }
    bool isPaused() const { return paused_; }

    // Stats
    void tickFrame(double nowMs);
    FrameStats getStats() const { return stats_; }
    const std::deque<float>& getFrameTimeHistory() const { return ftHistory_; }

    DemoScene& getScene() { return scene_; }
    Interpolator& getInterpolator() { return interpolator_; }

    // UI state
    bool showHelp = true;
    bool showGraph = true;

private:
    int W,H;
    DemoScene scene_;
    Interpolator interpolator_;
    FrameStats stats_;
    std::deque<float> ftHistory_; // last 120 frame times
    std::deque<double> fpsHistory_;
    double lastFrameMs = 0;
    bool paused_ = false;
    double accMs_ = 0;
};

} // namespace FIS4
