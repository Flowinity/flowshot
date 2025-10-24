//
// Created by troplo on 10/24/25.
//

#include "generalconf2.h"

GeneralConf::GeneralConf(QWidget* parent)
    : QWidget(parent), m_endpoints(new EndpointsJSON(this))
{
    m_openWindowCount = 0;
    auto* scrollArea = new QScrollArea(this);
    auto* scrollWidget = new QWidget();
    m_scrollAreaLayout = new QVBoxLayout(scrollWidget);

    scrollWidget->setLayout(m_scrollAreaLayout);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);

    initServerTPU();
    initWindowOffsets();
}

void GeneralConf::initServerTPU()
{
    auto* box = new QGroupBox(tr("Flowinity"));
    auto* box2 = new QGroupBox();
    box->setFlat(true);
    box2->setFlat(true);
    m_scrollAreaLayout->addWidget(box);
    m_scrollAreaLayout->addWidget(box2);

    auto* hboxLayout = new QHBoxLayout();
    auto* hboxLayoutKey = new QHBoxLayout();
    box->setLayout(hboxLayout);
    box2->setLayout(hboxLayoutKey);

    m_serverTPU = new QLineEdit(this);
    QString foreground = this->palette().windowText().color().name();
    m_serverTPU->setStyleSheet(QStringLiteral("color: %1").arg(foreground));
    m_serverTPU->setText(ConfigHandler().serverTPU());

    m_flowinityErrorMessage = new QLabel(this);
    m_flowinityErrorMessage->setWordWrap(true);
    m_flowinityErrorMessage->setStyleSheet(QStringLiteral("color: %1").arg(foreground));
    m_flowinityErrorMessage->setText("Flowinity auto-discovery status: ");

    connect(m_serverTPU, &QLineEdit::editingFinished, this, &GeneralConf::serverTPUEdited);

    QPushButton* saveButton = new QPushButton(tr("Test and Save"), this);
    connect(saveButton, &QPushButton::clicked, this, &GeneralConf::saveServerTPU);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_serverTPU);
    inputLayout->addWidget(saveButton);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    auto* label = new QLabel(tr("Flowinity Server URL"), this);
    mainLayout->addWidget(label);
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(m_flowinityErrorMessage);

    hboxLayout->addLayout(mainLayout);

    m_uploadToken = new QLineEdit(this);
    m_uploadToken->setStyleSheet(QStringLiteral("color: %1").arg(foreground));
    m_uploadToken->setText(ConfigHandler().uploadTokenTPU());
    connect(m_uploadToken, &QLineEdit::editingFinished, this, &GeneralConf::uploadTokenTPUEdited);

    auto* labelAPIKey = new QLabel(tr("Flowinity API Key"), this);
    hboxLayoutKey->addWidget(m_uploadToken);
    hboxLayoutKey->addWidget(labelAPIKey);
}

void GeneralConf::initWindowOffsets()
{
    auto* box = new QGroupBox(tr("Upload Notification Settings"));

    box->setFlat(true);
    m_scrollAreaLayout->addWidget(box);

    auto* vboxLayout = new QVBoxLayout();
    box->setLayout(vboxLayout);

    m_uploadWindowEnabled =
      new QCheckBox(tr("Enable Upload Notification"), this);
    m_uploadWindowEnabled->setChecked(ConfigHandler().uploadWindowEnabled());
    auto* uploadWindowEnabledWarning =
      new QLabel(tr("WAYLAND USERS: This upload notification window does not "
                    "currently function correctly under Wayland. Please "
                    "disable or force X11 (XCB) mode in Force Qt Platform."),
                 this);
    uploadWindowEnabledWarning->setWordWrap(true);

    auto* posY = new QHBoxLayout();
    auto* posYLabel = new QLabel(tr("Position Offset Y (px)"), this);
    m_uploadWindowOffsetY = new QSpinBox(this);
    m_uploadWindowOffsetY->setMinimum(-99999);
    m_uploadWindowOffsetY->setMaximum(99999);
    m_uploadWindowOffsetY->setValue(ConfigHandler().uploadWindowOffsetY());
    posY->addWidget(m_uploadWindowOffsetY);
    posY->addWidget(posYLabel);

    auto* posX = new QHBoxLayout();
    auto* posXLabel = new QLabel(tr("Position Offset X (px)"), this);
    m_uploadWindowOffsetX = new QSpinBox(this);
    m_uploadWindowOffsetX->setMinimum(-99999);
    m_uploadWindowOffsetX->setMaximum(99999);
    m_uploadWindowOffsetX->setValue(ConfigHandler().uploadWindowOffsetX());
    posX->addWidget(m_uploadWindowOffsetX);
    posX->addWidget(posXLabel);

    auto* scaleW = new QHBoxLayout();
    auto* scaleWLabel = new QLabel(tr("Window Width (px)"), this);
    m_uploadWindowScaleWidth = new QSpinBox(this);
    m_uploadWindowScaleWidth->setMinimum(0);
    m_uploadWindowScaleWidth->setMaximum(9999);
    m_uploadWindowScaleWidth->setValue(ConfigHandler().uploadWindowScaleWidth());
    scaleW->addWidget(m_uploadWindowScaleWidth);
    scaleW->addWidget(scaleWLabel);

    auto* scaleH = new QHBoxLayout();
    auto* scaleHLabel = new QLabel(tr("Window Height (px)"), this);
    m_uploadWindowScaleHeight = new QSpinBox(this);
    m_uploadWindowScaleHeight->setMinimum(0);
    m_uploadWindowScaleHeight->setMaximum(9999);
    m_uploadWindowScaleHeight->setValue(ConfigHandler().uploadWindowScaleHeight());
    scaleH->addWidget(m_uploadWindowScaleHeight);
    scaleH->addWidget(scaleHLabel);

    auto* imageToggle = new QHBoxLayout();
    m_uploadWindowImageEnabled =
      new QCheckBox(tr("Enable Preview Image"), this);
    m_uploadWindowImageEnabled->setChecked(ConfigHandler().uploadWindowImageEnabled());
    imageToggle->addWidget(m_uploadWindowImageEnabled);

    auto* imageW = new QHBoxLayout();
    auto* imageWLabel = new QLabel(tr("Preview Image Width (px)"), this);
    m_uploadWindowImageWidth = new QSpinBox(this);
    m_uploadWindowImageWidth->setMinimum(0);
    m_uploadWindowImageWidth->setMaximum(9999);
    m_uploadWindowImageWidth->setValue(ConfigHandler().uploadWindowImageWidth());
    imageW->addWidget(m_uploadWindowImageWidth);
    imageW->addWidget(imageWLabel);


    QString foreground = this->palette().windowText().color().name();
    m_uploadWindowOffsetY->setStyleSheet(
      QStringLiteral("color: %1").arg(foreground));
    m_uploadWindowOffsetX->setStyleSheet(
      QStringLiteral("color: %1").arg(foreground));

    auto* timeout = new QHBoxLayout();
    auto* timeoutLabel = new QLabel(tr("Timeout (ms)"), this);
    m_uploadWindowTimeout = new QSpinBox(this);
    m_uploadWindowTimeout->setMinimum(0);
    m_uploadWindowTimeout->setMaximum(99999999);
    m_uploadWindowTimeout->setValue(ConfigHandler().uploadWindowTimeout());
    timeout->addWidget(m_uploadWindowTimeout);
    timeout->addWidget(timeoutLabel);

    auto* stackPadding = new QHBoxLayout();
    auto* stackPaddingLabel = new QLabel(tr("Window Stack Padding (px)"), this);
    m_uploadWindowStackPadding = new QSpinBox(this);
    m_uploadWindowStackPadding->setMinimum(0);
    m_uploadWindowStackPadding->setMaximum(99999);
    m_uploadWindowStackPadding->setValue(
      ConfigHandler().uploadWindowStackPadding());
    stackPadding->addWidget(m_uploadWindowStackPadding);
    stackPadding->addWidget(stackPaddingLabel);

    auto* buttonsToggle = new QHBoxLayout();
    m_uploadWindowButtonsEnabled =
      new QCheckBox(tr("Enable Action Buttons"), this);
    m_uploadWindowButtonsEnabled->setChecked(ConfigHandler().uploadWindowButtonsEnabled());
    buttonsToggle->addWidget(m_uploadWindowButtonsEnabled);

    auto* displayLayout = new QHBoxLayout();
    auto* displayLabel = new QLabel(tr("Display"), this);
    m_selectDisplay = new QComboBox(this);
    m_selectDisplay->addItem(tr("Primary (Default)"), -1);

    QList<QScreen*> screens = QGuiApplication::screens();

    for (int i = 0; i < screens.size(); ++i) {
        QString screenName = screens[i]->name();
        m_selectDisplay->addItem(screenName, i);
    }

    m_selectDisplay->setCurrentIndex(ConfigHandler().uploadWindowDisplay() + 1);

    displayLayout->addWidget(m_selectDisplay);
    displayLayout->addWidget(displayLabel);

    connect(m_uploadWindowOffsetY,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowOffsetYEdited);

    connect(m_uploadWindowOffsetX,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowOffsetXEdited);

    connect(m_uploadWindowTimeout,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowTimeoutEdited);

    connect(m_uploadWindowStackPadding,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowStackPaddingEdited);

    connect(m_uploadWindowEnabled,
            &QCheckBox::clicked,
            this,
            &GeneralConf::uploadWindowEnabledEdited);

    connect(m_uploadWindowScaleWidth,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowScaleWidthEdited);

    connect(m_uploadWindowScaleHeight,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowScaleHeightEdited);

    connect(m_uploadWindowImageWidth,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &GeneralConf::uploadWindowImageWidthEdited);

    connect(m_selectDisplay,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &GeneralConf::uploadWindowDisplayEdited);

    connect(m_uploadWindowImageEnabled,
            &QCheckBox::clicked,
            this,
            &GeneralConf::uploadWindowImageEnabledEdited);

    connect(m_uploadWindowButtonsEnabled,
            &QCheckBox::clicked,
            this,
            &GeneralConf::uploadWindowButtonsEnabledEdited);

    // reset button
    auto* resetButton = new QPushButton(tr("Reset Window Options"), this);
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        m_uploadWindowOffsetY->setValue(100);
        m_uploadWindowOffsetX->setValue(10);
        m_uploadWindowTimeout->setValue(25000);
        m_uploadWindowStackPadding->setValue(25);
        m_uploadWindowScaleWidth->setValue(580);
        m_uploadWindowScaleHeight->setValue(120);
        m_uploadWindowImageWidth->setValue(125);
    });

    auto* testButton = new QPushButton(tr("Test Window"), this);

    connect(testButton, &QPushButton::clicked, this, [this]() {
        ImgUploaderBase* widget = ImgUploaderManager().uploader(nullptr);

        m_openWindowCount++;

        QObject::connect(
          widget, &QObject::destroyed, [this]() { m_openWindowCount--; });

        widget->showPreUploadDialog(m_openWindowCount);
        widget->showPostUploadDialog(m_openWindowCount);
    });

    vboxLayout->addWidget(m_uploadWindowEnabled);
    vboxLayout->addWidget(uploadWindowEnabledWarning);
    vboxLayout->addLayout(posY);
    vboxLayout->addLayout(posX);
    vboxLayout->addLayout(scaleW);
    vboxLayout->addLayout(scaleH);
    vboxLayout->addLayout(imageToggle);
    vboxLayout->addLayout(imageW);
    vboxLayout->addLayout(timeout);
    vboxLayout->addLayout(stackPadding);
    vboxLayout->addLayout(buttonsToggle);
    vboxLayout->addLayout(displayLayout);
    vboxLayout->addWidget(resetButton);
    vboxLayout->addWidget(testButton);
}

void GeneralConf::uploadWindowEnabledEdited(bool checked)
{
    ConfigHandler().setUploadWindowEnabled(checked);
}

void GeneralConf::uploadClientKeyEdited()
{
    ConfigHandler().setUploadClientSecret(m_uploadClientKey->text());
}

void GeneralConf::serverTPUEdited()
{
    ConfigHandler().setServerTPU(m_serverTPU->text());
}

void GeneralConf::uploadTokenTPUEdited()
{
    ConfigHandler().setUploadTokenTPU(m_uploadToken->text());
}

void GeneralConf::saveServerTPU()
{
    AbstractLogger::info() << "Saving Flowinity server URL: " << m_serverTPU->text();
    ConfigHandler().setServerTPU(m_serverTPU->text());

    connect(m_endpoints, &EndpointsJSON::endpointOk, this, [this](const QString& response) {
        m_flowinityErrorMessage->setText("Flowinity auto-discovery endpoint: " + response);
        m_flowinityErrorMessage->setStyleSheet("color: lime");
        AbstractLogger::info() << "Flowinity auto-discovery status: OK";
    });

    connect(m_endpoints, &EndpointsJSON::error, this, [this](const QString& error) {
        m_flowinityErrorMessage->setText("Flowinity auto-discovery status: " + error);
        m_flowinityErrorMessage->setStyleSheet("color: red");
        AbstractLogger::error() << "Flowinity auto-discovery status: " << error;
    });

    m_endpoints->getAPIEndpoint(true);
}

void GeneralConf::uploadWindowOffsetYEdited(int value)
{
    ConfigHandler().setUploadWindowOffsetY(value);
}

void GeneralConf::uploadWindowOffsetXEdited(int value)
{
    ConfigHandler().setUploadWindowOffsetX(value);
}

void GeneralConf::uploadWindowTimeoutEdited(int value)
{
    ConfigHandler().setUploadWindowTimeout(value);
}

void GeneralConf::uploadWindowStackPaddingEdited(int value)
{
    ConfigHandler().setUploadWindowStackPadding(value);
}

void GeneralConf::uploadWindowScaleWidthEdited(int value)
{
    ConfigHandler().setUploadWindowScaleWidth(value);
}

void GeneralConf::uploadWindowScaleHeightEdited(int value)
{
    ConfigHandler().setUploadWindowScaleHeight(value);
}

void GeneralConf::uploadWindowImageEnabledEdited()
{
    ConfigHandler().setUploadWindowImageEnabled(m_uploadWindowImageEnabled->isChecked());
}

void GeneralConf::uploadWindowButtonsEnabledEdited()
{
    ConfigHandler().setUploadWindowButtonsEnabled(m_uploadWindowButtonsEnabled->isChecked());
}

void GeneralConf::uploadWindowImageWidthEdited(int value)
{
    ConfigHandler().setUploadWindowImageWidth(value);
}

void GeneralConf::uploadWindowDisplayEdited(int index)
{
    ConfigHandler().setUploadWindowDisplay(m_selectDisplay->currentData().toInt());
}