/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef AUDIODEV_H
#define AUDIODEV_H

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>

#include <akaudiocaps.h>

class AudioDev: public QObject
{
    Q_OBJECT
    Q_ENUMS(DeviceMode)
    Q_PROPERTY(QString error
               READ error
               NOTIFY errorChanged)

    public:
        enum DeviceMode
        {
            DeviceModeCapture,
            DeviceModePlayback
        };

        explicit AudioDev(QObject *parent=NULL);
        ~AudioDev();

        Q_INVOKABLE QString error() const;
        Q_INVOKABLE bool preferredFormat(DeviceMode mode,
                                         AkAudioCaps::SampleFormat *sampleFormat,
                                         int *channels,
                                         int *sampleRate);
        Q_INVOKABLE bool init(DeviceMode mode,
                              AkAudioCaps::SampleFormat sampleFormat,
                              int channels,
                              int sampleRate);
        Q_INVOKABLE QByteArray read(int samples);
        Q_INVOKABLE bool write(const QByteArray &frame);
        Q_INVOKABLE bool uninit();

    private:
        QString m_error;
        AudioUnit m_audioUnit;
        UInt32 m_bufferSize;

        static QString statusToStr(OSStatus status);
        static AudioDeviceID defaultDevice(AudioDev::DeviceMode mode,
                                           bool *ok=NULL);
        static OSStatus audioCallback(void *audioDev,
                                      AudioUnitRenderActionFlags *actionFlags,
                                      const AudioTimeStamp *timeStamp,
                                      UInt32 busNumber,
                                      UInt32 nFrames,
                                      AudioBufferList *data);

    signals:
        void errorChanged(const QString &error);
};

#endif // AUDIODEV_H