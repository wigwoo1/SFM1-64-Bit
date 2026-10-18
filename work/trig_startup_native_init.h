#pragma once
static const unsigned char trigStartupBytes0[]={86,51,246,235,11,141,164,36,0,0,0,0,141,100,36,0,15,87,192,242,15,42,198,242,15,89,5,200,200,15,16,242,15,89,5,192,200,15,16,242,15,89,5,104,36,21,16,232,39,254,0,0,242,15,90,192,243,15,17,4,181,224,134,25,16,70,129,254,0,1,0,0,124,198,94,195};
static const unsigned trigStartupFixups0[][2]={{27,269469896},{35,269469888},{43,269821032},{61,270108384}};
static const unsigned trigStartupCalls0[][2]={{47,269374107}};
struct TrigStartupBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*fixups)[2];unsigned fixupCount;const unsigned (*calls)[2];unsigned callCount;};
static const TrigStartupBody trigStartupBodies[]={{trigStartupBytes0,76,870464,trigStartupFixups0,4,trigStartupCalls0,1}};
static const double trigonometricConstants[]={2,3.1415926535897931,0.00390625};
