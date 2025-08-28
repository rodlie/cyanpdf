/*
# SPDX-License-Identifier: AGPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Ole-André Rodlie <https://pdf.cyan.graphics>
*/

#ifndef CYANPDF_H
#define CYANPDF_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class CyanPDF;
}
QT_END_NAMESPACE

class CyanPDF : public QMainWindow
{
    Q_OBJECT

public:
    CyanPDF(QWidget *parent = nullptr);
    ~CyanPDF();

    enum RenderIntent {
        Perceptual,
        Colorimetric,
        Saturation,
        AbsoluteColorimetric,
        NoIntent
    };

    enum ColorSpace {
        RGB,
        CMYK,
        GRAY,
        NA
    };

    const QString getGhostscript(bool pathOnly = false);
    const QString getPostscript(const QString &profile);
    const QString getCachePath();
    const QString getChecksum(const QString &filename);
    const QString getConvertArgs(const QString &inputFile,
                                 const QString &inputIcc,
                                 const QString &outputFile,
                                 const QString &outputIcc,
                                 const int &colorSpace = ColorSpace::CMYK,
                                 const int &renderIntent = RenderIntent::Colorimetric,
                                 const bool &blackPoint = true);
    const int getColorspace(const QString &profile);
    const QStringList getProfiles(const int &colorspace);
    const QString getProfileName(const QString &profile);
    const bool isFileType(const QString &filename,
                          const QString &mime,
                          const bool &startsWith = false);
    const bool isPDF(const QString &filename);
    const bool isICC(const QString &filename);

private:
    Ui::CyanPDF *ui;
};

#endif // CYANPDF_H
