from pathlib import Path
p=Path('work/test_restore_dispatch_native.cpp');s=p.read_text()
assert 'crossCategory' not in s
s=s.replace('static bool mutateLists;', 'static bool crossCategory;static void (*appendSecond)();static bool mutateLists;')
s=s.replace('std::memcpy(replacement,oldLists[category],8*sizeof(Callback));replacement[8]=appended;', 'const auto count=fixtureOwner[(category?0x2eac:0x2e98)/4];std::memcpy(replacement,oldLists[category],count*sizeof(Callback));replacement[count]=appended;')
s=s.replace('/4]=9;oldLists', '/4]=count+1;oldLists')
s=s.replace('#else\nstatic Registry* activeLists', 'static void addSecond(){auto& count=fixtureOwner[0x2eac/4];oldLists[1][count++]=appended;}\n#else\nstatic Registry* activeLists')
s=s.replace('activeLists[category]->append(appended);}', 'activeLists[category]->append(appended);}\nstatic void addSecond(){activeLists[1]->append(appended);}')
s=s.replace('if(mutateLists&&calls[category]==1)mutateList(category);', 'if(calls[category]==1){if(crossCategory&&category==0){appendSecond();*flags[1]|=0x20;}if(mutateLists)mutateList(category);}')
s=s.replace('for(bool mutation:{false,true})', 'for(bool cross:{false,true})for(bool mutation:{false,true})')
s=s.replace('mutateLists=mutation;', 'crossCategory=cross;appendSecond=addSecond;mutateLists=mutation;')
s=s.replace('Callback firstList[8],secondList[8];', 'Callback firstList[16]{},secondList[16]{};')
s=s.replace('if(appendedCalls||calls[0]', 'const unsigned expectedAppended=cross&&!zeroFirst&&!suppressFirst&&!suppressSecond?1u:0u;if(appendedCalls!=expectedAppended||calls[0]')
p.write_text(s)
