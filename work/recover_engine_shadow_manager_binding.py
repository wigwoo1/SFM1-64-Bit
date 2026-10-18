from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
ranges=[(0x100ff790,0x100ff796),(0x1030a790,0x1030a7a6),(0x1030a7b0,0x1030a7c5),(0x1010c5a0,0x1010c8fb)]
ranges += [(0x101002a0,0x101002a7),(0x10104a30,0x10104a5f),(0x1010a7d0,0x1010a82c)]
ranges += [(0x1010a6b0,0x1010a715),(0x1010a590,0x1010a6ad)]
ranges += [(0x1010a500,0x1010a589)]
ranges += [(0x101083f0,0x101084a3),(0x101084b0,0x10108563)]
ranges += [(0x1019d6e0,0x1019d6f7)]
ranges += [(0x1010cf30,0x1010d11d),(0x10248ca0,0x10248cba)]
ranges += [(0x1024a580,0x1024a8b6)]
ranges += [(0x1024a9c0,0x1024ad5f)]
ranges += [(0x10249b70,0x10249efb),(0x10249f00,0x1024a137),(0x10246a40,0x10246a5f)]
ranges += [(0x102467e0,0x10246852),(0x10246860,0x102468d7),(0x102468e0,0x1024695d)]
ranges += [(0x10244060,0x102440cd)]
ranges += [(0x1010b9d0,0x1010bac3)]
ranges += [(0x10109b80,0x10109c4b)]
ranges += [(0x1014e1f0,0x1014e2ae)]
ranges += [(0x10109a60,0x10109b71),(0x10109990,0x10109a5a)]
ranges += [(0x10109250,0x101092df),(0x101092e0,0x10109354)]
ranges += [(0x10107f60,0x10107ff8)]
ranges += [(0x10104e70,0x10104fba)]
ranges += [(0x100a3730,0x100a37c3)]
ranges += [(0x10102500,0x1010257b)]
ranges += [(0x10104a60,0x10104bf0),(0x10260030,0x102600ad)]
ranges += [(0x100b1770,0x100b1800)]
ranges += [(0x101091e0,0x10109247)]
ranges += [(0x10246af0,0x10246bbe)]
ranges += [(0x10104a00,0x10104a28)]
ranges += [(0x1010a070,0x1010a15b),(0x101685b0,0x1016863e)]
ranges += [(0x10102790,0x101027ac),(0x100ff7a0,0x100ff7a9),(0x100d3880,0x100d389c)]
ranges += [(0x10308e70,0x10308e86),(0x100d8d80,0x100d8e0c)]
ranges += [(0x100d8b50,0x100d8cc2),(0x101027b0,0x101027b6)]
ranges += [(0x100d2090,0x100d2179)]
ranges += [(0x100d4c70,0x100d4e11),(0x100d32e0,0x100d3385)]
ranges += [(0x100d7440,0x100d74f3),(0x100d7eb0,0x100d7f4c),(0x100d8220,0x100d8295)]
ranges += [(0x100d48c0,0x100d4911),(0x1010a7b0,0x1010a7d0),(0x1010a160,0x1010a245)]
ranges += [(0x100d4920,0x100d4972),(0x100d89b0,0x100d8b4e),(0x100d9ee0,0x100d9f8a)]
ranges += [(0x100d6d90,0x100d6fb6)]
rows=[];pending=None
for line in Path('work/engine-disassembly.txt').open():
 m=re.match(r'^\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)',line)
 if m:
  va=int(m[1],16);pending=None
  if any(a<=va<z for a,z in ranges):pending={'address':hex(va),'bytes':m[2].strip(),'instruction':m[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+\n?',line):pending['bytes']+=' '+line.strip()
for row in rows:
 b=bytes.fromhex(row['bytes']);o=im.offset(int(row['address'],16));assert im.b[o:o+len(b)]==b
table=0x1034435c;col=im.word(table-4)
report={'sha256':sha,'checked_instructions':len(rows),'interface':im.string(0x103445c0),'factory':'100ff790 returns105f2198','initialization':'1030a790 invokes1010c5a0 on105f2198; constructor installs1034435c primary table.','rtti':im.string(im.word(col+12)+8),'slots':{str(n):hex(im.word(table+n*4)) for n in [1,5,7,15,21,31]},'limitations':'Static registration and full selected constructor verified; methods, live initialization and renderer resource lifecycle remain incomplete.','instructions':rows}
assert im.word(table+7*4)==0x10104a00
assert im.word(0x1010c88b)==0x10102790
assert im.word(0x1010c895)==0x100ff7a0
assert 0x105f2198+0x1c==0x105f21b4
model_table=0x1034086c;model_col=im.word(model_table-4)
model_rtti=im.string(im.word(model_col+12)+8)
assert model_rtti=='.?AVCModelRender@@'
assert 0x105cb978+0x0c==0x105cb984
assert im.word(0x10401cd0)==0x105f2198
assert im.word(table+46*4)==0x101027b0
assert im.word(0x10340588)==0xc97423f0
assert im.word(0x1034086c+4*4)==0x100d8220
assert im.word(0x1034435c+12*4)==0x1010a7b0
assert im.word(0x1034086c+5*4)==0x100d4920 and im.word(0x1034086c+13*4)==0x100d9ee0
assert im.word(0x1034086c+8*4)==0x100d6d90
report['model_render_record_studio_resource_creation']={'slot':8,'method':'100d6d90..100d6fb5 full byte-checked body ret1c','identity_evidence':'Caller warning '+repr(im.string(0x10340618))+' and material-variable '+repr(im.string(0x10340604))+'. Resource+8 is decal-related by this evidence; exact public resource type unproven.','lifetime':'After eligibility/prefix work, record captured inEDI.100d4a30 false triggersStudio026 slot34(record+8) then clearscapturedrecord+8. Ifrecord+8 null, record.model+134 uint16 ->global10694200 slot14 descriptor ->Studio026 slot33(descriptor), storesreturnedresource atcapturedrecord+8. No localnulldescriptor/result rejection.','continuation':'Fetchmodel metadata via1040a5a4 slot29; +e8 gates furtherdecalwork. Renderingcontext10694180 slot103, slot2 begin,slot96 allocatesmetadata+9c*48bytes, objectslot13 writes data. Studio026 slot35 receivesresource, descriptor andremainingarguments. Temporary100f7860 cleanup thencontextslots3/1.','limitations':'Full caller byteverified, exactargument semantics/helper100d4a30 andcontext allocation/dataformat pending. Capturedrecord remainsacrosscallbacks ratherthanreloaded; x64 relocation mustrespectthis. Resource isnotproof ofentiremodel GPUownership.'}
report['model_render_record_optional_setup']={'slot5':'100d4920 full body. Low16ffff skip. Nonnull argument2 reads its first32bit word, sets record+fa bit0, storesword+fc. Null clearsbit0 and+fc. Pointer interpretation not established.','helper100d89b0':'Complete body. Config105cb25c pointed+30 gate, objectslot0 result passed to101c46e0 service slots11/13 eligibility gates plus105ce3c1. Builds filename from service slot12/model value, selected formats '+repr(im.string(0x10340840))+' / '+repr(im.string(0x1034084c))+', filesystem-like service10643f20+4 slot14 loads into temporary10257260 buffer. Onload: model+134 uint16 service10694200 slot13 returns descriptor; checks loadedword0==2, loadedword1==descriptor+8, loadedword3==config105cae3c+30*4. Success flags=(flags&fffb)|2, mismatch flags|4. Tempdestructor10014b50; stackcookieverified path.','slot13':'100d9ee0 complete. ffff false; ifcapturedmodel+114 lacks2000bit true. Elseoptionalconfig105cb1ac+30 withrecord+fc invokes10196a40(value,0,1) and100d16a0(record+68,result). Model+134 service10694200 slot14 lookup; null false, nonnull objectslot0(full recordargument), then100d9500(owner,objectResult) andreturns itsAL.','limitations':'Full setup callers byteverified; publictypes/modelcache/filesystem binding, helper decoding/native setup parity andlivecreation unfinished. No assumed semantic meaning for flagbits2/4.'}
report['model_render_record_cleanup_dependencies']={'shadow_slot12':'1010a7b0 full body. Low16 ffff skips; otherwise forwards full argument to1010a160 on owner+130 paired container.','reverse_pair_cleanup':'1010a160 full body. Symmetric direction of1010a070: initial/final provider+3c selects model record head; current nodes are first pool at+0, payload word0 shadow key/word1 second-pool index. Provider+38 shadow head repaired if matching; unlink/recycle second pool via101685b0 then capture first-pool successor, unlink/recycle first pool. Final live model head clearedffff. Every storage/provider read follows original sequence.','record_resource_cleanup':'100d48c0 full body. Low16ffff skips; record+100 nonzero invokes owner+8 service slot8(recordResource,0,0,1). Reloads owner+0c storage after callback and clears selected record+100. No callback or clear if initial resource null. Service type/ownership contract unproven.','limitations':'All listed bodies byte verified. Reverse paired cleanup native execution and live resource service parity pending; first release service global106941d4 slot34 still unidentified.'}
report['model_render_record_lifetime']={'release_slot':4,'release':'100d8220 full body byte checked. Low16 ffff skips all work; otherwise reads record+8, calls global106941d4 service slot34(value), shadow singleton10401cd0 slot12(full original argument), then100d48c0(owner,full argument). Calls100d7440(pool,index), reloads storage, writes entry.next=freehead and freehead=low16index. Freshcount/highwater unchanged. No explicit record destructor or payload zero in this method.','unlink':'100d7440 full body; validates low16 index against signed capacity and uint16 maximum, self-previous guard; repairs previous/next or head/tail, marks both links self and decrements uint16 linked count. Same logical behavior as owned engineShadowPoolUnlinkCandidate, with original108h stride.','insert_before':'100d7eb0 full body; first unlinks incoming index via100d7440, then places it before low16 first argument. ffff selects current tail/append. Repairs neighbors/head/tail and increments uint16 linked count. Does not allocate or initialize payload.','limitations':'Static full methods only. Global106941d4 service identity, shadow slot12 and100d48c0 cleanup bodies remain unresolved; native full release/resource parity and live runtime pending.'}
report['model_render_record_allocation']={'method':'100d4c70 full body byte verified; thiscall ret4; lowbyte initialization flag','pool':'Storage+0, signedcapacity+4, growth+8, freeheaduint16+10, freshcountuint16+14, maximumuint16+16, cachedstorage+18. Entrystride108h, previous+104,next+106.','reuse':'Nonffff freehead popped usingentry.next; no freshcapacity/index gates repeated.','fresh':'If maximum<capacity, compute maximum+1 with low16 candidate validation; otherwisecandidate0 whenpositivecapacity, ffffotherwise. Invalidcandidate calls100d32e0(pool,1), reloadcache/storage and recomputes. Stillinvalid emits importedwarning atmost10 via105cbf4c and returnsffff. Candidateffff evenwhenvalidcapacity emits separateindexrangewarning atmost10 via105cbf48 thenreturnsffff. Incrementfreshcountuint16wrap/storemaximum onlyonsuccess.','initialize':'Lowbyteflag0 sets bothlinks toindex; nonzero setsbothffff. Creationpasses0. No payloadclear inallocator.','failure_evidence':'Warning strings verified: '+im.string(0x103217d0)+' / '+im.string(0x1032179c)+'. Original allocator can returnffff; creationcaller still indexesffff, so local allocator does not resolve caller safety.','growth':'100d32e0 fullbody verified. Negativegrowthvalue no-op; positivefixedincrement rounding; zero doublescapacity/start1. Applies uint16capacity-range corrections: zero low16 result with desired<=ffff decrementsroundedcapacity; otherwise desired mustfituint16, repeatedly signedaverages rounded/desired until low16capacity>=desired. Ifdesired doesnotfit, returnswithoutallocation. Commitscapacitybeforetier0g_pMemAlloc slot1malloc/nulloldptr orslot3realloc(oldptr,capacity*108h); resultunconditionallyreplacesstorage.','limitations':'Static allocation/growth only. Extremeoverflow, ownedx64allocator, nativeexecution parity, creationfailure/callerinvariants and model lifetime unfinished.'}
report['model_render_record_constructor']={'method':'100d2090 complete body byte verified; thiscall no stackargs, returns inputthis inEAX','writes':'Zero32bit float/word cells from+0c through+54 inclusive and+68 through+b0 inclusive. Total152bytes written. Remaining112bytes of108hrecord untouched, including object/model fields, blocks+58..67, trailing state and listlinks. No callback/absolute dependencies.','creation_override':'100d8b50 later replaces firstcomponents of six3float blocks at+68/+74/+80/+8c/+98/+a4 with1. Other explicitcreation fields also set separately; constructor alone is not fullrecordinitialization.','limitations':'Byte-schema initializer matches original constructor on4096 initial patterns, all264recordbytes plus32guards; original returns unchangedthis. See model-render-record native validation. Real64-bit pointers/layout must be represented separately; full creation/lifetime integration pending.'}
report['model_render_record_creation']={'slot':3,'method':'CModelRender primaryslot3 target100d8b50; full body byteverified, thiscall ret8/returnsAX record index','allocation':'Argument1 objectslot8 called first, result captured. Allocate108h pool via100d4c70(owner+0c,0), low16ffff failure choosesffff. Success appends via100d7eb0(owner+0c,ffff,index), initializes record100d2090 if computedaddressnonnull. Allocation/helperinternals pending.','initialization':'Records selected bylow16index*108h; objectarg1 at+0, capturedslot8result+4, zero+8/+100/+fc/+fa/+b0. Constant floatbitsc97423f0 at+f4. Explicit zero/one writes initialize sixfloat-triplets at+68..+ac. No entire-record memset inferred.','pair_head':'Global10401cd0 on-disk pointer105f2198 identifies shadow singleton; invokesprimaryslot46(+b8) target101027b0, completebody returnsEAXffff. StoresAX atmodelrecord+f8, establishing invalid initial shadow-pair head.','optional_setup':'Nonzeroarg2 invokesowner slot5(index,arg2),100d89b0(owner,index),owner slot13(index); returnAXindex. Exactservice/publictype names remainunproven.','failure':'After allocator returnsffff, caller still indexesrecordffff and performsinitialization; do not invent earlyfailure return absent original instructions. Allocator/callerinvariants need investigation before owned integration.','limitations':'Static full caller recovery; record constructor/allocator/optionalsetup/release and native lifetime parity unfinished.'}
report['projection_opposite_record_owner']={'singleton':'105cb978 CModelRender; RTTI from primarytable1034086c/COL'+hex(model_col)+' is '+model_rtti,'initialization':'Staticinitializer10308e70 selects105cb978 andcalls100d8d80, thenregisters103158b0 destructor via102de494. Complete initializer/constructor byteverified. Constructor primarytable1034086c, secondarytable10340860 at+4; clears recordstorage+0c/capacity+10/growth+14, sets+18=-1, +1c=ffff, +20=0,+22=ffff andcache+24 fromstorage.','storage':'Global105cb984 is CModelRender singleton+0c, not an independent unrelated global. Pairedmap opposite key selects low16*108h records there; pairhead is+f8 within record.','scope':'Identifies model-render owner and initial pool state. Exact public record/key type, allocation/release routines and live lifetime still require recovery. Constructor helper100d7220 not yet decoded.'}
report['projection_pair_head_providers']={'registration':'Byte-verified constructor1010c885/1010c88f writes10102790 toowner+168(container+38),100ff7a0 toowner+16c(container+3c); immediate operand bytes independently checked.','source':'10102790 cdecl: keylow16*150h + global105f21b4 pointed +12c. Global is registered shadow singleton105f2198+1c, hence source heads live in shadow records at+12c (distinct from receiver node head+128).','opposite':'100ff7a0 tailjumps100d3880; cdecl keylow16*108h +global105cb984 pointed +f8. Returns pointer to opposite record head, not an object or head value. Semantic record type unresolved.','reload':'Both providers reload global storage on every call. No bounds/null/sentinel guard present. Paired routine forwards full32 sourceargument; providers truncate tolow16.','limitations':'Full provider bodies/registration byte verified. Opposite record identity/allocation lifecycle, native provider parity and connected runtime remain unfinished.'}
report['projection_disable_pair_unlink']={'method':'1010a070 complete body byte verified; thiscall ret4 on owner+130 paired-index container','container':'Two8-byte-entry pools atcontainer+0 and+1c; uint16freeheads+10/+2c. Callback pointers+38/+3c take32-bit keys via cdecl and return pointers to uint16heads. Semantic key types remain unresolved.','traversal':'Callback+38(argument) obtains first uint16node index. Pool+1c entryword0 is oppositekey, word1 oppositepoolindex. Callback+3c(oppositekey) obtains otherhead. Ifmatching oppositeindex, reload oppositepoolnext+6 then callheadprovider again and update head. Unlink oppositeentry, push onto firstpoolfreehead. Reloadcurrentpoolnext+6 beforeunlinkcurrent andpush onto secondpoolfreehead. Continuecapturednext, final callback+38 reload thenwriteffff.','unlink_helper':'101685b0 full body byteverified. Low16index; signedcapacity +4 andunsignedmaximumindex+16 validation, previous+4 selfguard, next+6; uint16head/tail+0c/+0e andcount+12 decrementwrap. Logical fields match existing engineShadowPoolUnlinkCandidate, whileoriginalstride8 differs frompreviousc4/304 pools.','limitations':'Static pairedmap recovery only; callbacks/key identity, native execution, owned pair-map implementation and projection-disable integration unfinished.'}
report['projection_plane_record_accessor']={'slot':7,'method':'10104a00 complete body byte verified; thiscall ret4','selection':'Argument low16 selects150h shadow record from owner+1c; record+dc uint16 selects360h projection record from owner+141a4. Returns record base, whose first140h bytes are packed planes. Slot21 returns same record+140. No bounds/null/sentinel guards present.','special_gate':'10109b80 pushes maximum, minimum thenhandle; slot7 consumes onlyhandle, leaving two bound arguments for10246af0 thiscall on returned plane record.10246af0 ret8 consumes remainingbounds. This resolves observed stack convention.','limitations':'Accessor and layout static; live owner/storage/resource lifecycle remains unfinished.'}
report['methods']={'slot31':'101002a0 returns raw ownerbyte+141cc.','slot21':'10104a30 indexes owner+1c shadowrecords bylow16handle*150h; record+dclow16 selects owner+141a4 arraystride360h; returns selectedrecord+140h without localbounds/sentinelchecks.','slot15':'1010a7d0 nonzeroargumentlowbyte clears shadowrecord+78bit3. Zero invokes1010a6b0(handle) then1010a070(owner+130,handle), reloads recordpointer and setsbit3.','limitations':'Disablecleanup helper semantics, recordallocator and slot31 lifecycle unresolved; no inferred bounds policy.'}
report['disable_cleanup']={'method':'1010a6b0','sequence':'Low16handle indexes150hshadowrecords. Traverse record+128 linked pointer; capture node+8 next before1010a590(nodeindex targetownerfrom20bytearray,nodeword0index). Reload shadowrecordpointer afterloop and clearrecord+128, includinginitialnull.','release_helper':'1010a590 indexes20byteentries owner+44. Optional entry+0c16byteauxiliary released via1010a500/100b3990 and linked to owner+d8freehead, theninvalidatedffff. Optional entry+0a released through targetobject+18 slot11. Repairs targetobject+0e listhead and shadowrecord+128 when matching; removes node through101091e0(owner+6c), zeros entry+4nodepointer, invokes10107f60 thenlinks20byteentry to owner+58 freehead.','limitations':'Auxiliaryhelper semantics, resource service ownership and full nodepool mutation remain unresolved. Captured-next ordering must survive release; no parallel traversal policy inferred.'}
report['auxiliary_release']={'method':'1010a500','empty':'Auxiliaryword0 uint16zero returnswithoutrelease.','external':'Nonzero word0 andpointer+8 nonnull calls1019d6e0(pointer) andreturns.','pooled_small':'Nonzero word0<=8 withnullpointer uses poolowner+f8; auxiliary+4uint16index passed101083f0; reloadpoolstorage, recordstridec4 plusc2 receivespreviouspool+10freehead thenpool+10=index.','pooled_large':'Word0>8 withnullpointer uses poolowner+114, calls101084b0; reloadstorage, stride304 plus302 receivespreviouspool+10freehead thenpool+10=index.','limitations':'Word0semanticcount/type, externaldeallocator andpoolhelperinternals unresolved. No record-clearing operation inferred beyond demonstrated free-link stores.'}
report['auxiliary_pool_unlink']={'small':'101083f0 stridec4 previous/next fields+c0/+c2','large':'101084b0 stride304 previous/next fields+300/+302','validation':'Argumentlow16 must be signed-less-than pool+4capacity and unsigned<=pool+16 maximumindex; return when nodeprevious==index.','mutation':'Readprevious/next; previousffff meanspool+0chead=next elseprevious.next=next; nextffff meanspool+0etail=previous elsenext.previous=previous. Set removednodeprevious andnext toitsindex; decrement pool+12uint16count withwrap.','limitations':'These unlink helpers do not perform payload destruction. Caller1010a500 subsequently overwrites next with freehead. Allocation/highwater lifecycle and native parity unresolved.'}
opt=im.u32(60)+24;desc=im.base+im.u32(opt+104);binding=None
while im.word(desc+12):
 dll=im.string(im.base+im.word(desc+12));oft=im.word(desc) or im.word(desc+16);iat=im.base+im.word(desc+16);i=0
 while im.word(im.base+oft+i*4):
  value=im.word(im.base+oft+i*4)
  if iat+i*4==0x1031a4e8:
   assert not value&0x80000000
   binding={'dll':dll,'symbol':im.string(im.base+value+2)}
  i+=1
 desc+=20
assert binding=={'dll':'tier0.dll','symbol':'g_pMemAlloc'}
report['external_release_binding']={'method':'1019d6e0','import':binding,'dispatch':'Read imported variable address throughIAT1031a4e8; dereference toallocatorreceiver; callslot5 with originalpointerargument.','limitations':'Concrete allocatorimplementation, allocation failure and crossmodule ownership lifecycle remain unresolved. Do not substitute arbitrary CRTfree without confirming allocation source.'}
report['projection_prefix']={'slot5':'1010cf30 selected prefix through1010d045; profile '+im.string(0x1034445c),'cleanup':'When owner+141cc zero, cleanup1010a6b0(handle), unlink1010a070(owner+130,handle); record+dc selects360hprojectionrecord, increment projection+340 theninvoke10108170(projection+324) and10079a10(projection+2f4).','gate':'Reload shadowarray; skip projectionwork ifshadowrecord+78bit3 set or global105f1ce4 pointed+30 zero.','input_copy':'10248ca0(source argument2,destination shadowrecord) copies16 uint32words forward unlesssamepointer. Thus argument2 points64bytes; it is not a48byte transform. Then1024a580(shadowrecord,local64byteoutput),10249f00(local,projectionrecord),1024a9c0(local,shadow+d0,shadow+7c).','limitations':'Prefix only; normal projection continuation, mathhelper semantics and setting identity unresolved. Forwardcopy overlap policy differs from arbitrary memmove.'}
assert im.word(0x10350f0c)==0x358637bd
assert im.word(0x1031bdec)==0x3f800000
assert im.word(0x103717a0)==0x7fffffff
assert im.word(0x1031cbb0)==0x80000000
report['projection_matrix_inverse']={
 'method':'1024a580; complete body through1024a8b5 byte verified',
 'layout':'Input and output are row-major 4x4 float matrices (64 bytes). Local 4x8 augmented rows contain input and identity; permutation starts [0,1,2,3].',
 'pivot':'For each column, scan remaining rows in permutation order; choose strictly greatest absolute pivot above float bits358637bd (approximately1e-6). Equal values retain earlier row. Absolute value clears sign bit with7fffffff mask. COMISS/JBE excludes unordered comparisons.',
 'arithmetic':'Swap row permutation indices; divide1.0 by pivot, multiply all eight pivot-row cells separately and explicitly set pivot cell1.0. For each other logical row in ascending order, XOR pivot-column signbit, then separate scalar SSE multiply/add across eight cells; explicitly set eliminated cell positivezero. No fused multiply-add.',
 'success':'Copy right four cells from each final permuted row to output; return AL1. Input/output alias is safe because input is copied to local storage before output writes.',
 'failure':'If no pivot exceeds threshold, return AL0 before any output writes. Output remains unchanged even after earlier successful columns.',
 'caller':'Projection prefix1010d009 calls inverse, then proceeds to10249f00 and1024a9c0 without testing AL. Do not invent an identity fallback or claim caller handles singular input.',
 'limitations':'Owned x64 matches native return flag and all output bits for131072 finite/singular/threshold states, with separate and aliased output. See inverse native validation report. Exceptional values, floating-point environments and full projection integration remain unfinished.'}
assert im.word(0x1031cb50)==0x3f000000
assert im.word(0x1031dcf8)==0x40000000
report['projection_sphere']={
 'method':'1024a9c0 complete through1024ad5e; byte verified',
 'input':'64-byte inverse matrix, second argument output XYZ, third argument output scalar radius.',
 'points':'Transform four homogeneous points in order A=(0.5,0.5,0), B=(0.5,0.5,1), C=(0,0,0), D=(0,0,1). C corrected by complete register data flow and native comparison. Group matrix X+Y before multiplying by0.5 for A/B; C sums X+Y+Z before multiplying by0. Each XYZ is multiplied by reciprocal W when W is nonzero or unordered; exactly zero W is retained as scale, not replaced by1.',
 'construction':'L=sqrt((By-Ay)^2+(Bz-Az)^2+(Bx-Ax)^2); U=(Ay-Cy)^2+(Ax-Cx)^2+(Az-Cz)^2; V=(By-Dy)^2+(Bx-Dx)^2+(Bz-Dz)^2. T=(V+L*L-U)/(L*2). Center=A+(T/L)*(B-A); radius=sqrt(T*T+U). Evaluation follows separate scalar SSE instructions, including written addition order.',
 'interpretation':'Center and radius inferred from the output vector/scalar and geometric construction; this is not a min/max bounds output. Caller passes shadowrecord+d0 for center and+7c for radius.',
 'failure_policy':'No local zero-L rejection or fallback; do not substitute a generic robust bounding sphere without compatibility evidence.',
 'limitations':'Owned x64 matches all four output float bit patterns on65536 bounded finite projective matrices against copied original routine. See sphere native validation report. Exceptional values, FP environments, inverse failure and live integration unfinished.'}
report['projection_planes']={
 'method':'10249f00 complete body; six calls to10249b70 then10246a40 on receiver argument2.',
 'helper':'10249b70 custom register arguments EAX=A, EDX=B, EDI=C, ESI=output16bytes; matrix is stack argument. Transforms each point with row-major matrix and homogeneous divide; zero W remains zero scale, nonzero/unordered W gets reciprocal. Computes (B-A) cross (C-A), writes XYZ, invokes10244060(output), discards x87 return; writes distance dot(normal,A) in order Ny*Ay + Nx*Ax + Nz*Az.',
 'output_order':'Six16-byte local outputs in call order at indices4,5,0,1,2,3; exact point initialization still requires independent validation.',
 'installation':'10246a40(receiver,local96bytes) calls10246860(receiver,first64bytes), then102468e0(receiver+a0,last32bytes). Actual packing helpers remain unresolved.',
 'limitations':'Plane interpretation inferred from cross product plus distance output; normalization10244060 and packing helpers not decoded. Exact source-point triplets still require independent validation. Native parity and renderer integration unfinished.'}
assert im.b[im.offset(0x103b6020):im.offset(0x103b6020)+16]==bytes(16)
assert im.b[im.offset(0x103b6140):im.offset(0x103b6140)+16]==bytes.fromhex('ffffff7f'*4)
report['projection_plane_storage']={
 'four_plane_method':'10246860 reads four16-byte planes, transposes4x4 into component vectors X,Y,Z,D at receiver offsets0,10,20,30 hex, calls102467e0.',
 'two_plane_method':'102468e0 reads two16-byte planes, appends two zero planes (asserted103b6020 constant), transposes into same layout, calls102467e0. Called with receiver=projectionrecord+a0.',
 'derived_method':'102467e0 stores SIMD normal-component less-than-zero masks at offsets40,50,60; stores absolute component bits by AND7fffffff at70,80,90. Signedzero compares false; unordered NaN comparisons false. Absolute preserves NaN payload except signbit.',
 'combined_layout':'Projectionrecord offsets0..9f contain four-plane block; a0..13f contain two-plane block withzero lanes2/3. Each block is160bytes; combined320bytes. Slot21 returns projectionrecord+140, immediately after these blocks.',
 'limitations':'Full storage and derived helper bytes/constants verified. Original read/write overlap constraints, native execution parity, generated plane source points and normalization remain unfinished.'}
assert im.word(0x10322ae8)==0x34000000
report['projection_plane_normalization']={
 'method':'10244060 full body byte verified',
 'length':'Scalar SSE computes sqrt((X*X+Y*Y)+Z*Z); stores rounded float length then loads it into x87 for return.',
 'normalization':'Compute reciprocal1/(length+epsilon), multiply original X/Y/Z individually, write all three. Epsilon at10322ae8 has bits34000000. No branch or unit-normal fallback.',
 'caller':'10249b70 discards returned length, uses written XYZ for plane distance. Exact zero remains zero when epsilon yields finite reciprocal; tiny-vector handling differs from exact division by length.',
 'limitations':'Static instruction/constant verification; owned x64 implementation, original execution parity and complete projection integration pending.'}

# Replay only constant initialization and stack-address LEAs in the already
# byte-verified plane caller. Clear volatile SSE values after every helper call.
def stack_offset(expression):
 m=re.fullmatch(r'ebp([+-])([0-9A-F]+)h',expression)
 assert m,expression
 value=int(m[2],16)
 if m[1]=='-':return -value
 return value-(1<<32) if value&0x80000000 else value
registers={};memory={};calls=[]
for row in rows:
 address=int(row['address'],16)
 if not 0x10249f00<=address<0x1024a137:continue
 instruction=row['instruction']
 m=re.fullmatch(r'xorps\s+(xmm\d+),\1',instruction)
 if m:registers[m[1]]=0
 m=re.fullmatch(r'movss\s+(xmm\d+),dword ptr ds:\[([0-9A-F]+)h\]',instruction)
 if m:registers[m[1]]=im.word(int(m[2],16))
 m=re.fullmatch(r'movss\s+dword ptr \[(ebp[+-][0-9A-F]+h)\],(xmm\d+)',instruction)
 if m:
  assert m[2] in registers,instruction
  memory[stack_offset(m[1])]=registers[m[2]]
 m=re.fullmatch(r'lea\s+(eax|edx|edi|esi),\[(ebp[+-][0-9A-F]+h)\]',instruction)
 if m:registers[m[1]]=stack_offset(m[2])
 if re.fullmatch(r'call\s+10249B70',instruction):
  points={name:[memory[registers[reg]+k*4] for k in range(3)] for name,reg in [('A','eax'),('B','edx'),('C','edi')]}
  assert all(value in (0,0x3f800000) for point in points.values() for value in point)
  calls.append({'call':row['address'],'output_plane_index':(registers['esi']+0x64)//16,'point_bits':points})
  for reg in list(registers):
   if reg.startswith('xmm'):del registers[reg]
expected=[[[0,0,0],[1,0,0],[0,1,0]],[[0,0,1],[0,1,1],[1,0,1]],[[1,0,0],[1,1,1],[1,1,0]],[[0,0,0],[0,1,1],[0,0,1]],[[1,1,0],[1,1,1],[0,1,1]],[[1,0,0],[0,0,1],[1,0,1]]]
assert len(calls)==6
assert [[list(map(lambda value:int(value!=0),call['point_bits'][name])) for name in ['A','B','C']] for call in calls]==expected
assert [call['output_plane_index'] for call in calls]==[4,5,0,1,2,3]
report['projection_plane_source_initialization']={'calls':calls,'verification':'Symbolic replay of byte-verified constant writes and stack LEAs; volatile SSE registers invalidated after helper calls. Exact source bits retained rather than imposing conventional frustum corner order.','limitations':'Static caller replay only; original runtime comparison remains necessary before integration.'}
report['projection_continuation']={
 'method':'1010cf30 complete body through1010d11c nowbyteverified; ret16 confirmsfour stack arguments.',
 'arguments':'Argument1 uint16shadowhandle; argument2 64-byte matrix; argument3 signed32 itemcount; argument4 pointer to array of32-bit itemvalues. Countzero skipsenumeration; negativecount stillperformssetup butloopdoesnotexecute.',
 'setup':'Nonzero count: global105ca60c increment; global104083f0 pointed+6c passed100b1720. Build localpayload at ebp-84 withhandleword at0, projectionstate+140 XYZ at4/8/c, shadowcenter XYZ at1c/20/24, shadowradius at28, zero32 at2c. Otherpadding/fields notassigned on this branch.',
 'enumeration':'Ascending index0..count-1, reloadarraypointer savedargument4 eachiteration, read32-bit itemvalue; invoke1010b9d0 onowner+4 withitemvalue andpayloadpointer. Capturedcountnotreloaded.',
 'completion':'Allgates and enumeration convergeon importedprofilerexit, stackcookiecheck, ret16.',
 'limitations':'Item semantic type,100b1720/1010b9d0 internals andpayload fields/padding consumercontract unresolved. Sharedglobalincrement and callbacks do not establish safeparallel enumeration. Full original method runtime notexecuted.'}
report['projection_item_callback']={
 'method':'1010b9d0 full body verified; receivesowner+4,32-bit itemvalue,payload; alwaysreturnsAL1.',
 'optional_filter':'Ifpayload+2c pointer nonnull,101465b0(item) gives signedbitindex; arithmeticshift3 givesbyteoffset,low3 givebitmask. Unsetbit skipsprocessing andstillreturns1. Currentprojectioncaller setspayload+2c zero, bypassingthisfilter.',
 'records':'Payloadword0low16indexes150hshadowrecords via receiver+18 (=completeowner+1c). Itemvalue<<6 indexes64-byte records through global104083f0 pointed+10.',
 'dispatch':'Adjustreceiverback4tocompleteowner. Shadowrecord+78 bits1/2 selects1010b790(shadowrecord,itemrecord,payload) andlocalflag1; otherwise10109c50(sameargs) andlocalflag0.',
 'attached_objects':'Iteratefrom0 whileindex<reloaded itemrecord+3e uint16count. Baseitemrecord+3c uint16 plusindex selects global104083f0 pointed+e0 uint16array;100b16e0(globalpointed+6c,selectedindex) yieldsobject. Objectslot13(+34) lowbyte nonzero skips; otherwisecallslot14(+38),then10109b80(completeowner,payload,object,localflag).',
 'ordering':'Reloadglobalarrays/countbetweeniterations; callbacks canaffectcurrentstorage. No snapshot or parallelcallback policy inferred.',
 'limitations':'64-byte item semantic type,101465b0/1010b790/10109c50/100b16e0/10109b80 internals andobjectservice identities unfinished. No localnullguard returnedobject; full runtime/resource parity pending.'}
report['projection_attached_object_gate']={
 'method':'10109b80 fullbody; args payload,object,branchflaglowbyte.',
 'ordinary_prefilter':'Branchflagzero callsobjectslot5; returnedrecordword0 bit2000 set skips immediately. Specialbranch bypasses this prefilter.',
 'bounds':'Objectslot3 receives two pointers tolocal12-byte vectors; meaning inferred as bounds from downstream tests, exact service type notyetidentified.',
 'ordinary_spatial_test':'Call1014e1f0(localvector1,localvector2,payload+1c center,payload+28 radius); ALzero skips.',
 'special_spatial_test':'Engineownerslot7(10104a00) consumes handlelow16/ret4 and returns projectionrecordbase. Remainingbounds arguments feed thiscall10246af0/ret8. Any packedplane rejection returnsAL1 andskips; no rejection continues. See plane accessor and native box-plane validation.',
 'acceptance_flags':'Reloadobjectslot5 record. Require recordbyte+16 bit0 or nonzero branchflag. Reloadshadowrecord flags+78; ifbits1/2 clear andobjectrecordword0&3020 nonzero skip. Otherwise10109a60(owner,objectrecord,shadowhandle).',
 'limitations':'Owned x64 gate connects recovered spatial tests and flag ordering;512 combinations and record replacement pass. Record/service ABI, full original gate parity and resource association integration unfinished. Originalcallbacks/loadordering retained; no genericflag names invented.'}
report['projection_box_sphere_test']={
 'method':'1014e1f0 fullbody verified; args minimumXYZ,maximumXYZ,centerXYZ,radiusfloat.',
 'distance':'For eachaxis: minimum>center givescenter-minimum; elsecenter>maximum givesmaximum-center; otherwisezero. Squareselecteddifferences. Xinitializesaccumulator; Y adds contribution+accumulator, Z addsaccumulator+contribution; skippedaxes do notaddzero.',
 'accept':'Return1 onlywhen radius*radius strictlygreater thanaccumulateddistanceSquared. Tangency andzero-radius insidebox return0; negativeradius is squared without rejection. Unordered finalcomparison returns0.',
 'limitations':'Staticbyteverification; ownedimplementation/nativeparity pending. No boundsreordering/radiusclamp inferred.'}
report['projection_receiver_association']={
 'method':'10109a60 fullbody; args objectrecord,shadowhandle.',
 'allocation':'10104e70(owner+44,1) allocates20-byte entry; low16ffff rejected. Failure returnsffff, emits warning atmost10times via10606b04. Capacityhighwater diagnostics do not define allocation semantics.',
 'receiver_links':'Entryword0=objectrecord;101092e0(owner+44,oldobjectrecord+0ehead,newindex); objectrecord+0e=newindex.',
 'resource':'Objectrecordword0 bit11 set callsobjectrecord+18 receiver slot10(shadowhandle), storesAX entry+0a; clearbit storesffff. Releasehelper1010a590 alreadyidentified matching slot11.',
 'initialization':'Entry+8=shadowhandlelow16; entry+0c dword=ffffffff (twoinvaliduint16fields); entry+4nodepointer=0;10109990(owner,shadowhandle,index); returnAXindex.',
 'node_link':'10109990 invokes10104a60(owner+6c,1). Null result emits warning atmost10times andreturnswithoutrollback. Otherwise nodeword0=indexlow16; ifshadowheadnonnull invoke10109250(owner+6c,oldhead,newnode); reloadshadowstorage, setshadowrecord+128=newnode; reloadentryarray,setentry+4=newnode.',
 'partial_failure':'Node allocation failure leaves alreadyallocated receiver entry/resource association in place withentry+4null. Do not introduce all-or-nothing rollback without full compatibility evidence.',
 'limitations':'Poolallocation/linkhelperinternals/objectserviceidentity/nativeexecution andrenderer integration unfinished.'}
report['projection_association_link_helpers']={
 'pointer_method':'10109250(targetnode,newnode), receivernodepool. Ifnewnode nonnull and10102500(newnode,pool+20) ALzero andnewnode.previous!=self, firstunlink: repairneighbors orpool+0c/+10 head/tail, markprevious/nextself, decrementpool+18 count.',
 'pointer_insert':'Alwayswrite newnode.next=target. Targetnull meansappendusingpooltail+10 thenupdatetail; otherwisereadtarget.previous andsettarget.previous=newnode. Setnewnode.previous=previous; previousnull setshead+0c elseprevious.next=newnode; incrementcount+18. Nullnewnode is not a safe no-op: insertion stilldereferences it.',
 'index_method':'101092e0(targetindex,newindex), receiver20-byte entrypool. First10107f60(newindex) unlink, thenreloadstorage. Entry+12=targetlow16. Exact32-bit targetffff appends usingpool+10tail andsets32-bit tailnewindex; otherwiseentry+10=target.previous andtarget.previous=newindexlow16. Previousffff sets32-bit head+0c=newindex; otherwiseprevious.next=low16newindex; incrementpool+18 count32.',
 'index_fields':'20-byte entryprevious+10 andnext+12, consistentwithprevious release/free-link observations. Target comparison uses full32bits beforelow16 neighbor sentinel comparisons; no generalized highbit alias policy inferred.',
 'limitations':'10102500 membership predicate and10107f60 fullunlink/validation semantics unfinished. Owned x64 listimplementation/nativeparity pending.'}
report['projection_receiver_unlink']={
 'method':'10107f60 completebody byteverified.',
 'validation':'Fullsigned32 argument must>=0,<pool+4capacity,<=pool+20 highwater. Noinitiallow16 truncation. Nodepreviousuint16==fullargument meansalreadyunlinked return.',
 'mutation':'20-byte entryprevious+10/next+12. Repairprevious.next or32-bit pool+0chead; repairnext.previous or32-bit pool+10tail; decrement32-bit pool+18count withwrap; markprevious/next=argumentlow16.',
 'limitations':'Validneighbor/storagepreconditions; caller poolallocation/highwater invariants andnativeexecution pending. This differs frompreviouslyrecovered low16 small/largeauxiliarypool unlink.'}
report['projection_receiver_allocation']={
 'method':'10104e70 fullbody byteverified; poolowner+44, argumentlowbyte initializeslinks.',
 'reuse':'Pool+14 full32 freehead!=ffff selectsindexdirectly; entry+12 nextuint16 becomespool+14. No fresh-index gates appliedonthisreusepath.',
 'fresh_candidate':'Ifhighwater+20 is>=0 and<capacity+4, candidate=highwater+1 ifstillvalidelse-1; otherwisecandidate0 when capacity>0else-1.',
 'growth':'Invalidcandidate calls100a3730(pool,1), cachesreloadedstoragepointer atpool+24, recomputescandidate. Stillinvalidreturnsffff andwarnsatmost10times via10606aac.',
 'index_width':'Freshcandidate mustequalzeroextendedlow16 andlow16!=ffff; otherwisewarnsatmost10times via10606aa8 andreturnffff. Onaccept incrementpool+1c32-bitfield andstorehighwater+20=candidate.',
 'links':'Reloadstorage. Argumentlowbytezero initializes entry+10/+12 bothindexlow16 (self-linked); nonzero initializesbothffff. Currentassociationcaller passes1. No payloadclear beyondlinks.',
 'limitations':'Growthhelper/allocatorfailure/storageownership unfinished; meaningofpool+1c versuslinkedcount+18 notassigned. Reusedindexpath reliesonexistingfree-list invariants. Nativeexecution andownedallocator pending.'}
report['projection_receiver_pool_growth']={
 'method':'100a3730 fullbody byteverified; receiverpool, argumentadditionalcount.',
 'fixed_external':'Signed pool+8 growthvalue<0 returnswithoutmutation/allocation.',
 'capacity':'Requested=oldcapacity+additionalcount with32-bit arithmetic. Growthvalue>0 rounds via signeddivision ((requested-1)/growthvalue+1)*growthvalue. Growthvaluezero startsoldcapacity or2 ifzero anddoublesuntil>=requested. Originaloverflowadjustment paths retainedinaudit, not generalized as safe size_t math.',
 'allocation':'Store newcapacity to pool+4 BEFORE allocatorcall. Bytes=newcapacity*20 using32-bit arithmetic. Dereference tier0 g_pMemAlloc IAT1031a4e8 variable. Oldpointer nonnull callsallocator slot3(oldpointer,bytes); null calls slot1(bytes). ReturnedEAX unconditionally replacespoolword0.',
 'failure':'No localnull-result test/rollback ofstoredcapacity oroldpointer; do not invent generic vector allocation-failure policy. Growthvalue negative may indicate externalstorage but exact semanticidentity remainsunverified.',
 'lifecycle':'Slot1allocation/slot3reallocation pair withpreviouslyrecovered slot5free throughsame engine allocatorvariable. Native allocator object implementation andliveownership unfinished.',
 'limitations':'Signedoverflow/extreme capacities andrealallocationfailure requireadditionalcontract evidence; owned growthintegration/nativeexecution pending.'}
report['projection_node_frontier_predicate']={
 'method':'10102500 fullbody verified; args nodeaddress, descriptorpointer(pool+20).',
 'descriptor':'Descriptorword0 blockpointer; descriptor+4 signedindex. Nullblock orindexnegative orindex>=block+4signedcount returns0.',
 'current_block':'Range [block+8,block+8+count*12) usesunsignedaddresscomparisons. Nodeinside returns signedaddress(node)>signedaddress(block+8+index*12). Exactfrontier returns0; laterbytes return1 withno alignmenttest.',
 'older_blocks':'Nodeoutsidecurrentrange followsblockword0 linkedblocks. Any laterblockrange containingnode returns1, withno index/alignment check. No match returns0.',
 'caller':'10109250 skips preliminaryunlink whenpredicatetrue; otherwiseprevious!=self permitsunlink. Therefore callingthis a genericmembershipcheck wouldmiss current-frontier distinction.',
 'limitations':'Descriptorfrontier production/blockallocation/invariants/nativeparity unfinished. Native32-bit signedpointer comparison needs explicitpolicywhenporting to64-bit addresses.'}
report['projection_node_allocation']={
 'method':'10104a60 fullbody verified; poolowner+6c,initializearglowbyte.',
 'reuse':'Non-nullfreeheadpool+14 selecteddirectly, node+8 becomesnewfreehead.',
 'fresh':'Valid descriptor(pool+20block,+24index) advancesvia10260030; invaliddescriptor startsfirstpoolword0block/index0 ornull/-1. Invalidresult invokes100b1770(pool,1), clears pool+28, recomputes. Stillinvalid returnsnull withrate-limitedwarning.',
 'advance':'10260030 validates inputblock/index. index+1<count stayssameblock; otherwiseblockword0nextblock/index0 ifnonnull else null/-1. No recursive skip ofemptyblocks.',
 'commit':'Address=block+8+index*12, rejectnulladdress; storeselectedblock/index todescriptor; incrementpool+1c freshfield. Initializeprevious+4/next+8 self whenargumentlowbytezero ornullwhen nonzero. Currentassociationpasses1.',
 'limitations':'Blockgrowth100b1770/internalallocator unverified; pool+28 meaningunassigned. Native allocation/linkresource integration pending.'}
desc=im.base+im.u32(opt+104);error_binding=None
while im.word(desc+12):
 dll=im.string(im.base+im.word(desc+12));oft=im.word(desc) or im.word(desc+16);iat=im.base+im.word(desc+16);i=0
 while im.word(im.base+oft+i*4):
  value=im.word(im.base+oft+i*4)
  if iat+i*4==0x1031a4dc:
   assert not value&0x80000000
   error_binding={'dll':dll,'symbol':im.string(im.base+value+2)}
  i+=1
 desc+=20
assert error_binding is not None
report['projection_node_block_growth']={
 'method':'100b1770 fullbody verified; argumentadditionalcount signed<=0 no-op.',
 'block_count':'Nonzero pool+8 growthvalue useddirectly; zero usespool+4 cumulativecapacity or3ifzero. Ifselectedcount<requested round selectedcount*((selectedcount+requested-1)/selectedcount) via signeddivision. Negativegrowth/extremeoverflow behavior notgeneralized.',
 'allocate':'Increment cumulativecapacitypool+4 beforeallocation. tier0 g_pMemAlloc slot1 allocates selectedcount*12+8 bytes using32-bit arithmetic; null calls importedfailurefunction.',
 'failure_import':error_binding,
 'header_and_link':'Write newblockword0=null, word1=selectedcount. Ifpoolword0null installfirstblock; otherwisetraverseblockword0links totail andappend. Existingnodeaddresses remainstable; no realloccall here.',
 'limitations':'Nullallocation path reliesonimportedfailurefunction notreturning; otherwise subsequentheaderwrites dereference null. Actual allocator/fatalimplementation, blockrelease andfullownednodepoolintegration unfinished.'}
report['projection_node_removal']={
 'method':'101091e0 fullbody verified; nodepool/nodepointer.',
 'unlink':'Nonnullnode, frontierpredicate10102500 false andnode.previous!=self permitunlink. Repairprevious.next orpoolhead+0c; repairnext.previous orpooltail+10; selfmarkprevious/next; decrementpool+18 count32.',
 'free_link':'Regardlessofunlinkgate, write node.next=oldpool+14freehead; pool+14=node. Nullnode stilldereferencedhere; repeatedfree notguarded andcancreatefree-listcycle.',
 'limitations':'Validallocatednon-nullnode andsinglefree arecallerpreconditions. Frontier/blockownership/nativeparity/fullcleanupintegration pending.'}
Path('outputs/sfm64-engine-shadow-manager-binding.json').write_text(json.dumps(report,indent=2));print('checked_instructions',len(rows));print(report['rtti'],report['slots'])

