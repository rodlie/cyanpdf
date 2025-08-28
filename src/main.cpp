/*
# SPDX-License-Identifier: AGPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Ole-André Rodlie <https://pdf.cyan.graphics>
*/

#include "cyanpdf.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CyanPDF w;
    w.show();
    return a.exec();
}
