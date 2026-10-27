// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SSI
#include <QGuiApplication>
#include <QWheelEvent>
#include <QPointingDevice>
#include <cassert>
#include <cstdio>
#include <cstdlib>
extern "C" { extern int original_count,presses,moves,releases; extern double move_x,move_y; }
class MainRenderItemBase { public:void wheelEvent(QWheelEvent*); };
int main(int argc,char** argv){
 QGuiApplication app(argc,argv);
 MainRenderItemBase render;
 QPointingDevice pad("Test Pad",72,QInputDevice::DeviceType::TouchPad,QPointingDevice::PointerType::Finger,QInputDevice::Capability::Position|QInputDevice::Capability::Scroll,2,0);
 auto send=[&](Qt::KeyboardModifiers mods,QPoint delta=QPoint(20,12),Qt::MouseButtons buttons=Qt::NoButton,Qt::ScrollPhase phase=Qt::ScrollUpdate){
   QWheelEvent e(QPointF(200,200),QPointF(400,400),delta,QPoint(),buttons,mods,phase,false,Qt::MouseEventNotSynthesized,&pad);
   render.wheelEvent(&e);return e.isAccepted();
 };
 assert(!send(Qt::NoModifier) && original_count==1 && presses==0);
 assert(send(Qt::ControlModifier) && original_count==1 && presses==1 && moves==1 && releases==1);
 assert(move_x==15 && move_y==9);
 assert(!send(Qt::ShiftModifier) && original_count==2);
 assert(!send(Qt::ControlModifier|Qt::ShiftModifier) && original_count==3);
 assert(!send(Qt::ControlModifier,QPoint(10,10),Qt::LeftButton) && original_count==4);
 assert(send(Qt::ControlModifier,QPoint(0,0),Qt::NoButton,Qt::ScrollEnd) && presses==1);
 assert(send(Qt::ControlModifier,QPoint(10,10),Qt::NoButton,Qt::ScrollMomentum) && presses==1);
 setenv("CHITUBOX_TOUCHPAD_DISABLE","1",1);
 assert(!send(Qt::ControlModifier) && original_count==5);
 unsetenv("CHITUBOX_TOUCHPAD_DISABLE");
 {
   const int native_before=original_count;
   QWheelEvent mouse(QPointF(200,200),QPointF(400,400),QPoint(),QPoint(0,120),Qt::NoButton,Qt::ControlModifier,Qt::NoScrollPhase,false);
   render.wheelEvent(&mouse);
   assert(original_count==native_before+1);
   QWheelEvent legacyPad(QPointF(200,200),QPointF(400,400),QPoint(),QPoint(20,30),Qt::NoButton,Qt::ControlModifier,Qt::NoScrollPhase,false,Qt::MouseEventNotSynthesized,&pad);
   render.wheelEvent(&legacyPad);
   assert(original_count==native_before+1 && legacyPad.isAccepted());
 }
 for(int i=0;i<100;i++)assert(send(Qt::ControlModifier,QPoint(1,1)));
 assert(presses==moves && moves==releases);
 std::puts("PASS: Ctrl-only routing, both axes, unmodified native input, held-button safety, momentum, balanced drags, disable switch.");
}
