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
#include <QStandardPaths>
#include <QProcess>
#include <QRegularExpression>

#include <lcms2.h>

CyanPDF::CyanPDF(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CyanPDF)
{
    ui->setupUi(this);
    qDebug() << "Ghostscript?" << getGhostscript() << getGhostscriptVersion();
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
#endif
    QString gs = QStandardPaths::findExecutable("gs");
    if (gs.isEmpty()) {
        gs = QStandardPaths::findExecutable("gs", {"/opt/local/bin", "/usr/local/bin"});
    }
    QFileInfo info(gs);
    return pathOnly ? info.absoluteDir().absolutePath() : info.absoluteFilePath();
}

const QString CyanPDF::getGhostscriptVersion()
{
    const QString gs = getGhostscript();
    if (!QFile::exists(gs)) { return QString(); }
    QProcess proc;
    proc.start(gs, {"--version"});
    if (proc.waitForStarted()) {
        proc.waitForFinished();
        QByteArray result = proc.readAll();
        if (proc.exitCode() == 0) { return result.trimmed(); }
    }
    return QString();
}

const QString CyanPDF::getPostscript(const QString &profile)
{
    if (!isICC(profile)) { return QString(); }

    const QString gsPath = getGhostscript(true);
    if (!QFile::exists(gsPath)) { return QString(); }

    const QString gsVer = getGhostscriptVersion();
    if (gsVer.isEmpty()) { return QString(); }

    const QString ps = QString("%1/../share/ghostscript/%2/lib/PDFX_def.ps").arg(gsPath, gsVer);

    QFile file(ps);
    QString content;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        content = file.readAll();
        file.close();
    }

    if (!content.isEmpty()) {
        const QString output = QString("%1/PDFX_def.ps").arg(getCachePath());
        QRegularExpression regex("/ICCProfile \\([^)]*\\) def");
        QString replacement = QString("/ICCProfile (%1) def").arg(profile);
        QString modified = content.replace(regex, replacement);
        QFile newFile(output);
        if (newFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            newFile.write(modified.toUtf8());
            newFile.close();
            return output;
        }
    }

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

const QStringList CyanPDF::getConvertArgs(const QString &inputFile,
                                          const QString &outputFile,
                                          const QString &outputIcc,
                                          const QString defRgbIcc,
                                          const QString defGrayIcc,
                                          const QString defCmykIcc,
                                          const int &colorSpace,
                                          const int &renderIntent,
                                          const bool &blackPoint)
{
    QStringList args;
    QString cs = colorSpace == ColorSpace::CMYK ? "CMYK" : "GRAY";
    QString ps = getPostscript(outputIcc);

    if (!QFile::exists(ps) ||
        !isICC(defRgbIcc) ||
        !isICC(defGrayIcc) ||
        !isICC(defCmykIcc) ||
        !isICC(outputIcc) ||
        !isPDF(inputFile)) { return args; }

    if (getColorspace(defRgbIcc) != ColorSpace::RGB ||
        getColorspace(defGrayIcc) != ColorSpace::GRAY ||
        getColorspace(defCmykIcc) != ColorSpace::CMYK ||
        getColorspace(outputIcc) != colorSpace) { return args; }

    args << "-dPDFX" << "-dBATCH" << "-dNOPAUSE" << "-dNOSAFER" << "-sDEVICE=pdfwrite"
         << "-dOverrideICC=true" << "-dEncodeColorImages=true" << "-dEmbedAllFonts=true"
         << QString("-sProcessColorModel=Device%1").arg(cs)
         << QString("-sColorConversionStrategy=%1").arg(cs)
         << QString("-sColorConversionStrategyForImages=%1").arg(cs)
         << QString("-dRenderIntent=%1").arg(QString::number(renderIntent))
         << QString("-dPreserveBlack=%1").arg(blackPoint ? "true" : "false")
         << QString("-sDefaultRGBProfile=\"%1\"").arg(defRgbIcc)
         << QString("-sDefaultGrayProfile=\"%1\"").arg(defGrayIcc)
         << QString("-sDefaultCMYKProfile=\"%1\"").arg(defCmykIcc)
         << QString("-sOutputICCProfile=\"%1\"").arg(outputIcc)
         << QString("-sOutputFile=\"%1\"").arg(outputFile)
         << QString("\"%1\"").arg(ps)
         << QString("\"%1\"").arg(inputFile);
    return args;
}

const int CyanPDF::getColorspace(const QString &profile)
{
    int result = ColorSpace::NA;
    if (!isICC(profile)) { return result; }
    auto hprofile = cmsOpenProfileFromFile(profile.toStdString().c_str(), "r");
    if (hprofile) {
        auto space = cmsGetColorSpace(hprofile);
        switch (space) {
        case cmsSigRgbData:
            result = ColorSpace::RGB;
            break;
        case cmsSigCmykData:
            result = ColorSpace::CMYK;
            break;
        case cmsSigGrayData:
            result = ColorSpace::GRAY;
            break;
        default:;
        }
    }
    cmsCloseProfile(hprofile);
    return result;
}

const QStringList CyanPDF::getProfiles(const int &colorspace)
{
    // TODO
    return QStringList();
}

const QString CyanPDF::getProfileName(const QString &profile)
{
    QString result;
    if (!isICC(profile)) { return result; }
    auto hprofile = cmsOpenProfileFromFile(profile.toStdString().c_str(), "r");
    if (hprofile) {
        cmsUInt32Number size = 0;
        size = cmsGetProfileInfoASCII(hprofile,
                                      cmsInfoDescription,
                                      "en",
                                      "US",
                                      nullptr,
                                      0);
        if (size > 0) {
            std::vector<char> buffer(size);
            cmsUInt32Number newsize = cmsGetProfileInfoASCII(hprofile,
                                                             cmsInfoDescription,
                                                             "en",
                                                             "US",
                                                             &buffer[0],
                                                             size);
            if (size == newsize) { result = buffer.data(); }
        }
    }
    cmsCloseProfile(hprofile);
    return result.isEmpty() ? profile : result;
}

const bool CyanPDF::isFileType(const QString &filename,
                               const QString &mime,
                               const bool &startsWith)
{
    if (!QFile::exists(filename)) { return false; }
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(filename);
    return (startsWith? type.name().startsWith(mime) : type.name() == mime);
}

const bool CyanPDF::isPDF(const QString &filename)
{
    return isFileType(filename, "application/pdf");
}

const bool CyanPDF::isICC(const QString &filename)
{
    return isFileType(filename, "application/vnd.iccprofile");
}
