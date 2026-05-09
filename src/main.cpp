/*
 *  Copyright (c) 2020 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */
#include <QApplication>
#include <QCoreApplication>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QDebug>
#include <QtGlobal>
#include <QSurfaceFormat>
#include <QSettings>
#include <QTranslator>
#include <QIcon>
#include <QGuiApplication>
#include <geogram/basic/common.h>
#include "mainwindow.h"
#include "remeshcli.h"
#include "theme.h"
#include "version.h"
#include "preferences.h"

namespace {

bool argsContainCli(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--cli") == 0)
            return true;
    }
    return false;
}

} // namespace

int main(int argc, char **argv)
{
    if (argsContainCli(argc, argv)) {
        QCoreApplication app(argc, argv);
        QCoreApplication::setApplicationName(APP_NAME);
        QCoreApplication::setApplicationVersion(APP_HUMAN_VER);
        QCoreApplication::setOrganizationName(QStringLiteral("AutoRemesher"));
        GEO::initialize();
        return runRemeshCli(argc, argv);
    }

    QApplication app(argc, argv);

    // Wayland uses the desktop entry id to resolve the panel/task switcher icon (not QWindowIcon).
    QGuiApplication::setDesktopFileName(QStringLiteral("org.autoremesher.AutoRemesher"));

    QIcon icon(":/autoremesher.png");
    if (icon.isNull()) {
        QIcon iconFile(QCoreApplication::applicationDirPath() + "/autoremesher.png");
        if (!iconFile.isNull()) {
            icon = iconFile;
        }
    }
    if (!icon.isNull()) {
        QApplication::setWindowIcon(icon);
    }

    GEO::initialize();

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);
    
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, Theme::black);
    darkPalette.setColor(QPalette::WindowText, Theme::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::Text, Theme::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Theme::black);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Theme::white);
    darkPalette.setColor(QPalette::BrightText, Theme::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, Theme::red);
    darkPalette.setColor(QPalette::HighlightedText, Theme::black);    
    qApp->setPalette(darkPalette);
    
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setOrganizationName(QStringLiteral("AutoRemesher"));
    
    QFont font;
    font.setWeight(QFont::Light);
    font.setBold(false);
    QApplication::setFont(font);
    
    Theme::initAwsomeBaseSizes();
    
    MainWindow *mainWindow = new MainWindow();
    mainWindow->setAttribute(Qt::WA_DeleteOnClose);
    if (!icon.isNull()) {
        mainWindow->setWindowIcon(icon);
    }
    QSize size = Preferences::instance().mainWindowSize();
    if (size.isValid()) {
        mainWindow->resize(size);
        mainWindow->show();
    } else {
        mainWindow->showMaximized();
    }
    
    return app.exec();
}
