from pathlib import Path
p=Path('work/vtf_texture_layout.h');s=p.read_text().replace('// PC VTF7.2,','// PC VTF7.2/7.3,').replace('word(8)!=2','(word(8)!=2&&word(8)!=3)').replace('VTF7.2 required','VTF7.2/7.3 required')
a=s.index(' uint64_t offset=header;');b=s.index(' VtfTextureLayoutCandidate result;',a)
s=s[:a]+''' uint64_t offset=header;auto lowWidth=bytes[61],lowHeight=bytes[62];if((lowWidth==0)!=(lowHeight==0))throw std::runtime_error("VTF low-res dimensions");
 if(word(8)==2){if(lowWidth){auto lowSize=imageSize(lowWidth,lowHeight,word(57));range(offset,lowSize);offset+=lowSize;}}
 else{if(header<80)throw std::runtime_error("VTF resource header");auto resources=word(68);if(!resources||resources>32||uint64_t(80)+uint64_t(resources)*8>header)throw std::runtime_error("VTF resource table bounds");bool highSeen=false,lowSeen=false;uint64_t lowOffset=0,lowSize=0;
 for(uint32_t n=0;n<resources;++n){auto type=word(80+size_t(n)*8),data=word(84+size_t(n)*8);if(type==0x30){if(highSeen||data<header)throw std::runtime_error("VTF image resource");highSeen=true;offset=data;}else if(type==1){if(lowSeen||!lowWidth||data<header)throw std::runtime_error("VTF thumbnail resource");lowSeen=true;lowOffset=data;lowSize=imageSize(lowWidth,lowHeight,word(57));range(lowOffset,lowSize);}else throw std::runtime_error("unsupported VTF resource type");}
 if(!highSeen||bool(lowWidth)!=lowSeen)throw std::runtime_error("missing VTF image resource");
 uint64_t highSize=0;for(uint32_t level=0;level<count;++level){auto w=width>>level,h=height>>level;highSize+=imageSize(w?w:1,h?h:1,format);}range(offset,highSize);if(lowSeen&&lowOffset<offset+highSize&&offset<lowOffset+lowSize)throw std::runtime_error("overlapping VTF image resources");
 }
''' +s[b:]
p.write_text(s)
