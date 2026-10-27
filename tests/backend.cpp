// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SSI
#include <QWheelEvent>
#include <QMouseEvent>
#include <cassert>
extern "C" { int original_count=0, presses=0, moves=0, releases=0; double move_x=0,move_y=0; }
class MainRenderItemBase { public: void wheelEvent(QWheelEvent*); void mousePressEvent(QMouseEvent*); void mouseMoveEvent(QMouseEvent*); void mouseReleaseEvent(QMouseEvent*); };
static QPointF origin;
void MainRenderItemBase::wheelEvent(QWheelEvent* e){original_count++; e->ignore();}
void MainRenderItemBase::mousePressEvent(QMouseEvent* e){presses++; origin=e->position(); assert(e->button()==Qt::RightButton && e->modifiers()==Qt::NoModifier);}
void MainRenderItemBase::mouseMoveEvent(QMouseEvent* e){moves++; move_x=e->position().x()-origin.x();move_y=e->position().y()-origin.y();assert(e->buttons()==Qt::RightButton && e->modifiers()==Qt::NoModifier);}
void MainRenderItemBase::mouseReleaseEvent(QMouseEvent* e){releases++;assert(e->button()==Qt::RightButton && e->buttons()==Qt::NoButton && e->modifiers()==Qt::NoModifier);}
