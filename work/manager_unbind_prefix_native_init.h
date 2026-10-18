#pragma once
static const unsigned char managerUnbindPrefix0[]={85,139,236,81,83,139,93,8,86,87,137,77,252,131,251,255,117,18,106,0,106,0,232,69,252,255,255,95,94,91,139,229,93,194,4,0};
static const unsigned char managerUnbindPrefix1[]={85,139,236,81,83,139,93,8,86,87,139,249,137,125,252,131,251,255,117,18,106,0,106,0,232,67,252,255,255,95,94,91,139,229,93,194,4,0};
struct ManagerUnbindPrefixBody{const unsigned char* bytes;unsigned size,fileOffset,callOffset,target;};
static const ManagerUnbindPrefixBody managerUnbindPrefixBodies[]={{managerUnbindPrefix0,36,210224,22,268647824},{managerUnbindPrefix1,38,210416,24,268648016}};
