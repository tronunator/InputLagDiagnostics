# Input Lag Diagnostics Plugin

## Overview
A plugin for Unreal Tournament 4 that measures the time from Windows input arrival to when the frame with that input's visual effect is ready for display.


In console:
```
ShowInputLag
```

## Architecture

### Plugin Structure
```
InputLagDiagnostics/
├── Source/InputLagDiagnostics/
│   ├── Public/
│   │   ├── InputLagPlayerController.h    # Custom PlayerController
│   │   ├── InputLagHUD.h                 # Custom HUD for display
│   │   └── InputLagDiagnosticsMutator.h  # Mutator to swap classes
│   └── Private/
│       └── [Implementation files]
└── Content/
    └── BP_InputLagDiagnosticsMutator.uasset  # Blueprint mutator
```

### Key Classes

**AInputLagPlayerController** - Captures input timestamps and stores measurements
- `LastInputTimestamp` - Time when input arrived (FPlatformTime::Seconds)
- `InputFrameNumber` - Frame when input was recorded (GFrameCounter)
- `bPendingInputMeasurement` - Flag to track measurement state
- `InputLagHistory` - Circular buffer (200 samples)

**AInputLagHUD** - Displays diagnostics and finalizes measurements
- Calls `FinalizeInputLagMeasurement()` in `DrawHUD()` (end of frame)
- Renders overlay with current/average/last values

**AInputLagDiagnosticsMutator** - Swaps default classes
- Replaces `AUTPlayerController` with `AInputLagPlayerController`
- Replaces `AUTHUD` with `AInputLagHUD`

## How It Works

### Frame-Based Measurement
```
Frame N (Input arrives):
  1. Mouse moves → InputAxis() called
  2. RecordInputTimestamp():
     - LastInputTimestamp = FPlatformTime::Seconds()
     - InputFrameNumber = GFrameCounter  (e.g., 100)
     - bPendingInputMeasurement = true
  3. Game processes input, rendering happens
  4. DrawHUD() → MeasureInputLagEndOfFrame()
     - Check: GFrameCounter (100) <= InputFrameNumber (100)?
     - YES → Skip (same frame)

Frame N+1 (Visual result visible):
  1. Game logic and rendering complete
  2. DrawHUD() → MeasureInputLagEndOfFrame()
     - Check: GFrameCounter (101) > InputFrameNumber (100)?
     - YES → Measure now!
     - Lag = (CurrentTime - LastInputTimestamp) * 1000ms
```

### Tracked Inputs
- **Mouse X/Y** - Camera movement (continuous)
- **Left/Right Mouse Button** - Fire actions
- One input measured at a time (by design)

## Display

HUD shows (top-right corner):
```
Last: 16.7ms  (Color-coded: Green <10ms, Yellow 10-20ms, Red >20ms)
Avg:  18.2ms
Cur:  MouseX
```
