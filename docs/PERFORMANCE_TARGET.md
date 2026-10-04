# Performance Target

High fidelity only matters if the battle remains responsive.

## Initial target hardware

The first optimization target is a mainstream Windows gaming PC rather than the developer's current non-GPU laptop.

Primary acceptance target:

- 1920×1080;
- 60 fps gameplay target;
- <= 16.7 ms average frame time;
- no recurring frame-time spikes above 33 ms during normal infantry combat;
- scalable settings that preserve the combat simulation when visual quality is reduced.

## Budgets to measure

- game thread;
- render thread;
- GPU;
- active AI count;
- skeletal mesh update cost;
- animation evaluation;
- shadow cost;
- VFX count;
- draw calls;
- memory and streaming;
- physics;
- navigation.

## Rule

Do not optimize from intuition alone. Capture Unreal Insights traces and GPU profiles before introducing complexity such as pooling, reduced tick rates, or aggressive significance culling.
