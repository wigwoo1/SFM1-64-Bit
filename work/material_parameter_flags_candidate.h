#pragma once
#include <string>
#include <vector>
// Functional flag identifiers in their original bit order. This is the
// material flag-name interface, not a shader-specific parameter/default table.
inline std::vector<std::string> materialParameterFlagNamesCandidate(){
    return {"$debug","$no_fullbright","$no_draw","$use_in_fillrate_mode",
        "$vertexcolor","$vertexalpha","$selfillum","$additive","$alphatest",
        "$xxxxxunusedxxxxx","$znearer","$model","$flat","$nocull","$nofog",
        "$ignorez","$decal","$envmapsphere","$xxxxxxunusedxxxxx",
        "$envmapcameraspace","$basealphaenvmapmask","$translucent",
        "$normalmapalphaenvmapmask","$softwareskin","$opaquetexture",
        "$envmapmode","$nodecal","$halflambert","$wireframe",
        "$allowalphatocoverage","$alphamodifiedbyproxy_DO_NOT_SET_IN_VMT","$vertexfog"};
}
