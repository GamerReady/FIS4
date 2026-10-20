#pragma once
#include "fis4_interpolator.h"
#include <vector>
#include <array>
#include <cmath>

namespace FIS4 {

// Simple demo objects that move deterministically so FPS differences are obvious

struct Ball {
    FIS4::Vec2 pos;
    FIS4::Vec2 vel;
    float radius = 24.f;
    float hue = 0.f; // for color
    float trailPhase = 0.f;
    void update(float dt, int boundsW, int boundsH);
    FIS4::ObjectState toState(double ts, uint64_t fid) const;
};

struct RotatingBar {
    FIS4::Vec2 center;
    float length = 300.f;
    float thickness = 14.f;
    float angle = 0.f;
    float angularVel = 90.f; // deg/s
    void update(float dt);
    FIS4::ObjectState toState(double ts, uint64_t fid) const;
};

struct Particle {
    FIS4::Vec2 pos, vel;
    float life = 1.f;
    float size = 4.f;
};

enum class SceneID { BOUNCING_BALLS = 0, ROTATING_GRID = 1, TUNNEL = 2, STRESS = 3, COUNT = 4 };

class DemoScene {
public:
    DemoScene(int w = 1920, int h = 1080);
    void setSize(int w, int h) { W=w; H=h; reset(); }
    void reset();
    void nextScene();
    void prevScene();
    void setScene(SceneID id) { scene=id; reset(); }
    SceneID getScene() const { return scene; }
    SceneID getSceneID() const { return scene; }
    const char* getSceneName() const;

    // Called at native tick rate (e.g., 30Hz)
    void update(float dt, double nowMs, uint64_t frameId);

    // Called at display rate (may be interpolated)
    struct RenderState {
        std::vector<Ball> balls;
        RotatingBar bar;
        std::vector<Particle> particles;
        Vec2 tunnelOffset{0,0};
        float gridPhase = 0.f;
    };
    RenderState getNativeState() const { return native_; }
    RenderState getInterpolated(float t) const; // t in [0,1]

    // For stats overlay
    Vec2 getPrimaryPos() const;
    Vec2 getPrimaryVel() const;

private:
    int W, H;
    SceneID scene = SceneID::BOUNCING_BALLS;
    RenderState native_, prevNative_;
    bool hasPrev = false;

    void updateBouncingBalls(float dt);
    void updateRotatingGrid(float dt);
    void updateTunnel(float dt);
    void updateStress(float dt);
};

} // namespace FIS4
