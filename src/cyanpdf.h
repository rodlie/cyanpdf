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

private:
    Ui::CyanPDF *ui;
};
#endif // CYANPDF_H
