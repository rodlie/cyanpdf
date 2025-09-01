/*
# SPDX-License-Identifier: AGPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Ole-André Rodlie <https://pdf.cyan.graphics>
*/

#ifndef CYANPDF_H
#define CYANPDF_H

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QTreeWidget>
#include <QPdfDocument>
#include <QPdfPageRenderer>

class ComboBox : public QComboBox
{
public:
    explicit ComboBox(QWidget *parent = nullptr)
        : QComboBox(parent) {}
    QSize sizeHint() const override { return minimumSizeHint(); }
    QSize minimumSizeHint() const override { return QSize(50, QComboBox::minimumSizeHint().height()); }
};

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
    const QString getGhostscriptVersion();

    const QString getPostscript(const QString &filename,
                                const QString &profile);

    const QString getCachePath();
    const QString getChecksum(const QString &filename);

    const QStringList getConvertArgs(const QString &inputFile,
                                     const QString &outputFile,
                                     const QString &outputIcc,
                                     const QString defRgbIcc,
                                     const QString defGrayIcc,
                                     const QString defCmykIcc,
                                     const int &colorSpace = ColorSpace::CMYK,
                                     const int &renderIntent = RenderIntent::Colorimetric,
                                     const bool &blackPoint = true,
                                     const bool &overrideIcc = true);

    const int getColorspace(const QString &profile);
    const QStringList getProfiles(const int &colorspace);
    const QString getProfileName(const QString &profile);

    const bool isFileType(const QString &filename,
                          const QString &mime,
                          const bool &startsWith = false);
    const bool isPDF(const QString &filename);
    const bool isICC(const QString &filename);

    void setupWidgets();

    void populateComboBoxes();

    void readSettings();
    void writeSettings();

    void setLastOpenPath(const QString &path);
    const QString getLastOpenPath();

    void setLastSavePath(const QString &path);
    const QString getLastSavePath();

    void connectCombobox(QComboBox *box);

    void loadPDF(const QString &filename);
    void savePDF(const QString &filename);

private:
    QPdfDocument *mDocument;
    QPdfPageRenderer *mRenderer;
    QLabel *mLabel;
    ComboBox *mComboDefRgb;
    ComboBox *mComboDefCmyk;
    ComboBox *mComboDefGray;
    ComboBox *mComboOutIcc;
    ComboBox *mComboRenderIntent;
    QCheckBox *mCheckBlackPoint;
    QCheckBox *mCheckOverrideIcc;
    QTreeWidget *mSpecsList;
    QString mFilename;
};

#endif // CYANPDF_H
