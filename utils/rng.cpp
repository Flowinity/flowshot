//
// Created by troplo on 10/24/25.
//

#include "rng.h"

namespace Flowshot
{
    QString randomFilePath() {
        const QString tmpDir = "/tmp/";
        const QString randomName = QString("screenshot_%1.png")
                                   .arg(QRandomGenerator::global()->generate(), 8, 16, QChar('0'));
        return tmpDir + randomName;
    }

    QString randomString(int length)
    {
        const QString possibleChars = "0123456789ABCDEF";
        QString result;
        for (int i = 0; i < length; ++i) {
            int index = QRandomGenerator::global()->bounded(possibleChars.size());
            result.append(possibleChars.at(index));
        }
        return result;
    }
}
