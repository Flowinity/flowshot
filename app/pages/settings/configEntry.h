//
// Created by troplo on 10/27/25.
//

#ifndef CONFIGENTRY_H
#define CONFIGENTRY_H


#include <QWidget>

class QTabWidget;

class ConfigEntry : public QWidget {
    Q_OBJECT

public:
    explicit ConfigEntry(QWidget *parent = nullptr);

private:
    QTabWidget *tabWidget;
};
#endif //CONFIGENTRY_H
