/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Spatial Audio module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-NOGPL2$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSPATIALAUDIOENGINE_H
#define QSPATIALAUDIOENGINE_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QSpatialAudioEnginePrivate;
class QAudioDevice;

class Q_MULTIMEDIA_EXPORT QSpatialAudioEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(OutputMode outputMode READ outputMode WRITE setOutputMode NOTIFY outputModeChanged)
    Q_PROPERTY(QAudioDevice outputDevice READ outputDevice WRITE setOutputDevice NOTIFY outputDeviceChanged)
    Q_PROPERTY(float masterVolume READ masterVolume WRITE setMasterVolume NOTIFY masterVolumeChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
public:
    explicit QSpatialAudioEngine(QObject *parent = nullptr, int sampleRate = 44100);
    ~QSpatialAudioEngine();

    enum OutputMode {
        Normal,
        Headphone
    };
    Q_ENUM(OutputMode)

    void setOutputMode(OutputMode mode);
    OutputMode outputMode() const;

    int sampleRate() const;

    void setOutputDevice(const QAudioDevice &device);
    QAudioDevice outputDevice() const;

    void setMasterVolume(float volume);
    float masterVolume() const;

    void start();
    void stop();

    void setPaused(bool paused);
    bool paused() const;

    void setRoomEffectsEnabled(bool enabled);
    bool roomEffectsEnabled() const;

Q_SIGNALS:
    void outputModeChanged();
    void outputDeviceChanged();
    void masterVolumeChanged();
    void pausedChanged();

public Q_SLOTS:
    void pause() { setPaused(true); }
    void resume() { setPaused(false); }

private:
    friend class QSpatialAudioEnginePrivate;
    QSpatialAudioEnginePrivate *d;
};

QT_END_NAMESPACE

#endif