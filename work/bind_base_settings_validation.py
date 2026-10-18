from pathlib import Path
p=Path('work/test_dmx_editor_timing.cpp');s=p.read_text().replace('#include "attribute_float_candidate.h"','#include "attribute_float_candidate.h"\n#include "base_element_type_validation_candidate.h"')
s=s.replace('    // Serialized Elements represent graph objects. Native vslot4 inheritance\n    // validation remains a separate unresolved runtime binding.\n    bool acceptElementType(Element*){return true;}', '''    // Both retained settings nodes serialize as DmElement. Symbolic host
    // token represents base equality only; no original global token invented.
    bool acceptElementType(Element* e){
        if(g.strings.at(e->type)!="DmElement")throw std::runtime_error("derived graph type validation not recovered");
        constexpr std::uint32_t hostBaseToken=1;
        return baseElementAcceptsTypeCandidate(hostBaseToken,hostBaseToken);
    }''')
s=s.replace('        const auto fps=a.attributeFloat(rate);','''        const auto fps=a.attributeFloat(rate);
        assert(g.strings.at(render->type)=="DmElement");''')
s=s.replace('restored_graph_valid passed','base_DmElement_checked restored_graph_valid passed')
p.write_text(s)
