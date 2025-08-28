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

    const QString getGhostscript(bool pathOnly = false);
    const QString getCachePath();
    const QString getChecksum(const QString &filename);
    bool isFileType(const QString &filename,
                           const QString &mime,
                           bool startsWith = false);
    bool isPDF(const QString &filename);

private:
    Ui::CyanPDF *ui;
};

#endif // CYANPDF_H
