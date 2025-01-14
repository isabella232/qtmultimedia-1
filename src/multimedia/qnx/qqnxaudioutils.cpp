/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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

#include "qqnxaudioutils_p.h"

QT_BEGIN_NAMESPACE

namespace QnxAudioUtils
{

snd_pcm_channel_params_t formatToChannelParams(const QAudioFormat &format, QAudioDevice::Mode mode, int fragmentSize)
{
    snd_pcm_channel_params_t params;
    memset(&params, 0, sizeof(params));
    params.channel = (mode == QAudioDevice::Output) ? SND_PCM_CHANNEL_PLAYBACK : SND_PCM_CHANNEL_CAPTURE;
    params.mode = SND_PCM_MODE_BLOCK;
    params.start_mode = SND_PCM_START_DATA;
    params.stop_mode = SND_PCM_STOP_ROLLOVER;
    params.buf.block.frag_size = fragmentSize;
    params.buf.block.frags_min = 1;
    params.buf.block.frags_max = 1;
    strcpy(params.sw_mixer_subchn_name, "QAudio Channel");

    params.format.interleave = 1;
    params.format.rate = format.sampleRate();
    params.format.voices = format.channelCount();

    switch (format.sampleFormat()) {
    case QAudioFormat::UInt8:
        params.format.format = SND_PCM_SFMT_U8;
        break;
    default:
        // fall through
    case QAudioFormat::Int16:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        params.format.format = SND_PCM_SFMT_S16_LE;
#else
        params.format.format = SND_PCM_SFMT_S16_BE;
#endif
        break;
    case QAudioFormat::Int32:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        params.format.format = SND_PCM_SFMT_S32_LE;
#else
        params.format.format = SND_PCM_SFMT_S32_BE;
#endif
        break;
    case QAudioFormat::Float:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        params.format.format = SND_PCM_SFMT_FLOAT_LE;
#else
        params.format.format = SND_PCM_SFMT_FLOAT_BE;
#endif
        break;
    }

    return params;
}


HandleUniquePtr openPcmDevice(const QByteArray &id, QAudioDevice::Mode mode)
{
    const int pcmMode = mode == QAudioDevice::Output
        ? SND_PCM_OPEN_PLAYBACK
        : SND_PCM_OPEN_CAPTURE;

    snd_pcm_t *handle;

    const int errorCode = snd_pcm_open_name(&handle, id.constData(), pcmMode);

    if (errorCode != 0) {
        qWarning("Unable to open PCM device %s (0x%x)", id.constData(), -errorCode);
        return {};
    }

    return HandleUniquePtr { handle };
}

template <typename T, typename Func>
std::optional<T> pcmChannelGetStruct(snd_pcm_t *handle, QAudioDevice::Mode mode, Func &&func)
{
    // initialize in-place to prevent an extra copy when returning
    std::optional<T> t = { T{} };

    t->channel = mode == QAudioDevice::Output
        ? SND_PCM_CHANNEL_PLAYBACK
        : SND_PCM_CHANNEL_CAPTURE;

    const int errorCode = func(handle, &(*t));

    if (errorCode != 0) {
        qWarning("QAudioDevice: couldn't get channel info (0x%x)", -errorCode);
        return {};
    }

    return t;
}

template <typename T, typename Func>
std::optional<T> pcmChannelGetStruct(const QByteArray &device,
        QAudioDevice::Mode mode, Func &&func)
{
    const HandleUniquePtr handle = openPcmDevice(device, mode);

    if (!handle)
        return {};

    return pcmChannelGetStruct<T>(handle.get(), mode, std::forward<Func>(func));
}


std::optional<snd_pcm_channel_info_t> pcmChannelInfo(snd_pcm_t *handle, QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_info_t>(handle, mode, snd_pcm_plugin_info);
}

std::optional<snd_pcm_channel_info_t> pcmChannelInfo(const QByteArray &device,
        QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_info_t>(device, mode, snd_pcm_plugin_info);
}

std::optional<snd_pcm_channel_setup_t> pcmChannelSetup(snd_pcm_t *handle, QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_setup_t>(handle, mode, snd_pcm_plugin_setup);
}

std::optional<snd_pcm_channel_setup_t> pcmChannelSetup(const QByteArray &device,
        QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_setup_t>(device, mode, snd_pcm_plugin_setup);
}

std::optional<snd_pcm_channel_status_t> pcmChannelStatus(snd_pcm_t *handle, QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_status_t>(handle, mode, snd_pcm_plugin_status);
}

std::optional<snd_pcm_channel_status_t> pcmChannelStatus(const QByteArray &device,
        QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_status_t>(device, mode, snd_pcm_plugin_status);
}


} // namespace QnxAudioUtils

QT_END_NAMESPACE
