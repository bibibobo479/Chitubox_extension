// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SSI
// CHITUBOX Basic 2.3.1 Linux: Ctrl + touchpad -> existing view orbit.
// Only the 3D render item's wheel handler is wrapped. No global input injection.
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPointingDevice>
#include <dlfcn.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// The class layout is intentionally opaque: we only forward its existing ABI.
class MainRenderItemBase {
public:
    void wheelEvent(QWheelEvent *);
};

namespace {
using WheelHandler = void (*)(void *, QWheelEvent *);
using MouseHandler = void (*)(void *, QMouseEvent *);

class PadContacts {
    int fd = -1;
    int contactSlots = 0;
    std::chrono::steady_clock::time_point scanAfter{};
public:
    ~PadContacts() { if (fd >= 0) close(fd); }
    bool hasPair() {
        if (fd < 0 && std::chrono::steady_clock::now() >= scanAfter) {
            scanAfter = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            std::error_code error;
            for (const auto &entry : std::filesystem::directory_iterator("/sys/class/input", error)) {
                const std::string event = entry.path().filename().string();
                if (event.rfind("event", 0) != 0) continue;
                std::ifstream file(entry.path() / "device/name");
                std::string name; std::getline(file, name);
                std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
                // Never open a keyboard or unrelated input device.
                if (name.find("touchpad") == std::string::npos) continue;
                const int candidate = open(("/dev/input/" + event).c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
                if (candidate < 0) continue;
                input_absinfo info{};
                if (ioctl(candidate, EVIOCGABS(ABS_MT_SLOT), &info) == 0 &&
                        info.minimum == 0 && info.maximum >= 1 && info.maximum < 64) {
                    fd = candidate; contactSlots = info.maximum + 1; break;
                }
                close(candidate);
            }
        }
        if (fd < 0) return false;
        std::vector<int> ids(contactSlots + 1, -1);
        ids[0] = ABS_MT_TRACKING_ID;
        if (ioctl(fd, EVIOCGMTSLOTS(ids.size() * sizeof(int)), ids.data()) < 0) {
            close(fd); fd = -1; return false;
        }
        int active = 0;
        for (int i = 1; i <= contactSlots; ++i) active += ids[i] >= 0;
        return active == 2;
    }
};

bool isTouchpad(const QWheelEvent *event) {
    if (event->pointingDevice() && event->pointingDevice()->type() == QInputDevice::DeviceType::TouchPad)
        return true;
    if (!event->pixelDelta().isNull() || event->phase() != Qt::NoScrollPhase) return true;
    // XWayland can label touchpad scrolls as ordinary mouse wheel events.
    // Query only contact IDs, on demand; no raw key/position recording or daemon.
    static PadContacts contacts;
    return contacts.hasPair();
}

double speed() {
    static const double value = [] {
        const char *setting = std::getenv("CHITUBOX_TOUCHPAD_SPEED");
        if (!setting) return 1.0;
        char *end = nullptr;
        const double number = std::strtod(setting, &end);
        return end != setting && *end == '\0' && std::isfinite(number) ? std::clamp(number, .1, 4.0) : 1.0;
    }();
    return value;
}
}

void MainRenderItemBase::wheelEvent(QWheelEvent *event) {
    static auto original = reinterpret_cast<WheelHandler>(dlsym(RTLD_NEXT, "_ZN18MainRenderItemBase10wheelEventEP11QWheelEvent"));
    static auto press = reinterpret_cast<MouseHandler>(dlsym(RTLD_NEXT, "_ZN18MainRenderItemBase15mousePressEventEP11QMouseEvent"));
    static auto move = reinterpret_cast<MouseHandler>(dlsym(RTLD_NEXT, "_ZN18MainRenderItemBase14mouseMoveEventEP11QMouseEvent"));
    static auto release = reinterpret_cast<MouseHandler>(dlsym(RTLD_NEXT, "_ZN18MainRenderItemBase17mouseReleaseEventEP11QMouseEvent"));
    static thread_local void *previous = nullptr;
    static thread_local QPointF remainder;
    const char *disabled = std::getenv("CHITUBOX_TOUCHPAD_DISABLE");
    bool eligible = (!disabled || std::string(disabled) != "1") &&
        event->modifiers() == Qt::ControlModifier && event->buttons() == Qt::NoButton &&
        press && move && release;
    try { eligible = eligible && isTouchpad(event); }
    catch (...) { eligible = false; } // Device discovery must never break original input.
    if (!eligible) {
        previous = nullptr; remainder = {};
        if (original) original(this, event);
        return;
    }
    if (previous != this || event->phase() == Qt::ScrollBegin) remainder = {};
    previous = this;
    // Consume gesture tails instead of leaving the original zoom handler a tail.
    if (event->phase() == Qt::ScrollEnd || event->phase() == Qt::ScrollMomentum) {
        remainder = {}; event->accept(); return;
    }
    const bool pixels = !event->pixelDelta().isNull();
    QPointF delta = QPointF(pixels ? event->pixelDelta() : event->angleDelta()) * (pixels ? .75 : .15) * speed();
    delta += remainder;
    const double dx = std::trunc(std::clamp(delta.x(), -40.0, 40.0));
    const double dy = std::trunc(std::clamp(delta.y(), -40.0, 40.0));
    remainder = QPointF(delta.x() - std::trunc(delta.x()), delta.y() - std::trunc(delta.y()));
    if (dx == 0 && dy == 0) { event->accept(); return; }
    const QPointF offset(dx, dy);
    const QPointF local = event->position();
    const QPointF global = event->globalPosition();
    // These are delivered directly to the view. The desktop cursor and physical
    // buttons/Control key are never altered. Always finish the virtual drag.
    QMouseEvent down(QEvent::MouseButtonPress, local, global,
                     Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QMouseEvent drag(QEvent::MouseMove, local + offset, global + offset,
                     Qt::NoButton, Qt::RightButton, Qt::NoModifier);
    QMouseEvent up(QEvent::MouseButtonRelease, local + offset, global + offset,
                   Qt::RightButton, Qt::NoButton, Qt::NoModifier);
    press(this, &down);
    move(this, &drag);
    release(this, &up);
    event->accept();
}
