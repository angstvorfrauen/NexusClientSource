#pragma once
#include "../../math/math.hpp"
#include "../../imgui/imgui.h"

enum BONEINDEX : int {
    ROOT = 0,
    PELVIS = 1,
    SPINE1 = 2,
    SPINE2 = 4,
    CHEST = 12,
    NECK = 6,
    HEAD = 7,
    SHOULDER_L = 9,
    ELBOW_L = 10,
    HAND_L = 11,
    SHOULDER_R = 13,
    ELBOW_R = 14,
    HAND_R = 15,
    HIP_L = 17,
    KNEE_L = 18,
    FOOT_HEEL_L = 19,
    HIP_R = 20,
    KNEE_R = 21,
    FOOT_HEEL_R = 22
};

struct BoneJointData {
    Vector3 Pos;
    char pad[0x14];
};

struct BoneJointPos {
    Vector3 Pos;
    Vector2 ScreenPos;
};

struct bone_link {
    int a, b;
};

static const bone_link SKELETON_LINKS[] = {
    {PELVIS, SPINE1}, {SPINE1, SPINE2}, {SPINE2, CHEST}, {CHEST, NECK}, {NECK, HEAD},
    {NECK, SHOULDER_L}, {SHOULDER_L, ELBOW_L}, {ELBOW_L, HAND_L},
    {NECK, SHOULDER_R}, {SHOULDER_R, ELBOW_R}, {ELBOW_R, HAND_R},
    {PELVIS, HIP_L}, {HIP_L, KNEE_L}, {KNEE_L, FOOT_HEEL_L},
    {PELVIS, HIP_R}, {HIP_R, KNEE_R}, {KNEE_R, FOOT_HEEL_R}
};