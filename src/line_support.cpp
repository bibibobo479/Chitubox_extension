// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SSI
#include "native.hpp"
#include <QApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>
#include <QMetaProperty>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <chrono>
#include <mutex>
#include <filesystem>
using line::V;
static QString tr(const char*s){return QString::fromUtf8(s);}
static double now(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
struct Sample{void*group=nullptr;V p,n;bool onModel=false,valid=false;QPointF screen;double time=0;};
static std::mutex sampleMutex;
static Sample latest;
static QPointF mouseScreen;
extern "C" int capture(void*,const V&,const V&,bool) __asm__("_ZN12SupportGroup21onAddOneSupportByHandERK9FVector3DS2_b");
extern "C" int capture(void*g,const V&p,const V&n,bool support){
 static auto original=reinterpret_cast<int(*)(void*,const V&,const V&,bool)>(dlsym(RTLD_NEXT,"_ZN12SupportGroup21onAddOneSupportByHandERK9FVector3DS2_b"));
 if(!original)return 0;
 int result=original(g,p,n,support);
 std::lock_guard<std::mutex> lock(sampleMutex);latest={g,p,n,support,line::finite(p)&&line::finite(n),mouseScreen,now()};return result;
}
static QQuickItem* renderIn(QQuickItem*i){if(i->inherits("MainRenderItemBase")&&i->isVisible())return i;for(auto*c:i->childItems())if(auto*r=renderIn(c))return r;return nullptr;}
static QString addonDir(){
 Dl_info info{};if(dladdr(reinterpret_cast<void*>(&addonDir),&info)&&info.dli_fname)return QFileInfo(QString::fromLocal8Bit(info.dli_fname)).absolutePath();
 return QString();
}
static bool compatible(){
 Dl_info info{};auto symbol=dlsym(RTLD_DEFAULT,"_ZN14SupportManager11GetInstanceEv");
 if(!symbol||!dladdr(symbol,&info)||!info.dli_fname)return false;
 auto libraryDir=QFileInfo(QString::fromLocal8Bit(info.dli_fname)).absolutePath();
 const char*files[]={"libCbdKernel.so.1","libCbdModel.so.1"};
 const char*hashes[]={"5dbcedd7f0e90b91f7e0e34c4b98b48506fb891fda6b131921e2beaf34c241b1","05185ca2e4c76d6142ee3450e6efd43dae7ea22e0fc3a9150fc27b99180bf711"};
 for(int i=0;i<2;i++){QFile f(libraryDir+tr("/")+tr(files[i]));if(!f.open(QIODevice::ReadOnly))return false;QCryptographicHash h(QCryptographicHash::Sha256);if(!h.addData(&f)||h.result().toHex()!=hashes[i])return false;}return true;
}
static bool regular(){auto p=native::symbol<std::shared_ptr<void>(*)()>("_ZN11SupportPara11GetInstanceEv")();int state[2];std::memcpy(state,p.get(),sizeof(state));return state[0]==0&&state[1]==0;}
class LineSupportController final:public QObject{
 QPointer<QQuickWindow> main;
 std::shared_ptr<QObject> manager;
 QPointer<QQuickItem> sidebar;
 QPointer<QQmlComponent> pending;QPointer<QQuickItem> pendingButton;
 bool expanded=true,includeEnd=true;double spacing=2;int lastStart=0,lastCancel=0;
 QString result=tr("Выберите две точки на поверхности модели.");
 bool armed=false,swallowRelease=false,wasSupport=false,busy=false;
 Sample first;bool haveFirst=false;
 QString config=QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+tr("/chitubox-extension/settings.conf");
 static void set(QObject*o,const char*name,const QVariant&v){if(!o)return;int i=o->metaObject()->indexOfProperty(name);if(i>=0)o->metaObject()->property(i).write(o,v);}
 void message(const QString&s){result=s;set(sidebar,"resultText",s);}
 void save(){std::error_code error;std::filesystem::create_directories(QFileInfo(config).absolutePath().toLocal8Bit().constData(),error);QFile f(config);if(f.open(QIODevice::WriteOnly))f.write(QByteArray::number(spacing)+" "+QByteArray::number(int(includeEnd))+" "+QByteArray::number(int(expanded))+"\n");}
 double stepValue()const{return sidebar?sidebar->property("stepValue").toDouble():spacing;}
 bool endValue()const{return sidebar?sidebar->property("includeEnd").toBool():includeEnd;}
 void reveal(){
  if(!sidebar)return;
  set(sidebar,"expanded",true);
  for(auto*p=sidebar->parentItem();p;p=p->parentItem())if(p->inherits("QQuickFlickable")){
   auto value=p->property("contentItem");auto*content=*static_cast<QQuickItem*const*>(value.constData());
   double y=sidebar->mapToItem(content,QPointF()).y();double maxY=p->property("contentHeight").toDouble()-p->height();set(p,"contentY",std::clamp(y,0.0,std::max(0.0,maxY)));break;
  }
 }
 static bool nativeToolAt(QQuickItem*i,QPointF scene){
  if(!i->isVisible()||!i->isEnabled())return false;
  if(i->contains(i->mapFromScene(scene))){
   for(const char*property:{"btnImgUrl","imageSource"}){auto name=i->property(property).toString();
    for(const char*icon:{"ManualAddSupport","ManualDeleteSupport","ManualEditSupport","ManualSmartSupport","ManualDirectSupport"})if(name.contains(tr(icon)))return true;
   }
  }
  for(auto*c:i->childItems())if(nativeToolAt(c,scene))return true;
  return false;
 }
 static QQuickItem* findAuto(QQuickItem*i){if(i->objectName()==tr("event_auto_support")&&i->isVisible())return i;for(auto*c:i->childItems())if(auto*f=findAuto(c))return f;return nullptr;}
 void attach(){
  if(sidebar||!main||!inSupport())return;
  auto*button=findAuto(main->contentItem());if(!button)return;
  if(pendingButton!=button){if(pending)pending->deleteLater();pending=nullptr;pendingButton=button;}
  if(!pending){
   auto*engine=qmlEngine(button);if(!engine)return;
   QFile f(addonDir()+tr("/sidebar.qml"));
   if(!f.open(QIODevice::ReadOnly)){message(tr("Не найден файл панели."));return;}
   pending=new QQmlComponent(engine,this);pending->setData(f.readAll(),QUrl());
  }
  if(pending->isLoading())return;
  if(pending->isError()){fprintf(stderr,"Line-support QML: %s\n",pending->errorString().toUtf8().constData());return;}
  QObject*object=pending->create(qmlContext(button));
  auto*item=object&&object->inherits("QQuickItem")?static_cast<QQuickItem*>(object):nullptr;
  if(!item){fprintf(stderr,"Line-support QML creation failed: class=%s status=%d error=%s\n",object?object->metaObject()->className():"null",int(pending->status()),pending->errorString().toUtf8().constData());delete object;return;}
  pending->deleteLater();pending=nullptr;
  item->setParent(button->parentItem());item->setParentItem(button->parentItem());item->stackAfter(button);
  sidebar=item;lastStart=lastCancel=0;set(sidebar,"initialStep",spacing);set(sidebar,"includeEnd",includeEnd);set(sidebar,"expanded",expanded);set(sidebar,"resultText",result);
 }
 void tick(){
  if(!main)for(auto*w:QGuiApplication::allWindows())if(auto*q=qobject_cast<QQuickWindow*>(w))if(q->isVisible()&&renderIn(q->contentItem())){main=q;break;}
  bool active=inSupport();
  if(!active&&wasSupport)reset(tr("Выберите две точки на поверхности модели."));
  attach();
  if(sidebar){
   bool open=sidebar->property("expanded").toBool(),end=endValue();double step=stepValue();
   if(open!=expanded||end!=includeEnd||(std::isfinite(step)&&step>=0.2&&step<=100&&step!=spacing)){expanded=open;includeEnd=end;if(std::isfinite(step)&&step>=0.2&&step<=100)spacing=step;save();}
   int cancel=sidebar->property("cancelRequest").toInt();if(cancel!=lastCancel){lastCancel=cancel;if(armed)reset(tr("Выбор отменён."));}
   int start=sidebar->property("startRequest").toInt();if(start!=lastStart){lastStart=start;begin();}
  }else if(armed)reset(tr("Панель закрыта. Выбор отменён."));
  if(armed&&(!active||!regular()))reset(tr("Режим изменён. Выбор отменён."));
  wasSupport=active;
 }
 bool inSupport(){return manager&&manager->property("supportModeFlag").toBool();}
 void reset(const QString&s){armed=false;haveFirst=false;set(sidebar,"armed",false);message(s);}
 void begin(){
  if(!inSupport()){message(tr("Откройте вкладку поддержек CHITUBOX."));return;}
  if(!regular()){message(tr("Выберите обычные поддержки (Regular) и инструмент «Добавить»."));return;}
  if(armed){reset(tr("Ручные поддержки. Линейный инструмент выключен."));return;}
  armed=true;haveFirst=false;{std::lock_guard<std::mutex> lock(sampleMutex);latest.valid=false;}
  set(sidebar,"armed",true);message(tr("Точка 1: наведите курсор на модель и щёлкните левой кнопкой."));
 }
 void select(Sample s){
  if(!s.valid||!s.onModel||!native::modelOf(s.group)){message(tr("Наведите курсор на поверхность модели, дождитесь предпросмотра и щёлкните."));return;}
  if(!haveFirst){first=s;haveFirst=true;message(tr("Точка 1 выбрана. Теперь щёлкните точку 2 на той же модели."));return;}
  if(first.group!=s.group){message(tr("Обе точки должны находиться на одной модели."));return;}
  if(line::length(s.p-first.p)<0.1){message(tr("Точки слишком близко. Выберите точку 2 дальше."));return;}
  if(line::dot(line::unit(first.n),line::unit(s.n))<0){message(tr("Выберите точки на одной стороне модели."));return;}
  try{
   line::distances(line::length(s.p-first.p),stepValue(),endValue());
   busy=true;auto mesh=native::mesh(native::modelOf(s.group));
   if(!line::project(mesh,first.p,first.n,0.1).valid||!line::project(mesh,s.p,s.n,0.1).valid){busy=false;haveFirst=false;message(tr("Поверхность изменилась. Выберите точки заново."));return;}
   auto points=line::plan(mesh,first.p,s.p,first.n,s.n,stepValue(),endValue());
   haveFirst=false;
   int count=native::create(s.group,points);busy=false;
   message(tr("Создано: ")+QString::number(count)+tr(". Пропущено: ")+QString::number(int(points.size())-count)+tr(".\nСледующая линия: выберите точку 1. Ctrl+Z — отмена ряда."));
   save();
  }catch(const std::exception&e){busy=false;haveFirst=false;message(tr("Не удалось создать ряд: ")+tr(e.what()));}
 }
protected:
 bool eventFilter(QObject*o,QEvent*e)override{
  if(busy)return false;
  if(e->type()==QEvent::KeyPress){auto*k=static_cast<QKeyEvent*>(e);
   if(k->key()==Qt::Key_L&&k->modifiers()==(Qt::ControlModifier|Qt::ShiftModifier)){if(!k->isAutoRepeat()){attach();reveal();begin();}return true;}
   if(armed&&k->modifiers()==Qt::NoModifier&&(k->key()==Qt::Key_A||k->key()==Qt::Key_D||k->key()==Qt::Key_E)){reset(tr("Выбран другой инструмент."));}
   if(armed&&k->key()==Qt::Key_Escape){if(haveFirst){haveFirst=false;message(tr("Первая точка сброшена. Выберите точку 1 новой линии."));}else reset(tr("Ручные поддержки. Линейный инструмент выключен."));return true;}
  }
  if(!main||o!=main)return false;
  if(e->type()==QEvent::MouseMove){auto*m=static_cast<QMouseEvent*>(e);std::lock_guard<std::mutex> lock(sampleMutex);mouseScreen=m->globalPosition();latest.valid=false;}
  if(e->type()==QEvent::MouseButtonRelease&&swallowRelease&&static_cast<QMouseEvent*>(e)->button()==Qt::LeftButton){swallowRelease=false;return true;}
  if(!armed||e->type()!=QEvent::MouseButtonPress)return false;
  auto*m=static_cast<QMouseEvent*>(e);if(m->button()!=Qt::LeftButton||m->modifiers()!=Qt::NoModifier)return false;
  if(nativeToolAt(main->contentItem(),m->position())){reset(tr("Выбран другой инструмент."));return false;}
  auto*r=renderIn(main->contentItem());if(!r||!r->contains(r->mapFromScene(m->position())))return false;
  swallowRelease=true;
  if(!inSupport()||!regular()){reset(tr("Режим поддержек изменён. Выбор отменён."));return true;}
  Sample s;{std::lock_guard<std::mutex> lock(sampleMutex);s=latest;}
  if(now()-s.time>2.0||QLineF(s.screen,m->globalPosition()).length()>6)s.valid=false;
  select(s);return true;
 }
public:
 LineSupportController():QObject(qApp){
  QFile f(config);if(f.open(QIODevice::ReadOnly)){double step=2;int end=1,open=1;if(std::sscanf(f.readAll().constData(),"%lf %d %d",&step,&end,&open)>=2&&std::isfinite(step)&&step>=0.2&&step<=100){spacing=step;includeEnd=end!=0;expanded=open!=0;}}
  manager=native::symbol<std::shared_ptr<QObject>(*)()>("_ZN14SupportManager11GetInstanceEv")();
  qApp->installEventFilter(this);auto*t=new QTimer(this);QObject::connect(t,&QTimer::timeout,this,[this]{tick();});t->start(100);
 }
};
int QApplication::exec(){
 auto*timer=new QTimer(qApp);timer->setSingleShot(true);QObject::connect(timer,&QTimer::timeout,[timer]{timer->deleteLater();try{if(compatible())new LineSupportController;else fprintf(stderr,"Line-support: incompatible CHITUBOX libraries; disabled.\n");}catch(const std::exception&e){fprintf(stderr,"Line-support: %s\n",e.what());}});timer->start(2000);
 return reinterpret_cast<int(*)()>(dlsym(RTLD_NEXT,"_ZN12QApplication4execEv"))();
}
