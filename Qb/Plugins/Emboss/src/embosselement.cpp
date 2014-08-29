/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "embosselement.h"

EmbossElement::EmbossElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetAzimuth();
    this->resetElevation();
    this->resetWidth45();
    this->resetPixelScale();
}

float EmbossElement::azimuth() const
{
    return this->m_azimuth;
}

float EmbossElement::elevation() const
{
    return this->m_elevation;
}

float EmbossElement::width45() const
{
    return this->m_width45;
}

float EmbossElement::pixelScale() const
{
    return this->m_pixelScale;
}

void EmbossElement::setAzimuth(float azimuth)
{
    this->m_azimuth = azimuth;
}

void EmbossElement::setElevation(float elevation)
{
    this->m_elevation = elevation;
}

void EmbossElement::setWidth45(float width45)
{
    this->m_width45 = width45;
}

void EmbossElement::setPixelScale(float pixelScale)
{
    this->m_pixelScale = pixelScale;
}

void EmbossElement::resetAzimuth()
{
    this->setAzimuth(135.0 / 360.0);
}

void EmbossElement::resetElevation()
{
    this->setElevation(30.0 / 90.0);
}

void EmbossElement::resetWidth45()
{
    this->setWidth45(10.0 / 40.0);
}

void EmbossElement::resetPixelScale()
{
    this->setPixelScale(255.9);
}

void EmbossElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void EmbossElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void EmbossElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_RGB32);

    int videoArea = width * height;

    QImage oFrame = QImage(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    if (packet.caps() != this->m_caps) {
        this->m_caps = packet.caps();
    }

    double azimuthInput = this->m_azimuth * 360.0;
    double elevationInput = this->m_elevation * 90.0;
    double widthInput = this->m_width45 * 40.0;

    // Force correct ranges on input
    azimuthInput = qBound(0.0, azimuthInput, 360.0);
    elevationInput = qBound(0.0, elevationInput, 90.0);
    widthInput = qBound(1.0, widthInput, 40.0);

    // Convert to filter input values
    float azimuth = azimuthInput * M_PI / 180.0;
    float elevation = elevationInput * M_PI / 180.0;
    float width45 = widthInput;

    // Create brightness image
    QVector<quint8> bumpPixels;
    QVector<quint8> alphaVals;

    for (int i = 0; i < videoArea; i++) {
        bumpPixels << qGray(srcBits[i]);
        alphaVals << qAlpha(srcBits[i]);
    }

    // Create embossed image from brightness image
    int Lx = cos(azimuth) * cos(elevation) * this->m_pixelScale;
    int Ly = sin(azimuth) * cos(elevation) * this->m_pixelScale;
    int Lz = sin(elevation) * this->m_pixelScale;

    int Nz = 6 * 255 / width45;
    int Nz2 = Nz * Nz;
    int NzLz = Nz * Lz;

    int background = Lz;
    int bumpIndex = 0;

    for (int i = 0, y = 0; y < height; y++, bumpIndex += width) {
        int s1 = bumpIndex;
        int s2 = s1 + width;
        int s3 = s2 + width;

        for (int x = 0; x < width; i++, x++, s1++, s2++, s3++) {
            int shade;

            if (y != 0
                && y < height - 2
                && x != 0
                && x < width - 2) {
                int Nx = bumpPixels[s1 - 1]
                     + bumpPixels[s2 - 1]
                     + bumpPixels[s3 - 1]
                     - bumpPixels[s1 + 1]
                     - bumpPixels[s2 + 1]
                     - bumpPixels[s3 + 1];

                int Ny = bumpPixels[s3 - 1]
                     + bumpPixels[s3]
                     + bumpPixels[s3 + 1]
                     - bumpPixels[s1 - 1]
                     - bumpPixels[s1]
                     - bumpPixels[s1 + 1];

                int NdotL = Nx * Lx + Ny * Ly + NzLz;

                if (Nx == 0 && Ny == 0)
                    shade = background;
                else if (NdotL < 0)
                    shade = 0;
                else
                    shade = NdotL / sqrt(Nx * Nx + Ny * Ny + Nz2);
            }
            else
                shade = background;

            shade = qBound(0, shade, 255);

            destBits[i] = qRgba(shade, shade, shade, alphaVals[s1]);
        }
    }

    QbBufferPtr oBuffer(new char[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "bgr0");
    caps.setProperty("width", oFrame.width());
    caps.setProperty("height", oFrame.height());

    QbPacket oPacket(caps,
                     oBuffer,
                     oFrame.byteCount());

    oPacket.setPts(packet.pts());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}