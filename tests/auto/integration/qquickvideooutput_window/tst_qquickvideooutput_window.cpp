/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

//TESTED_COMPONENT=plugins/declarative/multimedia

#include "private/qquickvideooutput_p.h"
#include <QtCore/qobject.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <private/qplatformvideosink_p.h>

class SourceObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *mediaSource READ mediaSource CONSTANT)
public:
    explicit SourceObject(QObject *mediaSource, QObject *parent = nullptr)
        : QObject(parent), m_mediaSource(mediaSource)
    {}

    [[nodiscard]] QObject *mediaSource() const
    { return m_mediaSource; }

private:
    QObject *m_mediaSource;
};

class QtTestWindowControl : public QPlatformVideoSink
{
public:
    QtTestWindowControl(QVideoSink *parent = nullptr)
        : QPlatformVideoSink(parent)
    {}
    void setWinId(WId id) override { m_winId = id; }

    void setDisplayRect(const QRect &rect) override { m_displayRect = rect; }

    void setFullScreen(bool fullScreen) override { m_fullScreen = fullScreen; }

    [[nodiscard]] QSize nativeSize() const override { return m_nativeSize; }
    void setNativeSize(const QSize &size) { m_nativeSize = size; emit nativeSizeChanged(); }

    void setAspectRatioMode(Qt::AspectRatioMode mode) override { m_aspectRatioMode = mode; }

    void setBrightness(float brightness) override { m_brightness = brightness; }
    void setContrast(float contrast) override { m_contrast = contrast; }
    void setHue(float hue) override { m_hue = hue; }
    void setSaturation(float saturation) override { m_saturation = saturation; }

    [[nodiscard]] WId winId() const { return m_winId; }

    [[nodiscard]] QRect displayRect() const { return m_displayRect; }

    [[nodiscard]] bool isFullScreen() const { return m_fullScreen; }

    [[nodiscard]] Qt::AspectRatioMode aspectRatioMode() const { return m_aspectRatioMode; }

    [[nodiscard]] float brightness() const { return m_brightness; }
    [[nodiscard]] float contrast() const { return m_contrast; }
    [[nodiscard]] float hue() const { return m_hue; }
    [[nodiscard]] float saturation() const { return m_saturation; }

private:
    WId m_winId = 0;
    float m_brightness = 0;
    float m_contrast = 0;
    float m_hue = 0;
    float m_saturation = 0;
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;
    QRect m_displayRect;
    QSize m_nativeSize;
    bool m_fullScreen = false;
};

class QtTestVideoObject : public QObject
{
    Q_OBJECT
public:
    explicit QtTestVideoObject()
        : QObject(nullptr)
    {
    }
};

class tst_QQuickVideoOutputWindow : public QObject
{
    Q_OBJECT
public:
    tst_QQuickVideoOutputWindow()
        : QObject(nullptr)
        , m_sourceObject(&m_videoObject)
    {
    }

    ~tst_QQuickVideoOutputWindow() override
    = default;

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void winId();
    void nativeSize();
    void aspectRatio();
    void geometryChange();
    void resetCanvas();

private:
    QQmlEngine m_engine;
    QQuickItem *m_videoItem;
    QScopedPointer<QQuickItem> m_rootItem;
    QtTestWindowControl m_windowControl;
    QtTestVideoObject m_videoObject;
    SourceObject m_sourceObject;
    QQuickView m_view;
};

void tst_QQuickVideoOutputWindow::initTestCase()
{
    QQmlComponent component(&m_engine);
    component.loadUrl(QUrl("qrc:/main.qml"));

    m_rootItem.reset(qobject_cast<QQuickItem *>(component.create()));
    m_videoItem = m_rootItem->findChild<QQuickItem *>("videoOutput");
    QVERIFY(m_videoItem);
    m_rootItem->setParentItem(m_view.contentItem());
    m_videoItem->setProperty("source", QVariant::fromValue<QObject *>(&m_sourceObject));

    m_windowControl.setNativeSize(QSize(400, 200));
    m_view.resize(200, 200);
    m_view.show();
}

void tst_QQuickVideoOutputWindow::cleanupTestCase()
{
    // Make sure that QQuickVideoOutput doesn't segfault when it is being destroyed after
    // the service is already gone
    m_view.setSource(QUrl());
    m_rootItem.reset();
}

void tst_QQuickVideoOutputWindow::winId()
{
    QCOMPARE(m_windowControl.winId(), m_view.winId());
}

void tst_QQuickVideoOutputWindow::nativeSize()
{
    QCOMPARE(m_videoItem->implicitWidth(), qreal(400.0f));
    QCOMPARE(m_videoItem->implicitHeight(), qreal(200.0f));
}

void tst_QQuickVideoOutputWindow::aspectRatio()
{
    const QRect expectedDisplayRect(25, 50, 150, 100);
    m_videoItem->setProperty("fillMode", QQuickVideoOutput::Stretch);
    QTRY_COMPARE(m_windowControl.aspectRatioMode(), Qt::IgnoreAspectRatio);
    QCOMPARE(m_windowControl.displayRect(), expectedDisplayRect);

    m_videoItem->setProperty("fillMode", QQuickVideoOutput::PreserveAspectFit);
    QTRY_COMPARE(m_windowControl.aspectRatioMode(), Qt::KeepAspectRatio);
    QCOMPARE(m_windowControl.displayRect(), expectedDisplayRect);

    m_videoItem->setProperty("fillMode", QQuickVideoOutput::PreserveAspectCrop);
    QTRY_COMPARE(m_windowControl.aspectRatioMode(), Qt::KeepAspectRatioByExpanding);
    QCOMPARE(m_windowControl.displayRect(), expectedDisplayRect);
}

void tst_QQuickVideoOutputWindow::geometryChange()
{
    m_videoItem->setWidth(50);
    QTRY_COMPARE(m_windowControl.displayRect(), QRect(25, 50, 50, 100));

    m_videoItem->setX(30);
    QTRY_COMPARE(m_windowControl.displayRect(), QRect(30, 50, 50, 100));
}

void tst_QQuickVideoOutputWindow::resetCanvas()
{
    m_rootItem->setParentItem(nullptr);
    QCOMPARE((int)m_windowControl.winId(), 0);
}


QTEST_MAIN(tst_QQuickVideoOutputWindow)

#include "tst_qquickvideooutput_window.moc"