/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCAMERACONTROL_H
#define MOCKCAMERACONTROL_H

#include "private/qplatformcamera_p.h"
#include "qcamerainfo.h"
#include "qmockcameraimageprocessing.h"
#include <qtimer.h>

class QMockCamera : public QPlatformCamera
{
    friend class MockCaptureControl;
    Q_OBJECT

    static bool simpleCamera;
public:

    struct Simple {
        Simple() { simpleCamera = true; }
        ~Simple() { simpleCamera = false; }
    };

    QMockCamera(QCamera *parent)
        : QPlatformCamera(parent),
          m_status(QCamera::InactiveStatus),
          m_propertyChangesSupported(false)
    {
        if (!simpleCamera) {
            mockImageProcessing = new QMockCameraImageProcessing(this);
            minIsoChanged(100);
            maxIsoChanged(800);
            minShutterSpeedChanged(.001);
            maxShutterSpeedChanged(1);
            exposureCompensationRangeChanged(-2, 2);
            maximumZoomFactorChanged(4.);
            setFlashMode(QCamera::FlashAuto);
        }
    }

    ~QMockCamera() {}

    bool isActive() const override { return m_active; }
    void setActive(bool active) override {
        if (m_active == active)
            return;
        m_active = active;
        setStatus(active ? QCamera::ActiveStatus : QCamera::InactiveStatus);
        emit activeChanged(active);
    }

    QCamera::Status status() const override { return m_status; }

    /* helper method to emit the signal error */
    void setError(QCamera::Error err, QString errorString)
    {
        emit error(err, errorString);
    }

    /* helper method to emit the signal statusChaged */
    void setStatus(QCamera::Status newStatus)
    {
        m_status = newStatus;
        emit statusChanged(newStatus);
    }

    void setCamera(const QCameraInfo &camera) override
    {
        m_camera = camera;
    }

    void setFocusMode(QCamera::FocusMode mode) override
    {
        if (isFocusModeSupported(mode))
            focusModeChanged(mode);
    }
    bool isFocusModeSupported(QCamera::FocusMode mode) const override
    { return simpleCamera ? mode == QCamera::FocusModeAuto : mode != QCamera::FocusModeInfinity; }

    bool isCustomFocusPointSupported() const override { return !simpleCamera; }
    void setCustomFocusPoint(const QPointF &point) override
    {
        if (!simpleCamera)
            customFocusPointChanged(point);
    }

    void setFocusDistance(float d) override
    {
        if (!simpleCamera)
            focusDistanceChanged(d);
    }

    void zoomTo(float newZoomFactor, float /*rate*/) override { zoomFactorChanged(newZoomFactor); }

    void setFlashMode(QCamera::FlashMode mode) override
    {
        if (!simpleCamera)
            flashModeChanged(mode);
        flashReadyChanged(mode != QCamera::FlashOff);
    }
    bool isFlashModeSupported(QCamera::FlashMode mode) const override { return simpleCamera ? mode == QCamera::FlashOff : true; }
    bool isFlashReady() const override { return flashMode() != QCamera::FlashOff; }

    void setExposureMode(QCamera::ExposureMode mode) override
    {
        if (!simpleCamera && isExposureModeSupported(mode))
            exposureModeChanged(mode);
    }
    bool isExposureModeSupported(QCamera::ExposureMode mode) const override
    {
        return simpleCamera ? mode == QCamera::ExposureAuto : mode <= QCamera::ExposureBeach;
    }
    void setExposureCompensation(float c) override
    {
        if (!simpleCamera)
            exposureCompensationChanged(qBound(-2., c, 2.));
    }
    int isoSensitivity() const override
    {
        if (simpleCamera)
            return -1;
        return manualIsoSensitivity() > 0 ? manualIsoSensitivity() : 100;
    }
    void setManualIsoSensitivity(int iso) override
    {
        if (!simpleCamera)
            isoSensitivityChanged(qBound(100, iso, 800));
    }
    void setManualShutterSpeed(float secs) override
    {
        if (!simpleCamera)
            shutterSpeedChanged(qBound(0.001, secs, 1.));
    }
    float shutterSpeed() const override
    {
        if (simpleCamera)
            return -1.;
        return manualShutterSpeed() > 0 ? manualShutterSpeed() : .05;
    }

    QPlatformCameraImageProcessing *imageProcessingControl() override { return mockImageProcessing; }

    bool m_active = false;
    QCamera::Status m_status;
    QCameraInfo m_camera;
    bool m_propertyChangesSupported;

    QMockCameraImageProcessing *mockImageProcessing = nullptr;
};



#endif // MOCKCAMERACONTROL_H
