/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Quick3D Audio module of the Qt Toolkit.
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
#ifndef RESONANCE_AUDIO_H
#define RESONANCE_AUDIO_H

#include <api/resonance_audio_api.h>

namespace vraudio
{

class ResonanceAudioExtensions;
class ResonanceAudioApiImpl;

class EXPORT_API ResonanceAudio
{
public:
    ResonanceAudio(size_t num_channels, size_t frames_per_buffer, int sample_rate_hz);
    ~ResonanceAudio();

    // reverb is only calculated in stereo. We get it here as well, and our ambisonic
    // decoder will then add it to the generated surround signal.
    int getAmbisonicOutput(const float *buffers[], const float *reverb[], int nChannels);

    ResonanceAudioApi *api = nullptr;
    ResonanceAudioApiImpl *impl = nullptr;
    bool roomEffectsEnabled = true;
};



}

#endif
