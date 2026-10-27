// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SSI
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include <stdexcept>
#include <limits>
namespace line {
struct V { float x=0,y=0,z=0; };
inline V operator+(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V operator-(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V operator*(V a,double s){return {float(a.x*s),float(a.y*s),float(a.z*s)};}
inline double dot(V a,V b){return double(a.x)*b.x+double(a.y)*b.y+double(a.z)*b.z;}
inline V cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double length(V a){return std::sqrt(dot(a,a));}
inline V unit(V a){double n=length(a);return n>1e-9?a*(1/n):V{};}
inline bool finite(V a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
struct Triangle {V a,b,c;};
struct Hit {V p,n;bool valid=false;};
// Endpoint validation uses nearest hit. Row placement uses the first surface
// seen from outside in the outward normal direction, across the entire mesh.
// Thus protrusions occlude the underlying plane, regardless of their height.
inline Hit project(const std::vector<Triangle>& mesh,V p,V direction,double maxDistance,bool outermost=false){
 Hit hit; double best=outermost?-std::numeric_limits<double>::infinity():maxDistance+1e-6; auto d=unit(direction);
 for(const auto&t:mesh){auto e1=t.b-t.a,e2=t.c-t.a,n=unit(cross(e1,e2));if(dot(n,d)<0.15)continue;
 auto h=cross(d,e2);double det=dot(e1,h);if(std::abs(det)<1e-10)continue;
 auto s=p-t.a;double u=dot(s,h)/det;if(u < -1e-5 || u>1.00001)continue;
 auto q=cross(s,e1);double v=dot(d,q)/det;if(v < -1e-5 || u+v>1.00001)continue;
 double distance=dot(e2,q)/det;if(!std::isfinite(distance)||std::abs(distance)>maxDistance+1e-6)continue;
 if(outermost ? distance<=best : std::abs(distance)>best)continue;
 best=outermost?distance:std::abs(distance);hit={p+d*distance,n,true};
 }return hit;
}
inline std::vector<double> distances(double length,double step,bool end){
 if(!std::isfinite(length)||!std::isfinite(step)||length<0.1||step<0.2)throw std::runtime_error("Invalid length or step");
 if(length/step>499)throw std::runtime_error("Too many supports");
 std::vector<double> r;for(size_t i=0;i<=size_t(std::floor((length+1e-6)/step));i++)r.push_back(std::min(length,i*step));
 if(end&&length-r.back()>1e-4)r.push_back(length);
 return r;
}
inline std::vector<Hit> plan(const std::vector<Triangle>& mesh,V a,V b,V na,V nb,double step,bool end){
 double len=length(b-a);auto ds=distances(len,step,end);std::vector<Hit> result;
 if(mesh.size()*ds.size()>50000000)throw std::runtime_error("Too many triangles: increase support spacing");
 for(double d:ds){double t=d/len;auto n=unit(na*(1-t)+nb*t);result.push_back(project(mesh,a+(b-a)*t,n,std::numeric_limits<double>::infinity(),true));}return result;
}
}
