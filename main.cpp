/*
# SPDX-License-Identifier: AGPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Ole-André Rodlie <https://pdf.cyan.graphics>
*/

#include "cyanpdf.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("CyanPDF");
    QApplication::setOrganizationName("CyanPDF");
    QApplication::setApplicationVersion(QString(CYANPDF_VERSION));
    QApplication::setOrganizationDomain(QString(CYANPDF_ID));
    QGuiApplication::setDesktopFileName(QString(CYANPDF_ID));

    CyanPDF w;
    w.show();
    return a.exec();
}
