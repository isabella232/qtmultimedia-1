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

#ifndef QAUDIODECODERCONTROL_H
#define QAUDIODECODERCONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qaudiodecoder.h>

#include <QtCore/qpair.h>

#include <QtMultimedia/qaudiobuffer.h>
#include <QtMultimedia/qaudiodecoder.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QIODevice;
class Q_MULTIMEDIA_EXPORT QPlatformAudioDecoder : public QObject
{
    Q_OBJECT

public:
    virtual QUrl source() const = 0;
    virtual void setSource(const QUrl &fileName) = 0;

    virtual QIODevice* sourceDevice() const = 0;
    virtual void setSourceDevice(QIODevice *device) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual QAudioFormat audioFormat() const = 0;
    virtual void setAudioFormat(const QAudioFormat &format) = 0;

    virtual QAudioBuffer read() = 0;
    virtual bool bufferAvailable() const { return m_bufferAvailable; }

    virtual qint64 position() const { return m_position; }
    virtual qint64 duration() const { return m_duration; }

    void formatChanged(const QAudioFormat &format);

    void sourceChanged();

    void error(int error, const QString &errorString);
    void clearError() { error(QAudioDecoder::NoError, QString()); }

    void bufferReady();
    void bufferAvailableChanged(bool available);
    void setIsDecoding(bool running = true) {
        if (m_isDecoding == running)
            return;
        m_isDecoding = running;
        emit q->isDecodingChanged(m_isDecoding);
    }
    void finished();
    bool isDecoding() const { return m_isDecoding; }

    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);

    QAudioDecoder::Error error() const { return m_error; }
    QString errorString() const { return m_errorString; }

protected:
    explicit QPlatformAudioDecoder(QAudioDecoder *parent);
private:
    QAudioDecoder *q = nullptr;

    qint64 m_duration = -1;
    qint64 m_position = -1;
    QAudioDecoder::Error m_error = QAudioDecoder::NoError;
    QString m_errorString;
    bool m_isDecoding = false;
    bool m_bufferAvailable = false;
};

QT_END_NAMESPACE

#endif  // QAUDIODECODERCONTROL_H
