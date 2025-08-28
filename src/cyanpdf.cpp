/*
# SPDX-License-Identifier: AGPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Ole-André Rodlie <https://pdf.cyan.graphics>
*/

#include "cyanpdf.h"
#include "ui_cyanpdf.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QCryptographicHash>

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

const QString CyanPDF::getGhostscript(bool pathOnly)
{
#ifdef Q_OS_WIN
    QString appDir = QString("%1/gs").arg(qApp->applicationDirPath());
    if (QFile::exists(appDir)) {
        QString bin64 = appDir + "/bin/gswin64c.exe";
        if (QFile::exists(bin64)) { return pathOnly ? appDir : bin64; }
        QString bin32 = appDir + "/bin/gswin32c.exe";
        if (QFile::exists(bin32)) { return pathOnly ? appDir : bin32; }
    }
    QString programFilesPath(qgetenv("PROGRAMFILES"));
    QDirIterator it(programFilesPath + "/gs", QStringList() << "*.*", QDir::Dirs/*, QDirIterator::Subdirectories*/);
    while (it.hasNext()) {
        QString folder = it.next();
        QString bin64 = folder + "/bin/gswin64c.exe";
        if (QFile::exists(bin64)) { return pathOnly ? folder : bin64; }
        QString bin32 = folder + "/bin/gswin32c.exe";
        if (QFile::exists(bin32)) { return pathOnly ? folder : bin32; }
    }
    return QString();
#endif
// TODO
#ifdef Q_OS_MAC
    if (pathOnly) { return QString(); }
    QStringList gs;
    gs << "/opt/local/bin/gs" << "/usr/local/bin/gs";
    for (int i = 0; i < gs.size(); ++i) { if (QFile::exists(gs.at(i))) { return gs.at(i); } }
    return QString();
#endif
    return pathOnly ? QString() : QString("gs");
}

const QString CyanPDF::getPostscript(const QString &filename)
{
    if (!isICC(filename)) { return QString(); }
    // TODO
    return QString();
}

const QString CyanPDF::getCachePath()
{
    QString path = QDir::tempPath(); // TODO
    path.append("/cyanpdf");
    if (!QFile::exists(path)) {
        QDir dir(path);
        if (!dir.mkpath(path)) { return QString(); }
    }
    return path;
}

const QString CyanPDF::getChecksum(const QString &filename)
{
    if (!isPDF(filename)) { return QString(); }
    QFile file(filename);
    QString result;
    if (file.open(QFile::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (hash.addData(&file)) { result = hash.result().toHex(); }
        file.close();
    }
    return result;
}

bool CyanPDF::isFileType(const QString &filename,
                         const QString &mime,
                         bool startsWith)
{
    if (!QFile::exists(filename)) { return false; }
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(filename);
    qDebug() << "mime type" << filename << type.name();
    return (startsWith? type.name().startsWith(mime) : type.name() == mime);
}

bool CyanPDF::isPDF(const QString &filename)
{
    return isFileType(filename, "application/pdf");
}

bool CyanPDF::isICC(const QString &filename)
{
    return isFileType(filename, "application/vnd.iccprofile");
}
