#pragma once
static const unsigned char resourceStartupBytes0[]={131,61,76,9,25,16,0,116,21,128,61,253,17,27,16,0,117,12,232,153,0,0,0,198,5,253,17,27,16,1,139,13,4,9,25,16,133,201,116,33,128,61,252,17,27,16,0,117,24,139,1,139,80,32,104,0,18,27,16,255,210,198,5,252,17,27,16,1,233,55,0,0,0,195};
static const unsigned resourceStartupFixups0[][2]={{2,270076236},{11,270209533},{25,270209533},{32,270076164},{42,270209532},{55,270209536},{63,270209532}};
static const unsigned resourceStartupCalls0[][2]={{18,269344816},{68,269344768}};
static const unsigned char resourceStartupBytes1[]={86,139,53,44,18,27,16,133,246,116,26,235,3,141,73,0,139,13,4,9,25,16,139,1,139,80,48,86,255,210,139,118,12,133,246,117,235,94,195};
static const unsigned resourceStartupFixups1[][2]={{3,270209580},{18,270076164}};
static const unsigned resourceStartupCalls1[][2]={{0,0}};
static const unsigned char resourceStartupBytes2[]={86,51,246,235,11,141,164,36,0,0,0,0,141,100,36,0,139,13,76,9,25,16,139,1,139,20,181,76,18,27,16,139,64,40,86,82,255,208,70,131,254,5,124,228,94,195};
static const unsigned resourceStartupFixups2[][2]={{18,270076236},{27,270209612}};
static const unsigned resourceStartupCalls2[][2]={{0,0}};
struct ResourceStartupBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*fixups)[2];unsigned fixupCount;const unsigned (*calls)[2];unsigned callCount;};
static const ResourceStartupBody resourceStartupBodies[]={{resourceStartupBytes0,74,906112,resourceStartupFixups0,7,resourceStartupCalls0,2},{resourceStartupBytes1,39,906240,resourceStartupFixups1,2,resourceStartupCalls1,0},{resourceStartupBytes2,46,906288,resourceStartupFixups2,2,resourceStartupCalls2,0}};
static const unsigned resourceStartupAddressMap[][2]={{0x100ddf80,0},{0x100de000,512},{0x100de030,1024}};
