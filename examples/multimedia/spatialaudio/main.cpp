/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtWidgets/QtWidgets>
#include <QtMultimedia/QtMultimedia>

class AudioWidget : public QWidget
{
public:
    AudioWidget()
        : QWidget()
    {
        setWindowTitle(tr("Spatial Audio test application"));

        setMinimumSize(400, 300);
        auto *grid = new QGridLayout(this);
        fileEdit = new QLineEdit;
        fileDialogButton = new QPushButton(tr("Open Filedialog"));
        grid->addWidget(fileEdit, 0, 0);
        grid->addWidget(fileDialogButton, 0, 1);

        azimuth = new QSlider(Qt::Horizontal);
        azimuth->setRange(-180, 180);
        grid->addWidget(new QLabel(tr("Azimuth (-180 - 180 degree):")), 1, 0);
        grid->addWidget(azimuth, 1, 1);
        elevation = new QSlider(Qt::Horizontal);
        elevation->setRange(-90, 90);
        grid->addWidget(new QLabel(tr("Elevation (-90 - 90 degree)")), 2, 0);
        grid->addWidget(elevation, 2, 1);
        distance = new QSlider(Qt::Horizontal);
        distance->setRange(0, 10);
        grid->addWidget(new QLabel(tr("Distance (0 - 10 meter):")), 3, 0);
        grid->addWidget(distance, 3, 1);
        occlusion = new QSlider(Qt::Horizontal);
        occlusion->setRange(0, 200);
        grid->addWidget(new QLabel(tr("Occlusion (0 - 2):")), 4, 0);
        grid->addWidget(occlusion, 4, 1);

        roomDimension = new QSlider(Qt::Horizontal);
        roomDimension->setRange(0, 100);
        roomDimension->setValue(5);
        grid->addWidget(new QLabel(tr("Room dimension (0 - 100 meter):")), 5, 0);
        grid->addWidget(roomDimension, 5, 1);

        reverbGain = new QSlider(Qt::Horizontal);
        reverbGain->setRange(0, 500);
        reverbGain->setValue(100);
        grid->addWidget(new QLabel(tr("Reverb gain (0-5):")), 6, 0);
        grid->addWidget(reverbGain, 6, 1);

        reflectionGain = new QSlider(Qt::Horizontal);
        reflectionGain->setRange(0, 500);
        reflectionGain->setValue(100);
        grid->addWidget(new QLabel(tr("Reflection gain (0-5):")), 7, 0);
        grid->addWidget(reflectionGain, 7, 1);

        useHeadphone = new QCheckBox(tr("Use headphone spatialization"));
        grid->addWidget(useHeadphone, 8, 1, 1, 2);

        connect(fileEdit, &QLineEdit::textChanged, this, &AudioWidget::fileChanged);
        connect(fileDialogButton, &QPushButton::clicked, this, &AudioWidget::openFileDialog);

        connect(azimuth, &QSlider::valueChanged, this, &AudioWidget::newPosition);
        connect(elevation, &QSlider::valueChanged, this, &AudioWidget::newPosition);
        connect(distance, &QSlider::valueChanged, this, &AudioWidget::newPosition);
        connect(occlusion, &QSlider::valueChanged, this, &AudioWidget::newOcclusion);

        connect(roomDimension, &QSlider::valueChanged, this, &AudioWidget::updateRoom);
        connect(reverbGain, &QSlider::valueChanged, this, &AudioWidget::updateRoom);
        connect(reflectionGain, &QSlider::valueChanged, this, &AudioWidget::updateRoom);

        connect(useHeadphone, &QCheckBox::stateChanged, this, &AudioWidget::useHeadphoneChanged);

        room = new QSpatialAudioRoom(&engine);
        room->setDimensions(QVector3D(5, 5, 5));
        room->setWall(QSpatialAudioRoom::BackWall, QSpatialAudioRoom::Material::BrickBare);
        room->setWall(QSpatialAudioRoom::FrontWall, QSpatialAudioRoom::Material::BrickBare);
        room->setWall(QSpatialAudioRoom::LeftWall, QSpatialAudioRoom::Material::BrickBare);
        room->setWall(QSpatialAudioRoom::RightWall, QSpatialAudioRoom::Material::BrickBare);
        room->setWall(QSpatialAudioRoom::Floor, QSpatialAudioRoom::Material::Marble);
        room->setWall(QSpatialAudioRoom::Ceiling, QSpatialAudioRoom::Material::WoodCeiling);
        room->setReverbGain(1);
        room->setReflectionGain(1);

        listener = new QSpatialAudioListener(&engine);
        listener->setPosition({});
        listener->setRotation({});
        engine.start();

        sound = new QSpatialAudioSoundSource(&engine);

        distance->setValue(1);
    }
    void setFile(const QString &file) { fileEdit->setText(file); }
private slots:
    void newPosition()
    {
        float az = azimuth->value()/180.*M_PI;
        float el = elevation->value()/180.*M_PI;
        float d = distance->value()/10.;

        float x = d*sin(az)*cos(el);
        float y = d*cos(az)*cos(el);
        float z = d*sin(el);
        sound->setPosition({x, y, z});
    }
    void newOcclusion()
    {
        sound->setOcclusionIntensity(occlusion->value()/100.);
    }
    void useHeadphoneChanged(int state)
    {
        engine.setOutputMode(state ? QSpatialAudioEngine::Headphone : QSpatialAudioEngine::Normal);
    }
    void fileChanged(const QString &file)
    {
        sound->setSource(QUrl::fromLocalFile(file));
        sound->setMinimumDistance(5);
    }
    void openFileDialog()
    {
        auto file = QFileDialog::getOpenFileName(this);
        fileEdit->setText(file);
    }
    void updateRoom()
    {
        float d = roomDimension->value();
        room->setDimensions(QVector3D(d, d, 4));
        room->setReflectionGain(float(reflectionGain->value())/100);
        room->setReverbGain(float(reverbGain->value())/100);
    }

    QLineEdit *fileEdit = nullptr;
    QPushButton *fileDialogButton = nullptr;
    QSlider *azimuth = nullptr;
    QSlider *elevation = nullptr;
    QSlider *distance = nullptr;
    QSlider *occlusion = nullptr;
    QSlider *roomDimension = nullptr;
    QSlider *reverbGain = nullptr;
    QSlider *reflectionGain = nullptr;
    QCheckBox *useHeadphone = nullptr;

    QSpatialAudioEngine engine;
    QSpatialAudioListener *listener = nullptr;
    QSpatialAudioSoundSource *sound = nullptr;
    QSpatialAudioRoom *room = nullptr;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    AudioWidget *w = new AudioWidget;
    w->show();
    if (argc > 1) {
        auto file = QString::fromUtf8(argv[1]);
        w->setFile(file);
    }

    return app.exec();
}