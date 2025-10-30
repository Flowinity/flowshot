//
// Created by troplo on 10/27/25.
//

#include "configEntry.h"
#include <QApplication>
#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include <QTabWidget>
#include <QVBoxLayout>
#include "filenameEditor.h"
#include "generalconf2.h"

ConfigEntry::ConfigEntry(QWidget *parent) : QWidget(parent) {
    tabWidget = new QTabWidget(this);
    tabWidget->addTab(new GeneralConf(), "General Settings");
    tabWidget->addTab(new FileNameEditor(), "Filename Editor");

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(tabWidget);
    setLayout(layout);
}