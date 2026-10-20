# FIS4 Interpolation Notes

This document explains how FIS4 doubles FPS and where the future neural model plugs in.

## Classic path (ships now)

`FIS4::Interpolator::interpolateClassic(prev,curr,t)`:

- Position: `lerp(prev.pos, curr.pos, t) + vel * 0.02*(t-0.5)` — the `vel*` term is motion compensation to avoid the “stair-step” when t is far from 0.5.
- Velocity: averaged `lerp(prev.vel,curr.vel,t)`
- Angle: shortest-path lerp (handles 359→0 wrap)
- Scale: linear

`DemoScene::getInterpolated(t)` does the same for the whole render state (all balls + bar). That’s what `Renderer` draws at display time.

## AI path (stub)

`interpolateAI()` currently:

```cpp
auto c = interpolateClassic(a,b,t);
float s = smoothstep(t); // t*t*(3-2*t)
c.pos = lerp( lerp(a.pos,b.pos,t), lerp(a.pos,b.pos,s), 0.15 );
```

This simulates what a tiny learned ease would do. Swap it with real inference:

```cpp
ObjectState Interpolator::interpolateAI(const ObjectState& a,const ObjectState& b,float t) const {
    // 1. Build motion vectors from a→b
    // 2. Run ONNX: input = [a.pos,b.pos,t,depth,occlusionMask]
    // 3. Output = warped pos + confidence
    // 4. If confidence < thresh, fallback to classic
    return runONNX(a,b,t);
}
```

## Scheduling

- Native tick: every `1000/baseFPS` ms (`30→33.3ms`, `60→16.6ms`) calls `App::updateNative()` which `pushFrame()`.
- Display tick: every `1000/effectiveFPS` ms (`effective=base*2` when FIS4 on). Between native ticks we compute `t = elapsed/nativeInterval` and call `getRenderState(t)`.
- Frame graph: `App::tickFrame()` records `frameTimeMs` into a 120-entry deque drawn by `Renderer::drawHUD()`.

## Hooking a real game (future)

1. Intercept the game’s `sceVideoOutSubmitFlip` or its render-present.
2. Instead of flipping native buffer directly, hold it, call `FIS4::Interpolator::pushFrame()` with game motion vectors (extract from game’s transform matrices).
3. Submit original + synthetic flip. Requires patching vsync — GoldHEN plugin will do that.

## Metrics to tune

- **Ghost metric:** `velMag > prevMag*3` → interpolate is likely wrong (fast occlusion). Clamp or drop.
- **Judder metric:** variance of `ftHistory` — FIS4 hides judder but not uneven pacing. Fix game pacing first.
- **Effective FPS:** `base * multiplier` — if FIS4 on and base=30, must hit 60 display budget or you’ll drop.

## Files to touch when swapping AI

- `source/fis4_interpolator.h` — extend `AIModelInfo` (input tensors)
- `source/fis4_interpolator.cpp` — `loadAIModel()` + `interpolateAI()`
- `sce_sys/assets/fis4_nn.onnx` — ship model (not in repo yet)

No renderer or app changes needed — the app already drives `t` and mode switching.
