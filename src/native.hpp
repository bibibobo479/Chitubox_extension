// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SSI
#pragma once
#include "geometry.hpp"
#include <dlfcn.h>
#include <memory>
#include <string>
#include <cstring>
#include <unordered_set>
namespace native {
using line::V;
template<class T>T symbol(const char*name){void*p=dlsym(RTLD_DEFAULT,name);if(!p)throw std::runtime_error(name);return reinterpret_cast<T>(p);}
inline std::vector<void*> models(){auto context=symbol<std::shared_ptr<void>(*)()>("_ZN15SoftwareContext11GetInstanceEv")();return symbol<std::vector<void*>(*)(void*,bool)>("_ZN15SoftwareContext14getModelStdVecEb")(context.get(),false);}
// Fixed ABI for the SHA256-pinned CHITUBOX 2.3.1 libraries. These two offsets
// were checked against native disassembly and live geometry; never use after upgrade.
inline void* groupOf(void* model){void*p=nullptr;std::memcpy(&p,static_cast<char*>(model)+0x778,sizeof(p));return p;}
inline void* modelOf(void* group){for(auto*m:models())if(groupOf(m)==group)return m;return nullptr;}
inline std::vector<line::Triangle> mesh(void*model){
 auto count=symbol<size_t(*)(void*)>("_ZN14VertexElements12triangleSizeEv")(static_cast<char*>(model)+0x2e8);
 if(count>2000000)throw std::runtime_error("Mesh exceeds 2 million triangles");
 auto tri=symbol<void(*)(size_t*,void*,size_t)>("_ZN12TriModelBase11getTriangleEm");
 // FVector3D has a nontrivial copy constructor: its native return uses an sret pointer.
 auto vert=symbol<void(*)(V*,void*,size_t)>("_ZN12TriModelBase9getVertexEm");
 auto transform=symbol<void(*)(void*,V*,void*)>("_ZN9ModelBase19transTriPntsNoApplyEP9FVector3DP7Support");
 std::vector<line::Triangle> out;out.reserve(count);
 for(size_t i=0;i<count;i++){size_t ids[3];V p[3];tri(ids,model,i);for(int j=0;j<3;j++)vert(&p[j],model,ids[j]);transform(model,p,nullptr);if(!line::finite(p[0])||!line::finite(p[1])||!line::finite(p[2]))throw std::runtime_error("Non-finite mesh");out.push_back({p[0],p[1],p[2]});}return out;
}
inline std::vector<void*> supports(void*g){return symbol<std::vector<void*>(*)(void*,bool)>("_ZN16SupportGroupBase13getAllSupportEb")(g,false);}
inline void clearPreview(void*g){symbol<void(*)(void*)>("_ZN16SupportGroupBase19clearRealTimeAddSupEv")(g);}
inline void record(void*g,std::vector<void*>&added){std::vector<void*> removed;symbol<void(*)(void*,int,std::vector<void*>&,std::vector<void*>&)>("_ZN12SupportGroup20oldNewSupsToUndoRedoEN19SupportRedoUndoData6OpNameERSt6vectorIP7SupportSaIS4_EES7_")(g,0,removed,added);}
inline int create(void*g,const std::vector<line::Hit>&points){
 auto add=symbol<int(*)(void*,const V&,const V&,bool)>("_ZN12SupportGroup21onAddOneSupportByHandERK9FVector3DS2_b");
 auto commit=symbol<void(*)(void*)>("_ZN16SupportGroupBase21realTimeSup2NormalSupEv");
 clearPreview(g);auto before=supports(g);std::unordered_set<void*> old(before.begin(),before.end());int count=0;
 for(auto&h:points){if(!h.valid)continue;clearPreview(g);if(add(g,h.p,h.n,false)==2){commit(g);count++;}}
 clearPreview(g);auto after=supports(g);std::vector<void*> added;for(auto*s:after)if(!old.count(s))added.push_back(s);
 if(!added.empty())record(g,added);
 return count;
}
}
