#pragma once
#include "read_binary_asset_candidate.h"
#include "sfm_model_geometry.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <limits>
#include <string>
#include <tuple>
#include <set>

struct SourceBspBrushTriangleCandidate { std::array<uint32_t,3> vertices{}; uint32_t material=0; };
struct SourceBspBrushGeometryCandidate {
 std::vector<std::array<float,3>> positions,normals;
 std::vector<std::array<float,3>> lightColors;
 std::vector<float> displacementAlphas;
 std::vector<std::array<float,2>> uvs;
 std::vector<std::string> materials;
 std::vector<SourceBspBrushTriangleCandidate> triangles;
 size_t skippedDegenerateTriangles=0,displacementFaces=0,displacementVertices=0,displacementTrianglesRemoved=0;
 std::array<float,3> boundsMin{},boundsMax{},modelOrigin{};
};
struct SourceBspBrushLumpCandidate {uint32_t offset=0,size=0;int32_t version=0,fourCC=0;};
inline SfmModelGeometryCandidate sourceBspStaticModelCandidate(const SourceBspBrushGeometryCandidate& brush){
 if(brush.positions.size()!=brush.normals.size()||brush.positions.size()!=brush.uvs.size()||brush.positions.size()!=brush.displacementAlphas.size()||brush.materials.empty())throw std::runtime_error("invalid BSP static model channels");
 SfmModelGeometryCandidate model;model.staticGeometry=true;model.geometry.lod=0;model.geometry.vertices.resize(brush.positions.size());model.materials.names=brush.materials;model.materials.directories={""};model.materials.skins.resize(1);for(size_t i=0;i<brush.materials.size();++i)model.materials.skins[0].push_back(uint16_t(i));model.bodyModelCounts={1};model.bodyBases={1};
 for(size_t i=0;i<brush.positions.size();++i){auto&v=model.geometry.vertices[i];std::memcpy(v.data()+16,brush.positions[i].data(),12);std::memcpy(v.data()+28,brush.normals[i].data(),12);std::memcpy(v.data()+40,brush.uvs[i].data(),8);}
 model.meshes.resize(brush.materials.size());for(size_t i=0;i<model.meshes.size();++i){model.meshes[i].body=0;model.meshes[i].model=0;model.meshes[i].mesh=uint32_t(i);model.meshes[i].material=int32_t(i);}for(const auto&t:brush.triangles){if(t.material>=model.meshes.size())throw std::runtime_error("BSP static material index");model.meshes[t.material].triangles.push_back(t.vertices);}return model;
}
inline SourceBspBrushGeometryCandidate readSourceBspInlineBrushCandidate(const std::vector<uint8_t>&b,size_t modelIndex){
 auto range=[&](uint64_t p,uint64_t n){if(p>b.size()||n>b.size()-p)throw std::runtime_error("BSP data range");};
 auto u32=[&](size_t p){range(p,4);uint32_t v;std::memcpy(&v,b.data()+p,4);return v;};
 auto i32=[&](size_t p){uint32_t u=u32(p);int32_t v;std::memcpy(&v,&u,4);return v;};
 auto u16=[&](size_t p){range(p,2);uint16_t v;std::memcpy(&v,b.data()+p,2);return v;};
 auto i16=[&](size_t p){uint16_t u=u16(p);int16_t v;std::memcpy(&v,&u,2);return v;};
 auto f32=[&](size_t p){uint32_t u=u32(p);float v;std::memcpy(&v,&u,4);if(!std::isfinite(v))throw std::runtime_error("BSP nonfinite float");return v;};
 if(b.size()<1036||std::memcmp(b.data(),"VBSP",4))throw std::runtime_error("VBSP header required");
 std::array<SourceBspBrushLumpCandidate,64>lumps{};for(size_t n=0;n<64;++n){size_t p=8+n*16;lumps[n]={u32(p),u32(p+4),i32(p+8),i32(p+12)};range(lumps[n].offset,lumps[n].size);}
 // Keep face light offsets/styles paired with the selected lighting dataset.
 // Preserve the existing LDR preference and legacy HDR-without-HDR-faces fallback.
 const bool hdrLighting=!lumps[8].size&&lumps[53].size;
 auto&models=lumps[14];auto&faces=lumps[hdrLighting&&lumps[58].size?58:7];if(models.size%48||faces.version!=1||faces.size%56)throw std::runtime_error("unsupported VBSP model/face layout");if(modelIndex>=models.size/48)throw std::runtime_error("VBSP inline model index");
 auto&vertices=lumps[3];auto&edges=lumps[12];auto&surfedges=lumps[13];auto&texinfo=lumps[6];auto&texdata=lumps[2];auto&strings=lumps[43];auto&stringTable=lumps[44];auto&lighting=lumps[hdrLighting?53:8];auto&dispInfo=lumps[26];auto&dispVerts=lumps[33];auto&dispTris=lumps[48];
 if(vertices.size%12||edges.size%4||surfedges.size%4||texinfo.size%72||texdata.size%32||stringTable.size%4||dispInfo.size%176||dispVerts.size%20||dispTris.size%2)throw std::runtime_error("VBSP geometry lump alignment");
 size_t m=models.offset+modelIndex*48;int32_t firstFace=i32(m+40),faceCount=i32(m+44);if(firstFace<0||faceCount<0||uint64_t(firstFace)+faceCount>faces.size/56)throw std::runtime_error("VBSP model face span");
 SourceBspBrushGeometryCandidate out;for(size_t c=0;c<3;++c){out.boundsMin[c]=f32(m+c*4);out.boundsMax[c]=f32(m+12+c*4);out.modelOrigin[c]=f32(m+24+c*4);if(out.boundsMin[c]>out.boundsMax[c])throw std::runtime_error("VBSP model bounds");}
 auto vertexAt=[&](uint32_t index){if(index>=vertices.size/12)throw std::runtime_error("VBSP vertex index");std::array<float,3>v{};size_t p=vertices.offset+size_t(index)*12;for(size_t c=0;c<3;++c)v[c]=f32(p+c*4);return v;};
 auto materialName=[&](uint32_t texInfoIndex){if(texInfoIndex>=texinfo.size/72)throw std::runtime_error("VBSP texinfo index");size_t ti=texinfo.offset+size_t(texInfoIndex)*72;int32_t dataIndex=i32(ti+68);if(dataIndex<0||uint64_t(dataIndex)>=texdata.size/32)throw std::runtime_error("VBSP texdata index");size_t td=texdata.offset+size_t(dataIndex)*32;int32_t nameId=i32(td+12);uint32_t width=u32(td+16),height=u32(td+20),viewWidth=u32(td+24),viewHeight=u32(td+28);if(nameId<0||uint64_t(nameId)>=stringTable.size/4)throw std::runtime_error("VBSP material string id");int32_t relative=i32(stringTable.offset+size_t(nameId)*4);if(relative<0||uint32_t(relative)>=strings.size)throw std::runtime_error("VBSP material string offset");size_t p=strings.offset+uint32_t(relative),end=p;while(end<strings.offset+strings.size&&b[end])++end;if(end==strings.offset+strings.size)throw std::runtime_error("VBSP material string termination");if(!viewWidth)viewWidth=width?width:512;if(!viewHeight)viewHeight=height?height:512;return std::tuple<std::string,std::array<std::array<float,4>,2>,uint32_t,uint32_t>{std::string(reinterpret_cast<const char*>(b.data()+p),end-p),{{{{f32(ti),f32(ti+4),f32(ti+8),f32(ti+12)}},{{f32(ti+16),f32(ti+20),f32(ti+24),f32(ti+28)}}}},viewWidth,viewHeight};};
 auto sampleLight=[&](size_t face,size_t texInfoIndex,const std::array<float,3>&pos){
  std::array<float,3>sum{1,1,1};int32_t lightOffset=i32(face+20);if(lightOffset<0)return sum;
  int32_t minS=i32(face+28),minT=i32(face+32),width=i32(face+36),height=i32(face+40);if(width<0||height<0||width>4096||height>4096)throw std::runtime_error("VBSP lightmap dimensions");uint64_t stride=uint64_t(width+1)*uint64_t(height+1);if(!stride||stride>16u*1024u*1024u)throw std::runtime_error("VBSP lightmap sample count");
  size_t ti=texinfo.offset+texInfoIndex*72;std::array<float,2>coord{};for(size_t axis=0;axis<2;++axis){size_t v=ti+32+axis*16;coord[axis]=pos[0]*f32(v)+pos[1]*f32(v+4)+pos[2]*f32(v+8)+f32(v+12)-(axis?minT:minS);}
  for(float&v:coord)if(!std::isfinite(v))throw std::runtime_error("VBSP nonfinite lightmap coordinate");coord[0]=std::clamp(coord[0],0.f,float(width));coord[1]=std::clamp(coord[1],0.f,float(height));uint32_t x0=uint32_t(std::floor(coord[0])),y0=uint32_t(std::floor(coord[1])),x1=std::min<uint32_t>(x0+1,uint32_t(width)),y1=std::min<uint32_t>(y0+1,uint32_t(height));float fx=coord[0]-x0,fy=coord[1]-y0;std::array<uint8_t,4>styles{};for(size_t i=0;i<4;++i)styles[i]=b[face+16+i];sum={0,0,0};bool any=false;
  auto fetch=[&](uint64_t sample){uint64_t p=uint64_t(lighting.offset)+uint32_t(lightOffset)+sample*4;if(p+4>uint64_t(lighting.offset)+lighting.size)throw std::runtime_error("VBSP lightmap sample outside lighting lump");float exponent=std::ldexp(1.f,static_cast<int8_t>(b[size_t(p+3)]));return std::array<float,3>{b[size_t(p)]*(exponent/255.f),b[size_t(p+1)]*(exponent/255.f),b[size_t(p+2)]*(exponent/255.f)};};
  // lightOffset is a byte offset applied once by fetch; base counts samples.
  // Bumped faces interleave a base and three directional planes per style.
  // This preview samples the base plane; full normal-map lighting is separate.
  const uint64_t planesPerStyle=(u32(ti+64)&0x800u)?4u:1u;
  for(size_t style=0;style<4&&styles[style]!=255;++style){any=true;std::array<std::array<float,3>,4>samples{};const uint64_t base=style*planesPerStyle*stride;for(size_t q=0;q<4;++q){uint32_t x=(q&1)?x1:x0,y=(q&2)?y1:y0;samples[q]=fetch(base+uint64_t(y)*uint64_t(width+1)+x);}for(size_t c=0;c<3;++c){float top=samples[0][c]*(1-fx)+samples[1][c]*fx,bottom=samples[2][c]*(1-fx)+samples[3][c]*fx;sum[c]+=top*(1-fy)+bottom*fy;}}
  if(!any)sum={1,1,1};for(float value:sum)if(!std::isfinite(value)||value<0)throw std::runtime_error("VBSP invalid lightmap value");return sum;
 };
 std::map<std::string,uint32_t>materialIds;
 for(int32_t faceNo=0;faceNo<faceCount;++faceNo){size_t f=faces.offset+size_t(firstFace+faceNo)*56;int32_t firstEdge=i32(f+4);int16_t edgeCount=i16(f+8),texInfoIndex=i16(f+10),displacementIndex=i16(f+12);if(displacementIndex!=-1)++out.displacementFaces;if(firstEdge<0||edgeCount<3||uint64_t(firstEdge)+uint16_t(edgeCount)>surfedges.size/4)throw std::runtime_error("VBSP face edge span");if(texInfoIndex<0)throw std::runtime_error("VBSP face texinfo");auto[name,axes,w,h]=materialName(uint32_t(texInfoIndex));auto [mi,inserted]=materialIds.emplace(name,uint32_t(materialIds.size()));if(inserted)out.materials.push_back(name);
  std::vector<uint32_t>faceVertices;faceVertices.reserve(edgeCount);for(int16_t e=0;e<edgeCount;++e){int32_t se=i32(surfedges.offset+size_t(firstEdge+e)*4);if(se==INT32_MIN)throw std::runtime_error("VBSP surfedge overflow");uint32_t edge=uint32_t(se<0?-se:se);if(edge>=edges.size/4)throw std::runtime_error("VBSP edge index");size_t ep=edges.offset+size_t(edge)*4;uint32_t vertex=u16(ep+(se<0?2:0));faceVertices.push_back(vertex);}
  if(displacementIndex>=0){
   if(uint64_t(displacementIndex)>=dispInfo.size/176)throw std::runtime_error("VBSP displacement info index");size_t d=dispInfo.offset+size_t(displacementIndex)*176;uint32_t dispVertStart=u32(d+12),dispTriStart=u32(d+16);int32_t power=i32(d+20);uint16_t mapFace=u16(d+36);if(mapFace!=uint32_t(firstFace+faceNo)||power<1||power>4)throw std::runtime_error("VBSP displacement face identity or power");uint32_t side=(1u<<power)+1,quadCount=(side-1)*(side-1),vertexCount=side*side,triangleCount=quadCount*2;if(uint64_t(dispVertStart)+vertexCount>dispVerts.size/20||uint64_t(dispTriStart)+triangleCount>dispTris.size/2)throw std::runtime_error("VBSP displacement span");if(faceVertices.size()!=4)throw std::runtime_error("VBSP displacement base face is not a quad");
   std::array<std::array<float,3>,4>baseCorners{};for(size_t i=0;i<4;++i)baseCorners[i]=vertexAt(faceVertices[i]);std::array<float,3>start{f32(d),f32(d+4),f32(d+8)};size_t startCorner=0;float best=std::numeric_limits<float>::infinity();for(size_t i=0;i<4;++i){float dist=0;for(size_t c=0;c<3;++c){float delta=baseCorners[i][c]-start[c];dist+=delta*delta;}if(dist<best){best=dist;startCorner=i;}}if(best>1e-4f)throw std::runtime_error("VBSP displacement start position not on base face");auto c0=baseCorners[startCorner],c1=baseCorners[(startCorner+1)&3],c2=baseCorners[(startCorner+2)&3],c3=baseCorners[(startCorner+3)&3];
   if(out.positions.size()>UINT32_MAX-vertexCount)throw std::runtime_error("VBSP displacement vertex overflow");uint32_t first=uint32_t(out.positions.size());out.displacementVertices+=vertexCount;for(uint32_t y=0;y<side;++y){float fy=float(y)/(side-1);std::array<float,3>left{},right{};for(size_t c=0;c<3;++c){left[c]=c0[c]+(c1[c]-c0[c])*fy;right[c]=c3[c]+(c2[c]-c3[c])*fy;}for(uint32_t x=0;x<side;++x){float fx=float(x)/(side-1);size_t local=size_t(y)*side+x;std::array<float,3>base{},position{};for(size_t c=0;c<3;++c)base[c]=left[c]+(right[c]-left[c])*fx;size_t v=dispVerts.offset+size_t(dispVertStart+local)*20;float distance=f32(v+12),alpha=f32(v+16);for(size_t c=0;c<3;++c)position[c]=base[c]+f32(v+c*4)*distance;float u=(position[0]*axes[0][0]+position[1]*axes[0][1]+position[2]*axes[0][2]+axes[0][3])/float(w),uv=(position[0]*axes[1][0]+position[1]*axes[1][1]+position[2]*axes[1][2]+axes[1][3])/float(h);out.positions.push_back(position);out.normals.push_back({0,0,0});out.uvs.push_back({u,1.f-uv});out.lightColors.push_back(sampleLight(f,uint32_t(texInfoIndex),base));out.displacementAlphas.push_back(alpha);}}
   auto emitDispTriangle=[&](uint32_t a,uint32_t b0,uint32_t c,uint32_t tri){uint16_t tags=u16(dispTris.offset+size_t(dispTriStart+tri)*2);if(tags&(1u<<5)){++out.displacementTrianglesRemoved;return;}std::array<float,3>p=out.positions[first+a],q=out.positions[first+b0],r=out.positions[first+c];std::array<float,3>n{(q[1]-p[1])*(r[2]-p[2])-(q[2]-p[2])*(r[1]-p[1]),(q[2]-p[2])*(r[0]-p[0])-(q[0]-p[0])*(r[2]-p[2]),(q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])};float len=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(!std::isfinite(len)||len<1e-8f)throw std::runtime_error("VBSP degenerate displacement triangle");for(size_t c0=0;c0<3;++c0)n[c0]/=len;for(uint32_t vi:{a,b0,c})for(size_t c0=0;c0<3;++c0)out.normals[first+vi][c0]+=n[c0];out.triangles.push_back({{first+a,first+b0,first+c},mi->second});};
   for(uint32_t y=0;y+1<side;++y)for(uint32_t x=0;x+1<side;++x){uint32_t index=y*side+x,tri=2*(y*(side-1)+x);if(index&1){emitDispTriangle(index,index+1,index+side,tri);emitDispTriangle(index+1,index+side+1,index+side,tri+1);}else{emitDispTriangle(index,index+side+1,index+side,tri);emitDispTriangle(index,index+1,index+side+1,tri+1);}}
   for(uint32_t v=0;v<vertexCount;++v){auto&normal=out.normals[first+v];float len=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);if(!std::isfinite(len)||len<1e-8f)normal={0,0,1};else for(float&component:normal)component/=len;}
   continue;
  }
  std::vector<uint32_t>corners;corners.reserve(edgeCount);std::array<float,3>normal{};
   for(uint32_t vertex:faceVertices){auto pos=vertexAt(vertex);float u=(pos[0]*axes[0][0]+pos[1]*axes[0][1]+pos[2]*axes[0][2]+axes[0][3])/float(w);float v=1.f-(pos[0]*axes[1][0]+pos[1]*axes[1][1]+pos[2]*axes[1][2]+axes[1][3])/float(h);uint32_t ci=uint32_t(out.positions.size());out.positions.push_back(pos);out.uvs.push_back({u,v});out.normals.push_back({0,0,0});out.lightColors.push_back(sampleLight(f,uint32_t(texInfoIndex),pos));out.displacementAlphas.push_back(0.f);corners.push_back(ci);}
  for(size_t k=1;k+1<corners.size();++k){auto a=out.positions[corners[0]],bb=out.positions[corners[k]],c=out.positions[corners[k+1]];std::array<float,3>n{(bb[1]-a[1])*(c[2]-a[2])-(bb[2]-a[2])*(c[1]-a[1]),(bb[2]-a[2])*(c[0]-a[0])-(bb[0]-a[0])*(c[2]-a[2]),(bb[0]-a[0])*(c[1]-a[1])-(bb[1]-a[1])*(c[0]-a[0])};float length=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(!std::isfinite(length))throw std::runtime_error("nonfinite VBSP triangle normal");if(length<1e-8f){++out.skippedDegenerateTriangles;continue;}for(float&v0:n)v0/=length;for(size_t q:{size_t(0),k,k+1})out.normals[corners[q]]=n;out.triangles.push_back({{corners[0],corners[k],corners[k+1]},mi->second});}
 }
 return out;
}
