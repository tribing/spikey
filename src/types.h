// src/types.h
#pragma once

// Must live in a header so PlatformIO's auto-prototype injector
// can see SpinoPose before it generates void drawSpikey(..., SpinoPose, ...) 
enum SpinoPose {
    POSE_WALK0 = 0, POSE_WALK1, POSE_WALK2, POSE_WALK3, POSE_WALK4,
    POSE_EAT1,      POSE_EAT2,  POSE_EAT3,  POSE_EAT4,
    POSE_DRINK1,    POSE_DRINK2, POSE_DRINK3, POSE_DRINK4,
    POSE_JUMP,
    POSE_SLEEP1, POSE_SLEEP2,
    POSE_DEAD
};