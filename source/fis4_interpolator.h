#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <chrono>

/**
 * FIS4 Frame Interpolation Core
 * Goal: Duplicate FPS via frame interpolation (X2)
 * - Input: Native rendered frames at N FPS (e.g., 30)
 * - Output: Interpolated stream at 2*N FPS (e.g., 60)
 * 
 * Current implementation:
 *   - Motion-vector style linear interpolation (lerp + extrapolation)
 *   - Ghost-reduction blending
 *   - Future: Replace with neural inference (ONNX/TensorRT-lite style)
 * 
 * Architecture hook for AI:
 *   See FIS4::Interpolator::setAIModel() - will dispatch to NN when model loaded.
 */

namespace FIS4 {

struct Vec2 {
    float x, y;
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y};}
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y};}
    Vec2 operator*(float s) const { return {x*s, y*s};}
    Vec2 lerp(const Vec2& o, float t) const { return {x + (o.x - x)*t, y + (o.y - y)*t};}
};

struct ObjectState {
    Vec2 pos;
    Vec2 vel;
    float angle = 0.f;
    float scale = 1.f;
    uint64_t frameId = 0;
    double timestampMs = 0;
};

struct InterpFrame {
    ObjectState state;
    float alpha = 0.5f;      // blend factor
    bool isInterpolated = false;
    bool isGhost = false;    // for debug viz
};

enum class InterpMode {
    OFF,        // native only
    CLASSIC,    // linear motion compensation (current)
    AI,         // neural path (placeholder)
    HYBRID      // classic fallback + AI refinement
};

enum class BaseFPS {
    FPS_30 = 30,
    FPS_60 = 60
};

class Interpolator {
public:
    Interpolator();

    // Config
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    void setBaseFPS(BaseFPS fps) { baseFPS_ = fps; }
    BaseFPS getBaseFPS() const { return baseFPS_; }

    void setMode(InterpMode m) { mode_ = m; }
    InterpMode getMode() const { return mode_; }

    void setMultiplier(int x) { multiplier_ = (x == 2 ? 2 : 1); }
    int getMultiplier() const { return multiplier_; }

    // Main API: push native frame, get interpolated history
    void pushFrame(const ObjectState& current, double nowMs);
    InterpFrame getInterpolated(float t) const; // t in [0,1] between prev and curr

    // Stats
    double getEffectiveFPS() const; // base*multiplier if enabled
    float getLastBlendAlpha() const { return lastAlpha_; }
    uint64_t getGeneratedFrames() const { return generatedFrames_; }
    uint64_t getNativeFrames() const { return nativeFrames_; }

    // Future AI hook
    struct AIModelInfo {
        bool loaded = false;
        const char* name = "FIS4-NN v0 (placeholder)";
        float inferenceMs = 0.f;
    };
    AIModelInfo getAIModelInfo() const { return aiInfo_; }
    bool loadAIModel(const char* path); // stub, returns false until model exists

    void reset();

private:
    bool enabled_ = true; // FIS4 ON by default to showcase X2
    BaseFPS baseFPS_ = BaseFPS::FPS_30;
    InterpMode mode_ = InterpMode::CLASSIC;
    int multiplier_ = 2;

    ObjectState prev_;
    ObjectState curr_;
    bool hasPrev_ = false;
    float lastAlpha_ = 0.f;
    uint64_t generatedFrames_ = 0;
    uint64_t nativeFrames_ = 0;

    AIModelInfo aiInfo_;

    ObjectState extrapolate(const ObjectState& s, float dt) const;
    ObjectState interpolateClassic(const ObjectState& a, const ObjectState& b, float t) const;
    ObjectState interpolateAI(const ObjectState& a, const ObjectState& b, float t) const; // placeholder delegates to classic
};

} // namespace FIS4
