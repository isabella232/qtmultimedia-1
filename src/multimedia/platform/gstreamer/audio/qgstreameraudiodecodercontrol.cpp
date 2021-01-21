/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
//#define DEBUG_DECODER

#include "qgstreameraudiodecodercontrol_p.h"
#include <private/qgstreamerbushelper_p.h>

#include <private/qgstutils_p.h>

#include <gst/gstvalue.h>
#include <gst/base/gstbasesrc.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qsize.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qurl.h>

#define MAX_BUFFERS_IN_QUEUE 4

QT_BEGIN_NAMESPACE

typedef enum {
    GST_PLAY_FLAG_VIDEO         = 0x00000001,
    GST_PLAY_FLAG_AUDIO         = 0x00000002,
    GST_PLAY_FLAG_TEXT          = 0x00000004,
    GST_PLAY_FLAG_VIS           = 0x00000008,
    GST_PLAY_FLAG_SOFT_VOLUME   = 0x00000010,
    GST_PLAY_FLAG_NATIVE_AUDIO  = 0x00000020,
    GST_PLAY_FLAG_NATIVE_VIDEO  = 0x00000040,
    GST_PLAY_FLAG_DOWNLOAD      = 0x00000080,
    GST_PLAY_FLAG_BUFFERING     = 0x000000100
} GstPlayFlags;

QGstreamerAudioDecoderControl::QGstreamerAudioDecoderControl(QObject *parent)
    : QAudioDecoderControl(parent),
     m_state(QAudioDecoder::StoppedState),
     m_pendingState(QAudioDecoder::StoppedState),
     m_busHelper(0),
     m_bus(0),
     m_playbin(0),
     m_outputBin(0),
     m_audioConvert(0),
     m_appSink(0),
#if QT_CONFIG(gstreamer_app)
     m_appSrc(0),
#endif
     mDevice(0),
     m_buffersAvailable(0),
     m_position(-1),
     m_duration(-1),
     m_durationQueries(0)
{
    // Create pipeline here
    m_playbin = gst_element_factory_make("playbin", NULL);

    if (m_playbin != 0) {
        // Sort out messages
        m_bus = gst_element_get_bus(m_playbin);
        m_busHelper = new QGstreamerBusHelper(m_bus, this);
        m_busHelper->installMessageFilter(this);

        // Set the rest of the pipeline up
        setAudioFlags(true);

        m_audioConvert = gst_element_factory_make("audioconvert", NULL);

        m_outputBin = gst_bin_new("audio-output-bin");
        gst_bin_add(GST_BIN(m_outputBin), m_audioConvert);

        // add ghostpad
        GstPad *pad = gst_element_get_static_pad(m_audioConvert, "sink");
        Q_ASSERT(pad);
        gst_element_add_pad(GST_ELEMENT(m_outputBin), gst_ghost_pad_new("sink", pad));
        gst_object_unref(GST_OBJECT(pad));

        g_object_set(G_OBJECT(m_playbin), "audio-sink", m_outputBin, NULL);
#if QT_CONFIG(gstreamer_app)
        g_signal_connect(G_OBJECT(m_playbin), "deep-notify::source", (GCallback) &QGstreamerAudioDecoderControl::configureAppSrcElement, (gpointer)this);
#endif

        // Set volume to 100%
        gdouble volume = 1.0;
        g_object_set(G_OBJECT(m_playbin), "volume", volume, NULL);
    }
}

QGstreamerAudioDecoderControl::~QGstreamerAudioDecoderControl()
{
    if (m_playbin) {
        stop();

        delete m_busHelper;
#if QT_CONFIG(gstreamer_app)
        delete m_appSrc;
#endif
        gst_object_unref(GST_OBJECT(m_bus));
        gst_object_unref(GST_OBJECT(m_playbin));
    }
}

#if QT_CONFIG(gstreamer_app)
void QGstreamerAudioDecoderControl::configureAppSrcElement(GObject* object, GObject *orig, GParamSpec *pspec, QGstreamerAudioDecoderControl* self)
{
    Q_UNUSED(object);
    Q_UNUSED(pspec);

    // In case we switch from appsrc to not
    if (!self->appsrc())
        return;

    GstElement *appsrc;
    g_object_get(orig, "source", &appsrc, NULL);

    if (!self->appsrc()->setup(appsrc))
        qWarning()<<"Could not setup appsrc element";

    g_object_unref(G_OBJECT(appsrc));
}
#endif

bool QGstreamerAudioDecoderControl::processBusMessage(const QGstreamerMessage &message)
{
    GstMessage* gm = message.rawMessage();
    if (gm) {
        if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_DURATION) {
            updateDuration();
        } else if (GST_MESSAGE_SRC(gm) == GST_OBJECT_CAST(m_playbin)) {
            switch (GST_MESSAGE_TYPE(gm))  {
            case GST_MESSAGE_STATE_CHANGED:
                {
                    GstState    oldState;
                    GstState    newState;
                    GstState    pending;

                    gst_message_parse_state_changed(gm, &oldState, &newState, &pending);

#ifdef DEBUG_DECODER
                    QStringList states;
                    states << "GST_STATE_VOID_PENDING" <<  "GST_STATE_NULL" << "GST_STATE_READY" << "GST_STATE_PAUSED" << "GST_STATE_PLAYING";

                    qDebug() << QString("state changed: old: %1  new: %2  pending: %3") \
                            .arg(states[oldState]) \
                            .arg(states[newState]) \
                                .arg(states[pending]) << "internal" << m_state;
#endif

                    QAudioDecoder::State prevState = m_state;

                    switch (newState) {
                    case GST_STATE_VOID_PENDING:
                    case GST_STATE_NULL:
                        m_state = QAudioDecoder::StoppedState;
                        break;
                    case GST_STATE_READY:
                        m_state = QAudioDecoder::StoppedState;
                        break;
                    case GST_STATE_PLAYING:
                        m_state = QAudioDecoder::DecodingState;
                        break;
                    case GST_STATE_PAUSED:
                        m_state = QAudioDecoder::DecodingState;

                        //gstreamer doesn't give a reliable indication the duration
                        //information is ready, GST_MESSAGE_DURATION is not sent by most elements
                        //the duration is queried up to 5 times with increasing delay
                        m_durationQueries = 5;
                        updateDuration();
                        break;
                    }

                    if (prevState != m_state)
                        emit stateChanged(m_state);
                }
                break;

            case GST_MESSAGE_EOS:
                m_pendingState = m_state = QAudioDecoder::StoppedState;
                emit finished();
                emit stateChanged(m_state);
                break;

            case GST_MESSAGE_ERROR: {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_error(gm, &err, &debug);
                    if (err->domain == GST_STREAM_ERROR && err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND)
                        processInvalidMedia(QAudioDecoder::FormatError, tr("Cannot play stream of type: <unknown>"));
                    else
                        processInvalidMedia(QAudioDecoder::ResourceError, QString::fromUtf8(err->message));
                    qWarning() << "Error:" << QString::fromUtf8(err->message);
                    g_error_free(err);
                    g_free(debug);
                }
                break;
            case GST_MESSAGE_WARNING:
                {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_warning (gm, &err, &debug);
                    qWarning() << "Warning:" << QString::fromUtf8(err->message);
                    g_error_free (err);
                    g_free (debug);
                }
                break;
#ifdef DEBUG_DECODER
            case GST_MESSAGE_INFO:
                {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_info (gm, &err, &debug);
                    qDebug() << "Info:" << QString::fromUtf8(err->message);
                    g_error_free (err);
                    g_free (debug);
                }
                break;
#endif
            default:
                break;
            }
        } else if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ERROR) {
            GError *err;
            gchar *debug;
            gst_message_parse_error(gm, &err, &debug);
            QAudioDecoder::Error qerror = QAudioDecoder::ResourceError;
            if (err->domain == GST_STREAM_ERROR) {
                switch (err->code) {
                    case GST_STREAM_ERROR_DECRYPT:
                    case GST_STREAM_ERROR_DECRYPT_NOKEY:
                        qerror = QAudioDecoder::AccessDeniedError;
                        break;
                    case GST_STREAM_ERROR_FORMAT:
                    case GST_STREAM_ERROR_DEMUX:
                    case GST_STREAM_ERROR_DECODE:
                    case GST_STREAM_ERROR_WRONG_TYPE:
                    case GST_STREAM_ERROR_TYPE_NOT_FOUND:
                    case GST_STREAM_ERROR_CODEC_NOT_FOUND:
                        qerror = QAudioDecoder::FormatError;
                        break;
                    default:
                        break;
                }
            } else if (err->domain == GST_CORE_ERROR) {
                switch (err->code) {
                    case GST_CORE_ERROR_MISSING_PLUGIN:
                        qerror = QAudioDecoder::FormatError;
                        break;
                    default:
                        break;
                }
            }

            processInvalidMedia(qerror, QString::fromUtf8(err->message));
            g_error_free(err);
            g_free(debug);
        }
    }

    return false;
}

QString QGstreamerAudioDecoderControl::sourceFilename() const
{
    return mSource;
}

void QGstreamerAudioDecoderControl::setSourceFilename(const QString &fileName)
{
    stop();
    mDevice = 0;
#if QT_CONFIG(gstreamer_app)
    if (m_appSrc)
        m_appSrc->deleteLater();
    m_appSrc = 0;
#endif

    bool isSignalRequired = (mSource != fileName);
    mSource = fileName;
    if (isSignalRequired)
        emit sourceChanged();
}

QIODevice *QGstreamerAudioDecoderControl::sourceDevice() const
{
    return mDevice;
}

void QGstreamerAudioDecoderControl::setSourceDevice(QIODevice *device)
{
    stop();
    mSource.clear();
    bool isSignalRequired = (mDevice != device);
    mDevice = device;
    if (isSignalRequired)
        emit sourceChanged();
}

void QGstreamerAudioDecoderControl::start()
{
    if (!m_playbin) {
        processInvalidMedia(QAudioDecoder::ResourceError, "Playbin element is not valid");
        return;
    }

    addAppSink();

    if (!mSource.isEmpty()) {
        g_object_set(G_OBJECT(m_playbin), "uri", QUrl::fromLocalFile(mSource).toEncoded().constData(), NULL);
    } else if (mDevice) {
#if QT_CONFIG(gstreamer_app)
        // make sure we can read from device
        if (!mDevice->isOpen() || !mDevice->isReadable()) {
            processInvalidMedia(QAudioDecoder::AccessDeniedError, "Unable to read from specified device");
            return;
        }

        if (!m_appSrc)
            m_appSrc = new QGstAppSrc(this);
        m_appSrc->setStream(mDevice);

        g_object_set(G_OBJECT(m_playbin), "uri", "appsrc://", NULL);
#endif
    } else {
        return;
    }

    // Set audio format
    if (m_appSink) {
        if (mFormat.isValid()) {
            setAudioFlags(false);
            GstCaps *caps = QGstUtils::capsForAudioFormat(mFormat);
            gst_app_sink_set_caps(m_appSink, caps);
            gst_caps_unref(caps);
        } else {
            // We want whatever the native audio format is
            setAudioFlags(true);
            gst_app_sink_set_caps(m_appSink, NULL);
        }
    }

    m_pendingState = QAudioDecoder::DecodingState;
    if (gst_element_set_state(m_playbin, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qWarning() << "GStreamer; Unable to start decoding process";
        m_pendingState = m_state = QAudioDecoder::StoppedState;

        emit stateChanged(m_state);
    }
}

void QGstreamerAudioDecoderControl::stop()
{
    if (m_playbin) {
        gst_element_set_state(m_playbin, GST_STATE_NULL);
        removeAppSink();

        QAudioDecoder::State oldState = m_state;
        m_pendingState = m_state = QAudioDecoder::StoppedState;

        // GStreamer thread is stopped. Can safely access m_buffersAvailable
        if (m_buffersAvailable != 0) {
            m_buffersAvailable = 0;
            emit bufferAvailableChanged(false);
        }

        if (m_position != -1) {
            m_position = -1;
            emit positionChanged(m_position);
        }

        if (m_duration != -1) {
            m_duration = -1;
            emit durationChanged(m_duration);
        }

        if (oldState != m_state)
            emit stateChanged(m_state);
    }
}

QAudioFormat QGstreamerAudioDecoderControl::audioFormat() const
{
    return mFormat;
}

void QGstreamerAudioDecoderControl::setAudioFormat(const QAudioFormat &format)
{
    if (mFormat != format) {
        mFormat = format;
        emit formatChanged(mFormat);
    }
}

QAudioBuffer QGstreamerAudioDecoderControl::read()
{
    QAudioBuffer audioBuffer;

    int buffersAvailable;
    {
        QMutexLocker locker(&m_buffersMutex);
        buffersAvailable = m_buffersAvailable;

        // need to decrement before pulling a buffer
        // to make sure assert in QGstreamerAudioDecoderControl::new_buffer works
        m_buffersAvailable--;
    }


    if (buffersAvailable) {
        if (buffersAvailable == 1)
            emit bufferAvailableChanged(false);

        const char* bufferData = 0;
        int bufferSize = 0;

        GstSample *sample = gst_app_sink_pull_sample(m_appSink);
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo mapInfo;
        gst_buffer_map(buffer, &mapInfo, GST_MAP_READ);
        bufferData = (const char*)mapInfo.data;
        bufferSize = mapInfo.size;
        QAudioFormat format = QGstUtils::audioFormatForSample(sample);

        if (format.isValid()) {
            // XXX At the moment we have to copy data from GstBuffer into QAudioBuffer.
            // We could improve performance by implementing QAbstractAudioBuffer for GstBuffer.
            qint64 position = getPositionFromBuffer(buffer);
            audioBuffer = QAudioBuffer(QByteArray((const char*)bufferData, bufferSize), format, position);
            position /= 1000; // convert to milliseconds
            if (position != m_position) {
                m_position = position;
                emit positionChanged(m_position);
            }
        }
        gst_buffer_unmap(buffer, &mapInfo);
        gst_sample_unref(sample);
    }

    return audioBuffer;
}

bool QGstreamerAudioDecoderControl::bufferAvailable() const
{
    QMutexLocker locker(&m_buffersMutex);
    return m_buffersAvailable > 0;
}

qint64 QGstreamerAudioDecoderControl::position() const
{
    return m_position;
}

qint64 QGstreamerAudioDecoderControl::duration() const
{
     return m_duration;
}

void QGstreamerAudioDecoderControl::processInvalidMedia(QAudioDecoder::Error errorCode, const QString& errorString)
{
    stop();
    emit error(int(errorCode), errorString);
}

GstFlowReturn QGstreamerAudioDecoderControl::new_sample(GstAppSink *, gpointer user_data)
{
    // "Note that the preroll buffer will also be returned as the first buffer when calling gst_app_sink_pull_buffer()."
    QGstreamerAudioDecoderControl *control = reinterpret_cast<QGstreamerAudioDecoderControl*>(user_data);

    int buffersAvailable;
    {
        QMutexLocker locker(&control->m_buffersMutex);
        buffersAvailable = control->m_buffersAvailable;
        control->m_buffersAvailable++;
        Q_ASSERT(control->m_buffersAvailable <= MAX_BUFFERS_IN_QUEUE);
    }

    if (!buffersAvailable)
        QMetaObject::invokeMethod(control, "bufferAvailableChanged", Qt::QueuedConnection, Q_ARG(bool, true));
    QMetaObject::invokeMethod(control, "bufferReady", Qt::QueuedConnection);
    return GST_FLOW_OK;
}

void QGstreamerAudioDecoderControl::setAudioFlags(bool wantNativeAudio)
{
    int flags = 0;
    if (m_playbin) {
        g_object_get(G_OBJECT(m_playbin), "flags", &flags, NULL);
        // make sure not to use GST_PLAY_FLAG_NATIVE_AUDIO unless desired
        // it prevents audio format conversion
        flags &= ~(GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_TEXT | GST_PLAY_FLAG_VIS | GST_PLAY_FLAG_NATIVE_AUDIO);
        flags |= GST_PLAY_FLAG_AUDIO;
        if (wantNativeAudio)
            flags |= GST_PLAY_FLAG_NATIVE_AUDIO;
        g_object_set(G_OBJECT(m_playbin), "flags", flags, NULL);
    }
}

void QGstreamerAudioDecoderControl::addAppSink()
{
    if (m_appSink)
        return;

    m_appSink = (GstAppSink*)gst_element_factory_make("appsink", NULL);

    GstAppSinkCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.new_sample = &new_sample;
    gst_app_sink_set_callbacks(m_appSink, &callbacks, this, NULL);
    gst_app_sink_set_max_buffers(m_appSink, MAX_BUFFERS_IN_QUEUE);
    gst_base_sink_set_sync(GST_BASE_SINK(m_appSink), FALSE);

    gst_bin_add(GST_BIN(m_outputBin), GST_ELEMENT(m_appSink));
    gst_element_link(m_audioConvert, GST_ELEMENT(m_appSink));
}

void QGstreamerAudioDecoderControl::removeAppSink()
{
    if (!m_appSink)
        return;

    gst_element_unlink(m_audioConvert, GST_ELEMENT(m_appSink));
    gst_bin_remove(GST_BIN(m_outputBin), GST_ELEMENT(m_appSink));

    m_appSink = 0;
}

void QGstreamerAudioDecoderControl::updateDuration()
{
    gint64 gstDuration = 0;
    int duration = -1;

    if (m_playbin && qt_gst_element_query_duration(m_playbin, GST_FORMAT_TIME, &gstDuration))
        duration = gstDuration / 1000000;

    if (m_duration != duration) {
        m_duration = duration;
        emit durationChanged(m_duration);
    }

    if (m_duration > 0)
        m_durationQueries = 0;

    if (m_durationQueries > 0) {
        //increase delay between duration requests
        int delay = 25 << (5 - m_durationQueries);
        QTimer::singleShot(delay, this, SLOT(updateDuration()));
        m_durationQueries--;
    }
}

qint64 QGstreamerAudioDecoderControl::getPositionFromBuffer(GstBuffer* buffer)
{
    qint64 position = GST_BUFFER_TIMESTAMP(buffer);
    if (position >= 0)
        position = position / G_GINT64_CONSTANT(1000); // microseconds
    else
        position = -1;
    return position;
}

#if 0
QMultimedia::SupportEstimate QAudioDecoder::hasSupport(const QString &mimeType,
                                               const QStringList& codecs)
{
    // ### this code should not be there
    auto isDecoderOrDemuxer = [](GstElementFactory *factory) -> bool
    {
        return gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DEMUXER)
                || gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER
                                                           | GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO);
    };
    gst_init(nullptr, nullptr);
    auto set = QGstUtils::supportedMimeTypes(isDecoderOrDemuxer);
    return QGstUtils::hasSupport(mimeType, codecs, set);
}
#endif

QT_END_NAMESPACE
