//
// Created by troplo on 10/30/25.
//

#ifndef FLOWINITYVALIDUPLOADRESPONSE_H
#define FLOWINITYVALIDUPLOADRESPONSE_H
#include <QString>


class FlowinityValidUploadResponse {
private:
    QString m_Url;
    QString m_FilePath;
    bool m_showPreview;
public:
    inline FlowinityValidUploadResponse(QString url, QString filePath, bool showPreview = false)
    {
        m_Url = std::move(url);
        m_FilePath = std::move(filePath);
        m_showPreview = showPreview;
    }

    inline QString getUrl()
    {
        return m_Url;
    };

    inline QString getFilePath()
    {
        return m_FilePath;
    };

    inline bool getShowPreview()
    {
        return m_showPreview;
    };

};



#endif //FLOWINITYVALIDUPLOADRESPONSE_H
