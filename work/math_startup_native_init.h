#pragma once
static const unsigned char mathStartupBytes0[]={85,139,236,128,61,224,138,25,16,0,117,100,255,21,252,161,15,16,138,64,4,168,8,116,4,168,16,117,14,104,124,36,21,16,255,21,60,162,15,16,131,196,4,198,5,224,138,25,16,1,232,233,254,255,255,139,69,20,243,15,16,69,16,80,131,236,12,243,15,17,68,36,8,243,15,16,69,12,243,15,17,68,36,4,243,15,16,69,8,243,15,17,4,36,232,157,250,255,255,104,38,89,65,49,232,35,9,0,0,131,196,20,93,195};
static const unsigned mathStartupFixups0[][2]={{5,270109408},{14,269459964},{30,269821052},{36,269460028},{45,270109408}};
static const unsigned mathStartupCalls0[][2]={{50,269308992},{94,269307936},{104,269311664}};
struct MathStartupBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*fixups)[2];unsigned fixupCount;const unsigned (*calls)[2];unsigned callCount;};
static const MathStartupBody mathStartupBodies[]={{mathStartupBytes0,114,870688,mathStartupFixups0,5,mathStartupCalls0,3}};
static const char mathStartupDiagnostic[]="SSE and SSE2 are required.";
