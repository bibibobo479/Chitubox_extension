// SPDX-License-Identifier: MIT
// Optional integration test: starts its own CHITUBOX profile and authored boss fixture.
#include "../src/native.hpp"
#include <QApplication>
#include <QTimer>
#include <QQuickWindow>
#include <QQuickItem>
#include <QMetaProperty>
#include <QMouseEvent>
#include <QKeyEvent>
#include <functional>
using native::symbol;
static QQuickWindow* window=nullptr;
static QQuickItem* render=nullptr;
static QQuickItem* sidebar=nullptr;
static size_t firstCount=0,secondCount=0;
static void* group=nullptr;
void later(int ms,std::function<void()> f){auto*t=new QTimer(qApp);t->setSingleShot(true);QObject::connect(t,&QTimer::timeout,[t,f]{f();t->deleteLater();});t->start(ms);}
void check(bool ok,const char* message){fprintf(stderr,"%s: %s\n",ok?"PASS":"FAIL",message);if(!ok){fflush(stderr);std::exit(1);}}
QQuickItem* find(QQuickItem*i,const char* name){if(i->objectName()==QString::fromUtf8(name))return i;for(auto*c:i->childItems())if(auto*r=find(c,name))return r;return nullptr;}
QQuickItem* canvas(QQuickItem*i){if(i->inherits("MainRenderItemBase")&&i->isVisible())return i;for(auto*c:i->childItems())if(auto*r=canvas(c))return r;return nullptr;}
void prop(QObject*o,const char*n,const QVariant&v){int i=o->metaObject()->indexOfProperty(n);check(i>=0,n);o->metaObject()->property(i).write(o,v);}
void move(QPointF p){QMouseEvent e(QEvent::MouseMove,p,window->mapToGlobal(p.toPoint()),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QCoreApplication::sendEvent(window,&e);}
void click(QPointF p){for(auto type:{QEvent::MouseButtonPress,QEvent::MouseButtonRelease}){QMouseEvent e(type,p,window->mapToGlobal(p.toPoint()),Qt::LeftButton,type==QEvent::MouseButtonPress?Qt::LeftButton:Qt::NoButton,Qt::NoModifier);QCoreApplication::sendEvent(window,&e);}}
void key(int k,Qt::KeyboardModifiers mods=Qt::NoModifier){QKeyEvent e(QEvent::KeyPress,k,mods);QCoreApplication::sendEvent(window,&e);}
void pick(QPointF p,std::function<void()> next){move(p-QPointF(1,0));later(500,[p,next]{move(p);later(500,[p,next]{click(p);later(300,next);});});}
void toggle(){key(Qt::Key_L,Qt::ControlModifier|Qt::ShiftModifier);}
bool armed(){return sidebar->property("armed").toBool();}
QQuickItem* manual(QQuickItem*i){if(i->isVisible()&&i->isEnabled()&&i->property("btnImgUrl").toString().contains(QString::fromUtf8("ManualAddSupport")))return i;for(auto*c:i->childItems())if(auto*r=manual(c))return r;return nullptr;}
void undoRedo(){auto job=symbol<std::shared_ptr<void>(*)()>("_ZN8JobProxy11GetInstanceEv")();symbol<void(*)(void*)>("_ZN8JobProxy13undoOperationEv")(job.get());later(1200,[]{check(native::supports(group).size()==firstCount,"one undo removes only second row");auto job=symbol<std::shared_ptr<void>(*)()>("_ZN8JobProxy11GetInstanceEv")();symbol<void(*)(void*)>("_ZN8JobProxy13redoOperationEv")(job.get());later(1200,[]{check(native::supports(group).size()==secondCount,"redo restores second row");auto*add=manual(window->contentItem());check(add,"native Add button located");click(add->mapToScene(QPointF(add->width()/2,add->height()/2)));later(300,[]{check(!armed(),"clicking native Add exits line tool");toggle();check(armed(),"line tool can be reselected");key(Qt::Key_D);check(!armed(),"native keyboard tool exits line tool");QCoreApplication::exit(0);});});});}
void secondRow(){check(armed(),"tool remains selected after first row");firstCount=native::supports(group).size();check(firstCount>30,"first row created");prop(sidebar,"expanded",false);later(300,[]{check(armed()&&sidebar->height()<60,"collapsing panel preserves tool");pick({216,490},[]{key(Qt::Key_Escape);check(armed(),"Escape clears first point without leaving tool");pick({216,510},[]{pick({281,510},[]{secondCount=native::supports(group).size();fprintf(stderr,"COUNTS first=%zu second=%zu\n",firstCount,secondCount);check(secondCount>firstCount,"second row created without reselecting tool");check(armed(),"tool remains selected after second row");key(Qt::Key_Escape);check(!armed(),"Escape without pending point exits tool");toggle();undoRedo();});});});});}
int QApplication::exec(){later(45000,[]{check(false,"GUI test timeout");});later(10000,[]{for(auto*w:QGuiApplication::allWindows())if(auto*q=qobject_cast<QQuickWindow*>(w))if(q->isVisible()&&(render=canvas(q->contentItem()))){window=q;break;}check(window,"model window found");click({double(window->width()-75),142});later(8000,[]{auto mgr=symbol<std::shared_ptr<QObject>(*)()>("_ZN14SupportManager11GetInstanceEv")();check(mgr->property("supportModeFlag").toBool(),"support workspace open");auto sixValue=render->property("sixViewItem");QObject*six=*static_cast<QObject*const*>(sixValue.constData());check(six,"view selector found");symbol<void(*)(void*,int)>("_ZN15SixViewItemBase22handleSixViewPickIndexEi")(six,5);later(1500,[]{sidebar=find(window->contentItem(),"ChituboxLineSupportSidebar");check(sidebar,"sidebar attached");auto*aut=find(window->contentItem(),"event_auto_support");check(aut&&sidebar->parentItem()==aut->parentItem()&&sidebar->y()>=aut->y()+aut->height(),"sidebar follows Auto Supports");auto models=native::models();check(models.size()==1,"one test model");group=native::groupOf(models[0]);check(native::supports(group).empty(),"fixture starts without supports");toggle();check(armed(),"line tool selected");pick({216,470},[]{pick({281,470},secondRow);});});});});return reinterpret_cast<int(*)()>(dlsym(RTLD_NEXT,"_ZN12QApplication4execEv"))();}
