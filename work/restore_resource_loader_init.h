struct ResourceBody {unsigned file,offset,size;};
static const ResourceBody resourceBodies[]={{102272,0,563},{222000,576,15}};
struct ResourceCall {unsigned operand,next,target;};
static const ResourceCall resourceCalls[]={{77,81,4096},{181,185,576},{210,214,4096},{256,260,4096},{391,395,4096},{475,479,4096},{535,539,4096}};
struct ResourceFixup {unsigned operand,original,binding;};
static const ResourceFixup resourceFixups[]={{4,0x10183c64,0},{22,0x100fc91c,1},{28,0x100fa244,2},{53,0x10183ab4,3},{120,0x10183ab4,3},{162,0x10183b5c,4},{295,0x1016d130,5},{585,0x1016e5d4,6}};
static const unsigned resourceBindingCount=7;
static const char* resourceBindingNames[]={"diagnostic_convar","diagnostic_string","diagnostic_import_cell","padding_convar","format_device_cell","mode_service_cell","format_table"};
