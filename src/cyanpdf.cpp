/*
# SPDX-License-Identifier: AGPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Ole-André Rodlie <https://pdf.cyan.graphics>
*/

#include "cyanpdf.h"
#include "ui_cyanpdf.h"

CyanPDF::CyanPDF(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CyanPDF)
{
    ui->setupUi(this);
}

CyanPDF::~CyanPDF()
{
    delete ui;
}
