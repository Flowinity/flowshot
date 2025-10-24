//
// Created by troplo on 10/24/25.
//

#ifndef GENERALCONF2_H
#define GENERALCONF2_H



#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QScreen>
#include <QGuiApplication>
#include "../../../utils/flowinity/EndpointsJSON.h"
#include "../../../utils/abstractlogger.h"
#include "../../../utils/ConfigHandler.h"
#include "../../../uploader/imguploadermanager.h"

class GeneralConf : public QWidget
{
    Q_OBJECT
public:
    explicit GeneralConf(QWidget* parent = nullptr);

private:
    int m_openWindowCount;

    QVBoxLayout* m_scrollAreaLayout;

    // Upload client & server
    QLineEdit* m_uploadClientKey;
    QLineEdit* m_serverTPU;
    QLineEdit* m_uploadToken;
    QLabel* m_flowinityErrorMessage;

    // Custom environment
    QComboBox* m_selectPlatform;

    // Upload window
    QCheckBox* m_uploadWindowEnabled;
    QSpinBox* m_uploadWindowOffsetY;
    QSpinBox* m_uploadWindowOffsetX;
    QSpinBox* m_uploadWindowTimeout;
    QSpinBox* m_uploadWindowStackPadding;
    QSpinBox* m_uploadWindowScaleWidth;
    QSpinBox* m_uploadWindowScaleHeight;
    QCheckBox* m_uploadWindowImageEnabled;
    QCheckBox* m_uploadWindowButtonsEnabled;
    QSpinBox* m_uploadWindowImageWidth;
    QComboBox* m_selectDisplay;

    EndpointsJSON* m_endpoints;

    void initUploadClientSecret();
    void initServerTPU();
    void initCustomEnv();
    void initWindowOffsets();

private slots:
    void uploadClientKeyEdited();
    void serverTPUEdited();
    void uploadTokenTPUEdited();

    void uploadWindowEnabledEdited(bool checked);
    void uploadWindowOffsetYEdited(int value);
    void uploadWindowOffsetXEdited(int value);
    void uploadWindowTimeoutEdited(int value);
    void uploadWindowStackPaddingEdited(int value);
    void uploadWindowScaleWidthEdited(int value);
    void uploadWindowScaleHeightEdited(int value);
    void uploadWindowImageEnabledEdited();
    void uploadWindowButtonsEnabledEdited();
    void uploadWindowImageWidthEdited(int value);
    void uploadWindowDisplayEdited(int index);

    void saveServerTPU();
};




#endif //GENERALCONF2_H
