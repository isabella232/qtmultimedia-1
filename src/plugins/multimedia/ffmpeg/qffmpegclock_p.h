/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QFFMPEGCLOCK_P_H
#define QFFMPEGCLOCK_P_H

#include "qffmpeg_p.h"

#include <qelapsedtimer.h>
#include <qlist.h>
#include <qmutex.h>
#include <qmetaobject.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class ClockController;

// Clock runs in displayTime, ie. if playbackRate is not 1, it runs faster or slower
// than a regular clock. All methods take displayTime
// Exception: usecsTo() will return the real time that should pass until we will
// hit the requested display time
class Clock
{
    ClockController *controller = nullptr;
public:
    enum Type {
        SystemClock,
        AudioClock
    };
    Clock(ClockController *controller);
    virtual ~Clock();
    virtual Type type() const;

    float playbackRate() const;
    bool isMaster() const;

    // all times in usecs
    qint64 currentTime() const;
    qint64 seekTime() const;
    qint64 usecsTo(qint64 currentTime, qint64 displayTime);

protected:
    virtual void syncTo(qint64 usecs);
    virtual void setPlaybackRate(float rate, qint64 currentTime);
    virtual void setPaused(bool paused);

    qint64 timeUpdated(qint64 currentTime);

private:
    friend class ClockController;
    void setController(ClockController *c)
    {
        controller = c;
    }
};

class ClockController
{
    mutable QMutex m_mutex;
    QList<Clock *> m_clocks;
    Clock *m_master = nullptr;

    QElapsedTimer m_timer;
    qint64 m_baseTime = 0;
    qint64 m_seekTime = 0;
    float m_playbackRate = 1.;
    bool m_isPaused = true;

    qint64 m_lastMasterTime = 0;
    QObject *notifyObject = nullptr;
    QMetaMethod notify;
    qint64 currentTimeNoLock() const { return m_isPaused ? m_baseTime : m_baseTime + m_timer.elapsed()/m_playbackRate; }

    friend class Clock;
    qint64 timeUpdated(Clock *clock, qint64 time);
    void addClock(Clock *provider);
    void removeClock(Clock *provider);
public:
    // max 25 msecs tolerance for the clock
    enum { ClockTolerance = 25000 };
    ClockController() = default;
    ~ClockController();


    qint64 currentTime() const;

    void syncTo(qint64 usecs);

    void setPlaybackRate(float s);
    float playbackRate() const { return m_playbackRate; }
    void setPaused(bool paused);

    void setNotify(QObject *object, QMetaMethod method)
    {
        notifyObject = object;
        notify = method;
    }
};

inline float Clock::playbackRate() const
{
    return controller ? controller->m_playbackRate : 1.;
}

inline bool Clock::isMaster() const
{
    return controller ? controller->m_master == this : false;
}

inline qint64 Clock::seekTime() const
{
    return controller ? controller->m_seekTime : 0;
}


}

QT_END_NAMESPACE

#endif