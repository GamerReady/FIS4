#include "fis4_interpolator.h"
#include <cmath>

namespace FIS4 {

Interpolator::Interpolator() { reset(); }

void Interpolator::reset() {
    hasPrev_ = false;
    generatedFrames_ = 0;
    nativeFrames_ = 0;
    lastAlpha_ = 0.f;
}

void Interpolator::pushFrame(const ObjectState& current, double nowMs) {
    (void)nowMs;
    if (hasPrev_) {
        prev_ = curr_;
    } else {
        // first frame - duplicate prev as curr to avoid snap
        prev_ = current;
        hasPrev_ = true;
    }
    curr_ = current;
    curr_.frameId = nativeFrames_;
    nativeFrames_++;
}

InterpFrame Interpolator::getInterpolated(float t) const {
    if (!enabled_ || !hasPrev_ || multiplier_ == 1) {
        return InterpFrame{curr_, 0.f, false, false};
    }
    // Clamp t
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;

    ObjectState out;
    if (mode_ == InterpMode::AI) out = interpolateAI(prev_, curr_, t);
    else if (mode_ == InterpMode::HYBRID) {
        auto c = interpolateClassic(prev_, curr_, t);
        auto ai = interpolateAI(prev_, curr_, t);
        // hybrid lerp 70% AI / 30% classic (tunable)
        out.pos = c.pos.lerp(ai.pos, 0.7f);
        out.angle = c.angle + (ai.angle - c.angle) * 0.7f;
        out.vel = c.vel;
    } else out = interpolateClassic(prev_, curr_, t);

    // Generate ghost detection: if velocity spike > 3x avg, mark ghost
    float velMag = sqrtf(out.vel.x*out.vel.x + out.vel.y*out.vel.y);
    float prevMag = sqrtf(prev_.vel.x*prev_.vel.x + prev_.vel.y*prev_.vel.y);
    bool ghost = (prevMag > 1.f && velMag > prevMag * 3.f);
    return InterpFrame{out, t, true, ghost};
}

ObjectState Interpolator::interpolateClassic(const ObjectState& a, const ObjectState& b, float t) const {
    ObjectState r;
    // Position: linear with velocity-aware correction
    Vec2 posLerp = a.pos.lerp(b.pos, t);
    // Velocity is averaged then scaled
    Vec2 velLerp = a.vel.lerp(b.vel, t);
    // Simple motion compensation: add small extrapolation along velocity to reduce judder
    float mc = 0.02f; // motion comp strength
    posLerp = posLerp + velLerp * (mc * (t - 0.5f));
    r.pos = posLerp;
    r.vel = velLerp;
    // Angle shortest path lerp
    float dAngle = b.angle - a.angle;
    while (dAngle > 180.f) dAngle -= 360.f;
    while (dAngle < -180.f) dAngle += 360.f;
    r.angle = a.angle + dAngle * t;
    r.scale = a.scale + (b.scale - a.scale) * t;
    r.frameId = b.frameId;
    return r;
}

ObjectState Interpolator::interpolateAI(const ObjectState& a, const ObjectState& b, float t) const {
    // Placeholder: In future, this would run:
    // - motion vector estimation (optical flow)
    // - depth-aware warping
    // - occlusion mask
    // For now, delegate to classic + small nonlinear tweak to simulate "AI smoothness"
    ObjectState c = interpolateClassic(a, b, t);
    // Add smoothstep for more natural ease (AI would learn this)
    float smooth = t*t*(3.f - 2.f*t);
    // Re-lerp position with smoothstep influence
    Vec2 linear = a.pos.lerp(b.pos, t);
    Vec2 smoothPos = a.pos.lerp(b.pos, smooth);
    c.pos = linear.lerp(smoothPos, 0.15f); // 15% smoothstep influence
    return c;
}

ObjectState Interpolator::extrapolate(const ObjectState& s, float dt) const {
    ObjectState r = s;
    r.pos = r.pos + r.vel * dt;
    r.angle += r.vel.x * 0.01f * dt;
    return r;
}

double Interpolator::getEffectiveFPS() const {
    double base = (baseFPS_ == BaseFPS::FPS_30 ? 30.0 : 60.0);
    if (enabled_ && multiplier_ == 2) return base * 2.0;
    return base;
}

bool Interpolator::loadAIModel(const char* path) {
    (void)path;
    // TODO: Load ONNX model via embedded inference
    // Path expected: /app0/assets/fis4_nn.onnx
    aiInfo_.loaded = false;
    aiInfo_.inferenceMs = 0.f;
    return false;
}

} // namespace FIS4
