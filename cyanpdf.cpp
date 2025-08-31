/*
# SPDX-License-Identifier: AGPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Ole-André Rodlie <https://pdf.cyan.graphics>
*/

#include "cyanpdf.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QProcess>
#include <QRegularExpression>
#include <QDirIterator>
#include <QTimer>
#include <QImage>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QDesktopServices>

#include <lcms2.h>

CyanPDF::CyanPDF(QWidget *parent)
    : QMainWindow(parent)
    , mDocument(nullptr)
    , mRenderer(nullptr)
    , mLabel(nullptr)
    , mComboDefRgb(nullptr)
    , mComboDefCmyk(nullptr)
    , mComboDefGray(nullptr)
    , mComboOutIcc(nullptr)
    , mComboRenderIntent(nullptr)
    , mCheckBlackPoint(nullptr)
    , mSpecsList(nullptr)
{
    setupWidgets();
}

CyanPDF::~CyanPDF()
{
    mDocument->close();
    writeSettings();
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
    QDirIterator it(programFilesPath + "/gs", {"*.*"}, QDir::Dirs);
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
        gs = QStandardPaths::findExecutable("gs", {"/opt/local/bin",
                                                   "/usr/local/bin"});
    }
    if (gs.isEmpty()) { return QString(); }

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

const QString CyanPDF::getPostscript(const QString &filename,
                                     const QString &profile)
{
    if (!isPDF(filename) || !isICC(profile)) { return QString(); }

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
        static QRegularExpression regex("/ICCProfile \\([^)]*\\) def");
        const QString output = QString("%1/%2.ps").arg(getCachePath(), getChecksum(filename));
        const QString replacement = QString("/ICCProfile (%1) def").arg(profile);
        const QString modified = content.replace(regex, replacement);
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
    QStringList paths = QStandardPaths::standardLocations(QStandardPaths::GenericCacheLocation);
    QString path = paths.first();
    if (path.isEmpty()) { path = QDir::tempPath(); }
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
    const QString cs = colorSpace == ColorSpace::CMYK ? "CMYK" : "GRAY";
    const QString ps = getPostscript(inputFile, outputIcc);

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
         << QString("-sDefaultRGBProfile=%1").arg(defRgbIcc)
         << QString("-sDefaultGrayProfile=%1").arg(defGrayIcc)
         << QString("-sDefaultCMYKProfile=%1").arg(defCmykIcc)
         << QString("-sOutputICCProfile=%1").arg(outputIcc)
         << QString("-sOutputFile=%1").arg(outputFile)
         << QString("%1").arg(ps)
         << QString("%1").arg(inputFile);
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
    QStringList folders;
#ifdef Q_OS_WIN
    folders << QDir::rootPath() + "/WINDOWS/System32/spool/drivers/color";
#elif defined(Q_OS_MAC)
    folders << "/Library/ColorSync/Profiles";
    folders << QDir::homePath() + "/Library/ColorSync/Profiles";
#else
    const QStringList common = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &path : common) { folders << QString("%1/color/icc").arg(path); }
#endif
    folders << QDir::homePath() + "/.color/icc";

    QStringList profiles;
    for (const QString &path : folders) {
        QDir directory(path);
        if (directory.exists()) {
            QDir::Filters filters = QDir::Files | QDir::Readable;
            QDirIterator it(directory.absolutePath(),
                            {"*.icc"},
                            filters,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString profile = it.next();
                if (getColorspace(profile) == colorspace) { profiles << profile; }
            }
        }
    }
    return profiles;
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

void CyanPDF::setupWidgets()
{
    setWindowTitle("Cyan PDF");
    setWindowIcon(QIcon(":/docs/graphics.cyan.pdf.svg"));
    setFixedSize({800, 600});

    mDocument = new QPdfDocument(this);
    mRenderer = new QPdfPageRenderer(this);

    mLabel = new QLabel(this);
    mLabel->setScaledContents(false);
    mLabel->setFixedWidth(400);
    mLabel->setBackgroundRole(QPalette::Dark);
    mLabel->setAutoFillBackground(true);

    mComboDefRgb = new ComboBox(this);
    mComboDefCmyk = new ComboBox(this);
    mComboDefGray = new ComboBox(this);
    mComboOutIcc = new ComboBox(this);
    mComboRenderIntent = new ComboBox(this);

    mCheckBlackPoint = new QCheckBox(this);
    mCheckBlackPoint->setText(tr("Black Point"));

    mSpecsList = new QTreeWidget(this);
    mSpecsList->setHeaderLabels({tr("Key"), tr("Value")});
    mSpecsList->setHeaderHidden(true);
    mSpecsList->setDropIndicatorShown(false);
    mSpecsList->setIndentation(0);

    mComboDefRgb->setObjectName("rgb");
    mComboDefCmyk->setObjectName("cmyk");
    mComboDefGray->setObjectName("gray");
    mComboOutIcc->setObjectName("output");
    mComboRenderIntent->setObjectName("intent");

    mComboDefRgb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mComboDefCmyk->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mComboDefGray->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mComboOutIcc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mComboRenderIntent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mCheckBlackPoint->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    const auto rgbWid = new QWidget(this);
    const auto rgbLay = new QHBoxLayout(rgbWid);
    const auto cmykWid = new QWidget(this);
    const auto cmykLay = new QHBoxLayout(cmykWid);
    const auto grayWid = new QWidget(this);
    const auto grayLay = new QHBoxLayout(grayWid);
    const auto intentWid = new QWidget(this);
    const auto intentLay = new QHBoxLayout(intentWid);
    const auto outputWid = new QWidget(this);
    const auto outputLay = new QHBoxLayout(outputWid);
    const auto extraWid = new QWidget(this);
    const auto extraLay = new QHBoxLayout(extraWid);
    const auto buttonWid = new QWidget(this);
    const auto buttonLay = new QHBoxLayout(buttonWid);
    const auto sideWid = new QWidget(this);
    const auto sideLay = new QVBoxLayout(sideWid);

    const auto margins = QMargins(0, 0, 0, 0);
    rgbWid->setContentsMargins(margins);
    rgbLay->setContentsMargins(margins);
    cmykWid->setContentsMargins(margins);
    cmykLay->setContentsMargins(margins);
    grayWid->setContentsMargins(margins);
    grayLay->setContentsMargins(margins);
    intentWid->setContentsMargins(margins);
    intentLay->setContentsMargins(margins);
    outputWid->setContentsMargins(margins);
    outputLay->setContentsMargins(margins);
    extraWid->setContentsMargins(margins);
    extraLay->setContentsMargins(margins);
    buttonWid->setContentsMargins(margins);
    buttonLay->setContentsMargins(margins);

    auto sideMargins = sideWid->contentsMargins();
    sideMargins.setBottom(0);
    sideWid->setContentsMargins(sideMargins);
    sideLay->setContentsMargins(sideMargins);

    rgbLay->addWidget(new QLabel(tr("RGB Profile"), this), Qt::AlignLeft);
    rgbLay->addWidget(mComboDefRgb, Qt::AlignRight);

    cmykLay->addWidget(new QLabel(tr("CMYK Profile"), this), Qt::AlignLeft);
    cmykLay->addWidget(mComboDefCmyk, Qt::AlignRight);

    grayLay->addWidget(new QLabel(tr("GRAY Profile"), this), Qt::AlignLeft);
    grayLay->addWidget(mComboDefGray, Qt::AlignRight);

    intentLay->addWidget(new QLabel(tr("Rendering Intent"), this), Qt::AlignLeft);
    intentLay->addWidget(mComboRenderIntent, Qt::AlignRight);

    outputLay->addWidget(new QLabel(tr("Output Profile"), this), Qt::AlignLeft);
    outputLay->addWidget(mComboOutIcc, Qt::AlignRight);

    extraLay->addWidget(new QWidget(this), Qt::AlignLeft);
    extraLay->addWidget(mCheckBlackPoint, Qt::AlignRight);

    const auto appLabel = new QLabel(this);
    appLabel->setFixedHeight(128);
    appLabel->setScaledContents(false);
    appLabel->setAlignment(Qt::AlignCenter);
    appLabel->setPixmap(QPixmap(":/docs/graphics.cyan.pdf.svg"));

    const auto buttonOpen = new QPushButton(this);
    buttonOpen->setText(tr("Open"));
    buttonOpen->setShortcut(QKeySequence("Ctrl+O"));
    QIcon iconOpen = QIcon::fromTheme("document-open");
    if (iconOpen.isNull()) { iconOpen = QIcon::fromTheme("document-open-symbolic"); }
    buttonOpen->setIcon(iconOpen);
    connect(buttonOpen, &QPushButton::released,
            this, [this]{
        const QString filename = QFileDialog::getOpenFileName(this,
                                                              tr("Open PDF"),
                                                              getLastOpenPath(),
                                                              "*.pdf");
        loadPDF(filename);
    });

    const auto buttonSave = new QPushButton(this);
    buttonSave->setText(tr("Save"));
    buttonSave->setShortcut(QKeySequence("Ctrl+S"));
    QIcon iconSave = QIcon::fromTheme("document-save");
    if (iconSave.isNull()) { iconSave = QIcon::fromTheme("document-save-symbolic"); }
    buttonSave->setIcon(iconSave);
    connect(buttonSave, &QPushButton::released,
            this, [this]{
        const QString filename = QFileDialog::getSaveFileName(this,
                                                              tr("Save PDF"),
                                                              getLastSavePath(),
                                                              "*.pdf");
        savePDF(filename);
    });

    const auto buttonClose = new QPushButton(this);
    buttonClose->setText(tr("Quit"));
    buttonClose->setShortcut(QKeySequence("Ctrl+Q"));
    QIcon iconClose = QIcon::fromTheme("application-exit");
    if (iconClose.isNull()) { iconClose = QIcon::fromTheme("application-exit-symbolic"); }
    buttonClose->setIcon(iconClose);
    connect(buttonClose, &QPushButton::released,
            this, &QMainWindow::close);

    buttonLay->addWidget(buttonOpen);
    buttonLay->addWidget(buttonSave);
    buttonLay->addStretch();
    buttonLay->addWidget(buttonClose);

    sideLay->addWidget(appLabel);
    sideLay->addSpacing(10);
    sideLay->addWidget(rgbWid);
    sideLay->addWidget(cmykWid);
    sideLay->addWidget(grayWid);
    sideLay->addSpacing(10);
    sideLay->addWidget(outputWid);
    sideLay->addWidget(intentWid);
    sideLay->addWidget(extraWid);
    sideLay->addWidget(mSpecsList);
    sideLay->addWidget(buttonWid);

    const auto wid = new QWidget(this);
    const auto lay = new QHBoxLayout(wid);

    setCentralWidget(wid);
    lay->addWidget(mLabel);
    lay->addWidget(sideWid);

    populateComboBoxes();

    QTimer::singleShot(10, this, &CyanPDF::readSettings);
}

void CyanPDF::populateComboBoxes()
{
    const auto rgbProfiles = getProfiles(ColorSpace::RGB);
    const auto cmykProfiles = getProfiles(ColorSpace::CMYK);
    const auto grayProfiles = getProfiles(ColorSpace::GRAY);

    mComboDefRgb->clear();
    mComboDefCmyk->clear();
    mComboDefGray->clear();

    QIcon iconPrint = QIcon::fromTheme("document-print");
    if (iconPrint.isNull()) { iconPrint = QIcon::fromTheme("document-print-symbolic"); }
    QIcon iconDef = QIcon::fromTheme("applications-graphics");
    if (iconDef.isNull()) { iconDef = QIcon::fromTheme("applications-graphics-symbolic"); }

    for (const QString &icc: rgbProfiles) {
        mComboDefRgb->addItem(iconDef, getProfileName(icc), icc);
    }
    for (const QString &icc: cmykProfiles) {
        mComboDefCmyk->addItem(iconDef, getProfileName(icc), icc);
        mComboOutIcc->addItem(iconPrint, getProfileName(icc), icc);
    }
    for (const QString &icc: grayProfiles) {
        mComboDefGray->addItem(iconDef, getProfileName(icc), icc);
        mComboOutIcc->addItem(iconPrint, getProfileName(icc), icc);
    }

    mComboRenderIntent->addItem(iconDef, tr("Perceptual"), 0);
    mComboRenderIntent->addItem(iconDef, tr("Relative Colorimetric"), 1);
    mComboRenderIntent->addItem(iconDef, tr("Saturation"), 2);
    mComboRenderIntent->addItem(iconDef, tr("Absolute Colorimetric"), 3);
}

void CyanPDF::readSettings()
{
    QSettings settings;
    settings.beginGroup("cyanpdf");

    if (settings.value("geometry").isValid()) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }

    if (settings.value("rgb").isValid()) {
        {
            int index = mComboDefRgb->findData(settings.value("rgb").toString());
            if (index != -1) { mComboDefRgb->setCurrentIndex(index); }
        }
    } else {
        {
            int index = mComboDefRgb->findText("Adobe RGB (1998)");
            if (index != -1) { mComboDefRgb->setCurrentIndex(index); }
            else {
                index = mComboDefRgb->findText("sRGB");
                if (index != -1) { mComboDefRgb->setCurrentIndex(index); }
                else {
                    index = mComboDefRgb->findText("Artifex PS RGB Profile");
                    if (index != -1) { mComboDefRgb->setCurrentIndex(index); }
                }
            }
        }
    }
    if (settings.value("cmyk").isValid()) {
        {
            int index = mComboDefCmyk->findData(settings.value("cmyk").toString());
            if (index != -1) { mComboDefCmyk->setCurrentIndex(index); }
        }
    } else {
        {
            int index = mComboDefCmyk->findText("ISO Coated v2 (ECI)");
            if (index != -1) { mComboDefCmyk->setCurrentIndex(index); }
            else {
                index = mComboDefCmyk->findText("U.S. Web Coated (SWOP) v2");
                if (index != -1) { mComboDefCmyk->setCurrentIndex(index); }
                else {
                    index = mComboDefCmyk->findText("Artifex PS CMYK Profile");
                    if (index != -1) { mComboDefCmyk->setCurrentIndex(index); }
                }
            }
        }
    }
    if (settings.value("gray").isValid()) {
        {
            int index = mComboDefGray->findData(settings.value("gray").toString());
            if (index != -1) { mComboDefGray->setCurrentIndex(index); }
        }
    } else {
        {
            int index = mComboDefGray->findText("Gray");
            if (index != -1) { mComboDefGray->setCurrentIndex(index); }
            else {
                index = mComboDefGray->findText("Artifex PS Gray Profile");
                if (index != -1) { mComboDefGray->setCurrentIndex(index); }
            }
        }
    }
    if (settings.value("output").isValid()) {
        {
            int index = mComboOutIcc->findData(settings.value("output").toString());
            if (index != -1) { mComboOutIcc->setCurrentIndex(index); }
        }
    } else {
        {
            int index = mComboOutIcc->findText(mComboDefCmyk->currentText());
            if (index != -1) { mComboOutIcc->setCurrentIndex(index); }
        }
    }

    mComboRenderIntent->setCurrentIndex(settings.value("intent", 1).toInt());
    mCheckBlackPoint->setChecked(settings.value("blackpont", true).toBool());

    settings.endGroup();

    connect(mCheckBlackPoint, &QCheckBox::stateChanged,
            this, [](int state) {
        QSettings settings;
        settings.beginGroup("cyanpdf");
        settings.setValue("blackpoint", state == Qt::Checked ? true : false);
        settings.endGroup();
    });

    connectCombobox(mComboDefRgb);
    connectCombobox(mComboDefCmyk);
    connectCombobox(mComboDefGray);
    connectCombobox(mComboOutIcc);
    connectCombobox(mComboRenderIntent);
}

void CyanPDF::writeSettings()
{
    QSettings settings;
    settings.beginGroup("cyanpdf");
    settings.setValue("geometry", geometry());
    settings.endGroup();
}

void CyanPDF::setLastOpenPath(const QString &path)
{
    QSettings settings;
    settings.beginGroup("cyanpdf");
    settings.setValue("LastOpenPath", path);
    settings.endGroup();
}

const QString CyanPDF::getLastOpenPath()
{
    QString path;
    QSettings settings;
    settings.beginGroup("cyanpdf");
    path = settings.value("LastOpenPath", QDir::homePath()).toString();
    settings.endGroup();
    return path;
}

void CyanPDF::setLastSavePath(const QString &path)
{
    QSettings settings;
    settings.beginGroup("cyanpdf");
    settings.setValue("LastSavePath", path);
    settings.endGroup();
}

const QString CyanPDF::getLastSavePath()
{
    QString path;
    QSettings settings;
    settings.beginGroup("cyanpdf");
    path = settings.value("LastSavePath", QDir::homePath()).toString();
    settings.endGroup();
    return path;
}

void CyanPDF::connectCombobox(QComboBox *box)
{
    if (!box) { return; }
    connect(box, &QComboBox::currentIndexChanged,
            this, [this, box](int index) {
        const auto val = box->itemData(index).toString();
        QSettings settings;
        settings.beginGroup("cyanpdf");
        settings.setValue(box->objectName(), val);
        settings.endGroup();
    });
}

void CyanPDF::loadPDF(const QString &filename)
{
    if (!isPDF(filename)) { return; }

    mSpecsList->clear();
    mDocument->close();
    mFilename.clear();

    setLastOpenPath(QFileInfo(filename).absolutePath());

    if (mDocument->load(filename) == QPdfDocument::Error::None) {
        mFilename = filename;

        QString title = mDocument->metaData(QPdfDocument::MetaDataField::Title).toString();
        QString subject = mDocument->metaData(QPdfDocument::MetaDataField::Subject).toString();
        QString author = mDocument->metaData(QPdfDocument::MetaDataField::Author).toString();
        QString producer = mDocument->metaData(QPdfDocument::MetaDataField::Producer).toString();
        QString creator = mDocument->metaData(QPdfDocument::MetaDataField::Creator).toString();
        int pages = mDocument->pageCount();

        if (title.isEmpty()) { title = QFileInfo(filename).fileName(); }
        {
            const auto item = new QTreeWidgetItem(mSpecsList);
            item->setText(0, tr("Title"));
            item->setText(1, title);
            mSpecsList->addTopLevelItem(item);
        }
        if (!subject.isEmpty()) {
            {
                const auto item = new QTreeWidgetItem(mSpecsList);
                item->setText(0, tr("Subject"));
                item->setText(1, subject);
                mSpecsList->addTopLevelItem(item);
            }
        }
        if (!author.isEmpty()) {
            {
                const auto item = new QTreeWidgetItem(mSpecsList);
                item->setText(0, tr("Author"));
                item->setText(1, author);
                mSpecsList->addTopLevelItem(item);
            }
        }
        if (!producer.isEmpty()) {
            {
                const auto item = new QTreeWidgetItem(mSpecsList);
                item->setText(0, tr("Producer"));
                item->setText(1, producer);
                mSpecsList->addTopLevelItem(item);
            }
        }
        if (!creator.isEmpty()) {
            {
                const auto item = new QTreeWidgetItem(mSpecsList);
                item->setText(0, tr("Creator"));
                item->setText(1, creator);
                mSpecsList->addTopLevelItem(item);
            }
        }
        {
            const auto item = new QTreeWidgetItem(mSpecsList);
            item->setText(0, tr("Pages"));
            item->setText(1, QString::number(pages));
            mSpecsList->addTopLevelItem(item);
        }

        mRenderer->setDocument(mDocument);

        connect(mRenderer, &QPdfPageRenderer::pageRendered,
                this, [this](int pageNumber,
                             QSize imageSize,
                             const QImage &image,
                             QPdfDocumentRenderOptions options,
                             quint64 requestId) {
            if (!image.isNull()) {
                mLabel->setPixmap(QPixmap::fromImage(image.scaled(mLabel->size(),
                                                                  Qt::KeepAspectRatio,
                                                                  Qt::SmoothTransformation)));
            }
        });
        mRenderer->requestPage(0, mDocument->pagePointSize(0).toSize());
    }
}

void CyanPDF::savePDF(const QString &filename)
{
    if (filename.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Missing filename"),
                             tr("Missing output filename."));
        return;
    }
    setLastSavePath(QFileInfo(filename).absolutePath());

    if (!isPDF(mFilename)) {
        QMessageBox::warning(this, tr("Missing PDF"),
                             tr("No PDF document loaded."));
        return;
    }
    const QString defRgb = mComboDefRgb->currentData().toString();
    if (!isICC(defRgb)) {
        QMessageBox::warning(this, tr("Missing RGB Profile"),
                             tr("Missing default RGB profile."));
        return;
    }
    const QString defCmyk = mComboDefCmyk->currentData().toString();
    if (!isICC(defCmyk)) {
        QMessageBox::warning(this, tr("Missing CMYK Profile"),
                             tr("Missing default CMYK profile."));
        return;
    }
    const QString defGray = mComboDefGray->currentData().toString();
    if (!isICC(defGray)) {
        QMessageBox::warning(this, tr("Missing GRAY Profile"),
                             tr("Missing default GRAY profile."));
        return;
    }
    const QString outIcc = mComboOutIcc->currentData().toString();
    if (!isICC(outIcc)) {
        QMessageBox::warning(this, tr("Missing Output Profile"),
                             tr("Missing output (CMYK/GRAY) profile."));
        return;
    }

    const int intent = mComboRenderIntent->currentData().toInt();
    const bool blackPoint = mCheckBlackPoint->isChecked();

    const QString gsPath = getGhostscript();
    const QString gsVer = getGhostscriptVersion();
    if (gsPath.trimmed().isEmpty() || gsVer.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Missing Ghostscript"),
                             tr("Ghostscript not found, please install."));
        return;
    }

    const QString ps = getPostscript(mFilename, outIcc);
    if (!QFile::exists(ps)) {
        QMessageBox::warning(this, tr("Missing Postscript"),
                             tr("Unable to create postscript file."));
        return;
    }

    const QStringList args = getConvertArgs(mFilename,
                                            filename,
                                            outIcc,
                                            defRgb,
                                            defGray,
                                            defCmyk,
                                            getColorspace(outIcc),
                                            intent,
                                            blackPoint);
    if (args.count() < 1) {
        QMessageBox::warning(this, tr("Missing Arguments"),
                             tr("Unable to generate Ghostscript arguments."));
        return;
    }

    QProcess proc;
    proc.start(gsPath, args);
    if (proc.waitForStarted()) {
        proc.waitForFinished();
        QByteArray result = proc.readAll();
        if (proc.exitCode() == 0) { if (isPDF(filename)) { QDesktopServices::openUrl(QUrl(filename)); } }
        else {
            QMessageBox::warning(this, tr("Failed to Convert"),
                                 tr("Failed converting PDF.<br><br><pre>%1</pre>").arg(result));
        }
    }
}
