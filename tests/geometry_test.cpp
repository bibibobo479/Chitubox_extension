// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SSI
#include "../src/geometry.hpp"
#include <cassert>
#include <iostream>
using namespace line;
int main(){
 auto d=distances(10,3,true);assert((d==std::vector<double>{0,3,6,9,10}));assert(distances(9,3,true).size()==4);assert(distances(10,3,false).back()==9);assert(distances(1,2,true).size()==2);
 bool threw=false;try{distances(1000,0.2,true);}catch(...){threw=true;}assert(threw);
 std::vector<Triangle> plane{{{-10,-10,5},{10,10,5},{10,-10,5}},{{-10,-10,5},{-10,10,5},{10,10,5}}};
 auto p=plan(plane,{-8,0,5},{8,0,5},{0,0,-1},{0,0,-1},4,true);assert(p.size()==5);for(auto&h:p){assert(h.valid);assert(std::abs(h.p.z-5)<1e-5);}for(size_t i=1;i<p.size();i++)assert(std::abs(length(p[i].p-p[i-1].p)-4)<1e-5);
 assert(!project(plane,{15,0,5},{0,0,-1},5).valid);assert(!project(plane,{0,0,15},{0,0,-1},5).valid);assert(!project(plane,{0,0,5},{0,0,1},5).valid);
 // Gap between disconnected patches must not get supports.
 std::vector<Triangle> gap{{{-10,-10,5},{-2,10,5},{-2,-10,5}},{{-10,-10,5},{-10,10,5},{-2,10,5}},{{2,-10,5},{10,10,5},{10,-10,5}},{{2,-10,5},{2,10,5},{10,10,5}}};
 auto g=plan(gap,{-8,0,5},{8,0,5},{0,0,-1},{0,0,-1},4,true);assert(g[0].valid&&g[1].valid&&!g[2].valid&&g[3].valid&&g[4].valid);
 std::vector<Triangle> slope{{{-10,-10,-10},{10,10,10},{10,-10,10}},{{-10,-10,-10},{-10,10,-10},{10,10,10}}};
 auto h=project(slope,{2,0,0},unit({1,0,-1}),5);assert(h.valid&&std::abs(h.p.x-h.p.z)<1e-5);
 // A 40 mm protrusion must win over the nearer backing plane, not be skipped.
 auto boss=plane;boss.push_back({{-2,-10,-35},{2,10,-35},{2,-10,-35}});boss.push_back({{-2,-10,-35},{-2,10,-35},{2,10,-35}});
 auto bossHits=plan(boss,{-8,0,5},{8,0,5},{0,0,-1},{0,0,-1},4,true);
 assert(bossHits[2].valid&&std::abs(bossHits[2].p.z+35)<1e-5);
 assert(bossHits[0].valid&&std::abs(bossHits[0].p.z-5)<1e-5);
 // A deep recess is found too, without a distance cut-off.
 auto recess=gap;recess.push_back({{-2,-10,45},{2,10,45},{2,-10,45}});recess.push_back({{-2,-10,45},{-2,10,45},{2,10,45}});
 assert(std::abs(plan(recess,{-8,0,5},{8,0,5},{0,0,-1},{0,0,-1},4,true)[2].p.z-45)<1e-5);
 std::cout<<"PASS: spacing, endpoints, limits, plane, slope, holes, normals, projection distance\n";
}
