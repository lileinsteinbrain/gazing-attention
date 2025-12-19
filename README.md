## What this is
An HCI system that models observation as a measurable, time-dependent force, translating accumulated perceptual pressure into state-based physical response.

## Input
- Visual observation from a camera
- Distance and proximity sensing
- Ambient light intensity

## Processing
- Perceptual inputs are normalized and aggregated over time
- Observation is treated as continuous pressure rather than discrete events
- An internal system state evolves through accumulation, weighting, and uncertainty
- State transitions are resolved through a finite set of regimes rather than linear scaling

## Output
- Mechanical rotation and vibration mounted on the mask
- Physical responses express internal state resolution rather than feedback signals

## What currently works
- Sustained observation produces different system behavior than brief or transient interaction
- Changes in observation pressure lead to distinct state transitions
- Physical response makes internal system tension perceptible at the boundary of the body

## What’s unfinished
- Formal parameterization of observation pressure and state transition thresholds
- Extended testing of how users interpret state-based physical response over time
- The wearable mask itself functions as a prototype output, but its current physical implementation requires refinement and stabilization

## Why it matters
WhiteMask reframes perception as an accumulative, time-based computational state, challenging event-driven models of sensing and interaction in wearable systems.
