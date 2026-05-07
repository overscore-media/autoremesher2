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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTextBrowser>
#include <QAction>
#include <QMenuBar>
#include <QUrl>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMimeData>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QFrame>
#include <QToolButton>
#include <QStackedLayout>
#include <QApplication>
#include <QThread>
#include <QDebug>
#include <utility>
#include <QTextStream>
#include <QFile>
#include <QComboBox>
#include <QSpinBox>
#include <QColorDialog>
#include <QToolBar>
#include <QDialog>
#include <QWidgetAction>
#include <QUuid>
#include <QEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QMouseEvent>
#include <cmath>
#include "mainwindow.h"
#include "graphicscontainerwidget.h"
#include "aboutwidget.h"
#include "howtousewidget.h"
#include "util.h"
#include "version.h"
#include "preferences.h"
#include "theme.h"
#include "rendermeshgenerator.h"
#include "quadmeshgenerator.h"
#include "spinnableawesomebutton.h"
#include "logbrowser.h"
#include "settingsdialog.h"
#include "objmeshio.h"

namespace {

static QString statsPercentChangeBracket(double originalCount, double newCount)
{
    if (originalCount <= 0.0)
        return QString();
    const double pct = (newCount - originalCount) / originalCount * 100.0;
    const double rounded = std::round(pct * 10.0) / 10.0;
    const QChar sign = rounded > 0   ? QLatin1Char('+')
                       : rounded < 0 ? QLatin1Char('-')
                                    : QLatin1Char('+');
    return QStringLiteral(" (%1%2%)")
        .arg(sign)
        .arg(QString::number(std::abs(rounded), 'f', 1));
}

} // namespace

LogBrowser *g_logBrowser = nullptr;
QTextBrowser *g_acknowledgementsWidget = nullptr;
AboutWidget *g_aboutWidget = nullptr;
HowToUseWidget *g_howToUseWidget = nullptr;
std::map<MainWindow *, QUuid> g_windows;

namespace {

bool mimeHasObjUrl(const QMimeData *mime)
{
    if (!mime || !mime->hasUrls())
        return false;
    for (const QUrl &url : mime->urls()) {
        const QString f = url.toLocalFile();
        if (!f.isEmpty() && f.endsWith(QStringLiteral(".obj"), Qt::CaseInsensitive))
            return true;
    }
    return false;
}

} // namespace

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_logBrowser)
        g_logBrowser->outputMessage(type, msg, context.file, context.line);
}

size_t MainWindow::total()
{
    return g_windows.size();
}

MainWindow::MainWindow()
{
    if (!g_logBrowser) {
        g_logBrowser = new LogBrowser;
        qInstallMessageHandler(&outputMessage);
    }

    g_windows.insert({this, QUuid::createUuid()});

    GraphicsWidget *graphicsWidget = new GraphicsWidget;
    
    GraphicsContainerWidget *containerWidget = new GraphicsContainerWidget;
    containerWidget->setGraphicsWidget(graphicsWidget);
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(1, 0, 0, 0);
    containerLayout->addWidget(graphicsWidget);
    containerWidget->setLayout(containerLayout);
    containerWidget->setMinimumSize(400, 400);
    
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *howToUseAction = new QAction(tr("Instructions"), this);
    connect(howToUseAction, &QAction::triggered, this, &MainWindow::showHowToUse);
    helpMenu->addAction(howToUseAction);

    QAction *showDebugDialogAction = new QAction(tr("Debug Console"), this);
    connect(showDebugDialogAction, &QAction::triggered, g_logBrowser, &LogBrowser::showDialog);
    helpMenu->addAction(showDebugDialogAction);

    QAction *showAcknowledgementsAction = new QAction(tr("Acknowledgements"), this);
    connect(showAcknowledgementsAction, &QAction::triggered, this, &MainWindow::showAcknowledgements);
    helpMenu->addAction(showAcknowledgementsAction);
    
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    QAction *preferencesAction = new QAction(tr("Preferences…"), this);
    settingsMenu->addAction(preferencesAction);
    connect(preferencesAction, &QAction::triggered, this, &MainWindow::showPreferencesDialog);

    m_modelRenderWidget = new PbrShaderWidget(containerWidget);
    m_modelRenderWidget->setMoveAndZoomByWindow(false);
    m_modelRenderWidget->move(0, 0);
    m_modelRenderWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_modelRenderWidget->toggleWireframe();
    m_modelRenderWidget->enableEnvironmentLight();
    m_modelRenderWidget->disableCullFace();
    m_modelRenderWidget->setEyePosition(QVector3D(0.0, 0.0, -4.0));
    m_modelRenderWidget->setBackgroundColor(PbrShaderWidget::defaultBackgroundColor());

    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        m_modelRenderWidget, &PbrShaderWidget::canvasResized);
    
    graphicsWidget->setModelWidget(m_modelRenderWidget);
    containerWidget->setModelWidget(m_modelRenderWidget);
    
    QHBoxLayout *toolLayout = new QHBoxLayout;
    toolLayout->setContentsMargins(0, 0, 0, 0);
    toolLayout->setSpacing(0);
    toolLayout->addStretch();

    SpinnableAwesomeButton *loadModelButton = new SpinnableAwesomeButton();
    loadModelButton->setAwesomeIcon(QChar(static_cast<char16_t>(fa::folderopeno)));
    connect(loadModelButton->button(), &QPushButton::clicked, this, &MainWindow::loadModel);

    m_loadModelButton = loadModelButton;
    
    m_viewportWrap = new QWidget;
    QStackedLayout *stackedCanvas = new QStackedLayout(m_viewportWrap);
    stackedCanvas->setStackingMode(QStackedLayout::StackAll);
    stackedCanvas->setContentsMargins(0, 0, 0, 0);
    stackedCanvas->addWidget(containerWidget);

    m_emptyStateOverlay = new QWidget(m_viewportWrap);
    m_emptyStateOverlay->setAutoFillBackground(true);
    {
        QPalette pal = m_emptyStateOverlay->palette();
        pal.setColor(QPalette::Window, QColor(38, 38, 38, 245));
        m_emptyStateOverlay->setPalette(pal);
    }
    QVBoxLayout *emptyLay = new QVBoxLayout(m_emptyStateOverlay);
    emptyLay->addStretch();
    QToolButton *openFileBtn = new QToolButton(m_emptyStateOverlay);
    {
        const int bigIconPx = Theme::toolIconFontSize * 5;
        QPixmap iconPm(bigIconPx, bigIconPx);
        iconPm.fill(Qt::transparent);
        {
            QPainter p(&iconPm);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setFont(Theme::awesome()->font(bigIconPx));
            p.setPen(Theme::white);
            p.drawText(iconPm.rect(), Qt::AlignCenter, QString(QChar(static_cast<char16_t>(fa::folderopeno))));
        }
        openFileBtn->setIcon(QIcon(iconPm));
        openFileBtn->setIconSize(QSize(bigIconPx, bigIconPx));
        openFileBtn->setText(tr("Open File"));
        openFileBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        openFileBtn->setCursor(Qt::PointingHandCursor);
        openFileBtn->setFocusPolicy(Qt::NoFocus);
        openFileBtn->setToolTip(tr("Open a Wavefront .obj file"));
        openFileBtn->setStyleSheet(QStringLiteral(
            "QToolButton { color: %1; background: transparent; border: 2px solid #6a6a6a; border-radius: 8px; padding: 14px 20px; }"
            "QToolButton:hover { border-color: #999; background-color: rgba(255,255,255,0.06); }")
            .arg(Theme::white.name()));
        connect(openFileBtn, &QToolButton::clicked, this, &MainWindow::loadModel);
    }
    emptyLay->addWidget(openFileBtn, 0, Qt::AlignCenter);
    QLabel *hint = new QLabel(tr("or drag and drop a Wavefront .obj file onto this window"));
    hint->setStyleSheet(QStringLiteral("color:#bbb;font-size:12px;"));
    hint->setAlignment(Qt::AlignCenter);
    emptyLay->addWidget(hint);
    emptyLay->addStretch();
    stackedCanvas->addWidget(m_emptyStateOverlay);

    m_dragDropOverlay = new QWidget(m_viewportWrap);
    m_dragDropOverlay->hide();
    m_dragDropOverlay->setAutoFillBackground(true);
    {
        QPalette pal = m_dragDropOverlay->palette();
        pal.setColor(QPalette::Window, QColor(38, 38, 38, 210));
        m_dragDropOverlay->setPalette(pal);
    }
    QVBoxLayout *dragLay = new QVBoxLayout(m_dragDropOverlay);
    QLabel *dragLbl = new QLabel(tr("<b>Drop .obj file to open</b>"));
    dragLbl->setAlignment(Qt::AlignCenter);
    dragLbl->setStyleSheet(QStringLiteral("color:white;font-size:18px;"));
    dragLay->addStretch();
    dragLay->addWidget(dragLbl);
    dragLay->addStretch();
    stackedCanvas->addWidget(m_dragDropOverlay);

    m_statsPanel = new QWidget;
    m_statsPanel->hide();
    m_statsPanel->setAutoFillBackground(true);
    {
        QPalette stPal = m_statsPanel->palette();
        stPal.setColor(QPalette::Window, QColor(42, 42, 42));
        m_statsPanel->setPalette(stPal);
    }
    m_statsPanel->setMinimumWidth(160);
    m_statsPanel->setMaximumWidth(280);
    QVBoxLayout *statsLayout = new QVBoxLayout(m_statsPanel);
    statsLayout->setContentsMargins(5, 5, 5, 5);

    QLabel *originalLabel = new QLabel(tr("<b>Original</b>"));
    statsLayout->addWidget(originalLabel);

    QHBoxLayout *originalVertLayout = new QHBoxLayout;
    originalVertLayout->setContentsMargins(0, 0, 0, 0);
    originalVertLayout->addWidget(new QLabel(tr("Vertices:")));
    m_originalVertCountLabel = new QLabel("0");
    originalVertLayout->addWidget(m_originalVertCountLabel);
    statsLayout->addLayout(originalVertLayout);

    QHBoxLayout *originalFaceLayout = new QHBoxLayout;
    originalFaceLayout->setContentsMargins(0, 0, 0, 0);
    originalFaceLayout->addWidget(new QLabel(tr("Faces:")));
    m_originalFaceCountLabel = new QLabel("0");
    originalFaceLayout->addWidget(m_originalFaceCountLabel);
    statsLayout->addLayout(originalFaceLayout);

    statsLayout->addSpacing(10);

    QLabel *remeshedLabel = new QLabel(tr("<b>Remeshed</b>"));
    statsLayout->addWidget(remeshedLabel);

    QHBoxLayout *remeshedVertLayout = new QHBoxLayout;
    remeshedVertLayout->setContentsMargins(0, 0, 0, 0);
    remeshedVertLayout->addWidget(new QLabel(tr("Vertices:")));
    m_remeshedVertCountLabel = new QLabel("0");
    remeshedVertLayout->addWidget(m_remeshedVertCountLabel);
    m_vertChangeLabel = new QLabel("");
    remeshedVertLayout->addWidget(m_vertChangeLabel);
    statsLayout->addLayout(remeshedVertLayout);

    QHBoxLayout *remeshedFaceLayout = new QHBoxLayout;
    remeshedFaceLayout->setContentsMargins(0, 0, 0, 0);
    remeshedFaceLayout->addWidget(new QLabel(tr("Faces:")));
    m_remeshedFaceCountLabel = new QLabel("0");
    remeshedFaceLayout->addWidget(m_remeshedFaceCountLabel);
    m_faceChangeLabel = new QLabel("");
    remeshedFaceLayout->addWidget(m_faceChangeLabel);
    statsLayout->addLayout(remeshedFaceLayout);

    statsLayout->addStretch();

    m_compareLabelAfter = new QLabel(tr("<b>Remeshed</b>"), m_viewportWrap);
    m_compareLabelBefore = new QLabel(tr("<b>Original</b>"), m_viewportWrap);
    {
        const QString cmpLblStyle = QStringLiteral(
            "QLabel { color: #eee; background: rgba(0,0,0,0.5); padding: 3px 10px 0 10px; border-radius: 4px; }");
        m_compareLabelAfter->setTextFormat(Qt::RichText);
        m_compareLabelBefore->setTextFormat(Qt::RichText);
        m_compareLabelAfter->setStyleSheet(cmpLblStyle);
        m_compareLabelBefore->setStyleSheet(cmpLblStyle);
        m_compareLabelAfter->hide();
        m_compareLabelBefore->hide();
    }

    m_compareSplitHandle = new QWidget(m_viewportWrap);
    m_compareSplitHandle->hide();
    m_compareSplitHandle->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_compareSplitHandle->setCursor(Qt::SplitHCursor);
    m_compareSplitHandle->setStyleSheet(QStringLiteral(
        "QWidget { background-color: rgba(170, 170, 178, 0.92); border: none; }"));
    m_compareSplitHandle->raise();
    m_compareSplitHandle->installEventFilter(this);

    QHBoxLayout *canvasLayout = new QHBoxLayout;
    canvasLayout->setSpacing(0);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->addWidget(m_statsPanel);
    canvasLayout->addWidget(m_viewportWrap, 1);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(toolLayout);
    mainLayout->addLayout(canvasLayout);

    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);

    setCentralWidget(centralWidget);
    setAcceptDrops(true);
    centralWidget->setAcceptDrops(true);
    if (m_viewportWrap)
        m_viewportWrap->setAcceptDrops(true);
    
    QToolBar *toolbar = new QToolBar(tr("Main Toolbar"));
    toolbar->setMovable(false);
    addToolBar(toolbar);
    m_mainToolBar = toolbar;
    
    QWidget *stretchLeft = new QWidget();
    stretchLeft->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_toggleStatsButton = new SpinnableAwesomeButton();
    m_toggleStatsButton->setAwesomeIcon(QChar(static_cast<char16_t>(fa::barchart)));
    m_toggleStatsButton->setToolTip(tr("Show stats"));
    m_toggleStatsButton->setCheckable(true);
    m_toggleStatsButton->setChecked(false);
    m_toggleStatsButton->button()->setCheckable(true);
    m_toolbarStatsAction = toolbar->addWidget(m_toggleStatsButton);
    connect(m_toggleStatsButton->button(), &QPushButton::clicked, this, [this]() {
        if (!m_statsPanel)
            return;
        m_statsPanel->setVisible(!m_statsPanel->isVisible());
        syncStatsButtonFromPanel(m_statsPanel->isVisible());
    });

    m_resetViewButton = new SpinnableAwesomeButton();
    m_resetViewButton->setAwesomeIcon(QChar(static_cast<char16_t>(fa::compress)));
    m_resetViewButton->setToolTip(tr("Reset to Default View"));
    m_toolbarResetAction = toolbar->addWidget(m_resetViewButton);
    connect(m_resetViewButton->button(), &QPushButton::clicked, this, &MainWindow::resetToDefaultView);

    toolbar->addWidget(stretchLeft);

    m_cancelRemeshButton = new SpinnableAwesomeButton();
    m_cancelRemeshButton->setAwesomeIcon(QChar(static_cast<char16_t>(fa::timescircle)));
    m_cancelRemeshButton->setToolTip(tr("Cancel Remesh"));
    m_toolbarCancelAction = toolbar->addWidget(m_cancelRemeshButton);
    connect(m_cancelRemeshButton->button(), &QPushButton::clicked, this, &MainWindow::cancelRemesh);

    m_restartRemeshButton = new SpinnableAwesomeButton();
    m_restartRemeshButton->setAwesomeIcon(QChar(static_cast<char16_t>(fa::refresh)));
    m_restartRemeshButton->setToolTip(tr("Restart Remesh operation"));
    m_toolbarRestartAction = toolbar->addWidget(m_restartRemeshButton);
    connect(m_restartRemeshButton->button(), &QPushButton::clicked, this, &MainWindow::restartRemesh);

    m_saveMeshButton = new SpinnableAwesomeButton();
    m_saveMeshButton->setAwesomeIcon(QChar(static_cast<char16_t>(fa::save)));
    m_toolbarSaveAction = toolbar->addWidget(m_saveMeshButton);
    connect(m_saveMeshButton->button(), &QPushButton::clicked, this, &MainWindow::saveMesh);

    toolbar->addWidget(loadModelButton);

    if (m_toggleStatsButton)
        m_toggleStatsButton->setVisible(false);
    if (m_cancelRemeshButton)
        m_cancelRemeshButton->setVisible(false);
    if (m_restartRemeshButton)
        m_restartRemeshButton->setVisible(false);
    if (m_resetViewButton)
        m_resetViewButton->setVisible(false);
    if (m_saveMeshButton)
        m_saveMeshButton->setVisible(false);

    if (m_emptyStateOverlay)
        m_emptyStateOverlay->installEventFilter(this);
    if (m_dragDropOverlay)
        m_dragDropOverlay->installEventFilter(this);
    if (m_viewportWrap)
        m_viewportWrap->installEventFilter(this);
    centralWidget->installEventFilter(this);

    if (m_modelRenderWidget)
        m_modelRenderWidget->setCompareSplit(0.f);

    updateTitle();
    updateChromeVisibility();
}

void MainWindow::updateButtonStates()
{
    bool isProcessing = (nullptr != m_quadMeshGenerator || m_quadMeshResultIsDirty);

    if (m_loadModelButton) {
        if (!isProcessing)
            m_loadModelButton->showSpinner(false);
        else
            m_loadModelButton->showSpinner(true);
    }

    updateChromeVisibility();
}

bool MainWindow::loadObj(const QString &filename)
{
    qDebug() << "loadObj:" << filename;

    std::vector<AutoRemesher::Vector3> vertices;
    std::vector<std::vector<size_t>> triangles;
    QString loadErr;
    if (!ObjMeshIo::loadWavefrontObj(filename, vertices, triangles, &loadErr)) {
        if (!loadErr.isEmpty())
            qDebug() << loadErr;
        return false;
    }

    ++m_sceneGeneration;
    while (!m_renderQueue.empty())
        m_renderQueue.pop();
    if (m_modelRenderWidget) {
        m_modelRenderWidget->clearCompareMesh();
        m_modelRenderWidget->setCompareModeEnabled(false);
    }
    delete m_remeshedVertices;
    m_remeshedVertices = nullptr;
    delete m_remeshedQuads;
    m_remeshedQuads = nullptr;
    m_remeshCompletedSuccessfully = false;

    m_originalVertices = std::move(vertices);
    m_originalTriangles = std::move(triangles);

    qDebug() << "m_originalVertices.size():" << m_originalVertices.size();
    qDebug() << "m_originalTriangles.size():" << m_originalTriangles.size();

    ResultMesh primary;
    primary.vertices = m_originalVertices;
    primary.faces = m_originalTriangles;
    primary.useSharedNormalization = false;
    primary.destination = ResultMesh::PrimaryMesh;
    primary.sceneGeneration = m_sceneGeneration;
    m_renderQueue.push(std::move(primary));
    checkRenderQueue();

    return true;
}

void MainWindow::loadModel()
{
    if (!m_saved) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to load another file and lose the unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }
    
    if (m_inProgress) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to load another file and lose the in progress operations?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
    }
    
    QString filename = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Wavefront (*.obj)"));
    if (filename.isEmpty())
        return;
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool objLoaded = loadObj(filename);
    QApplication::restoreOverrideCursor();
    
    if (objLoaded) {
        setCurrentFilename(filename);
        Preferences::instance().setTrackVertCount(m_originalVertices.size());
        Preferences::instance().setTrackFaceCount(m_originalTriangles.size());
        updateStatsViewer();
        updateButtonStates();
        generateQuadMesh();
    }
}

void MainWindow::setCurrentFilename(const QString &filename)
{
    m_currentFilename = filename;
    m_saved = true;
    updateTitle();
}

void MainWindow::saveMesh()
{
    if (nullptr == m_remeshedVertices || nullptr == m_remeshedQuads) {
        qDebug() << "Save failed: no remeshed data";
        return;
    }
    
    qDebug() << "Saving remeshed model with" << m_remeshedVertices->size() << "vertices and" << m_remeshedQuads->size() << "faces";
    
    QString filename = QFileDialog::getSaveFileName(this, QString(), QString(),
       tr("Wavefront (*.obj)"));
    if (filename.isEmpty()) {
        return;
    }
    
    if (!filename.endsWith(".obj"))
        filename += ".obj";
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
        return;
    QString writeErr;
    if (!ObjMeshIo::writeQuadObj(file, *m_remeshedVertices, *m_remeshedQuads, &writeErr))
        qDebug() << writeErr;
}

void MainWindow::updateTitle()
{
    QString appName = APP_NAME;
    QString appVer = APP_HUMAN_VER;
    setWindowTitle(QString("%1 %2 %3%4").arg(appName).arg(appVer).arg(m_currentFilename).arg(m_saved ? "" : "*"));
}

void MainWindow::updateProgress(float progress)
{
    Q_UNUSED(progress);
}

MainWindow::~MainWindow()
{
    g_windows.erase(this);
}

PbrShaderWidget *MainWindow::modelRenderWidget() const
{
    return m_modelRenderWidget;
}

void MainWindow::showAcknowledgements()
{
    if (!g_acknowledgementsWidget) {
        g_acknowledgementsWidget = new QTextBrowser;
        g_acknowledgementsWidget->setWindowTitle(unifiedWindowTitle(tr("Acknowledgements")));
        g_acknowledgementsWidget->setMinimumSize(QSize(640, 380));
        QFile file(":/ACKNOWLEDGEMENTS.html");
        file.open(QFile::ReadOnly | QFile::Text);
        QTextStream stream(&file);
        QString html = stream.readAll();
        QFile supportersFile(QStringLiteral(":/SUPPORTERS"));
        if (supportersFile.open(QFile::ReadOnly | QFile::Text)) {
            const QString sup = QString::fromUtf8(supportersFile.readAll());
            html += QStringLiteral("<hr/><h2>SUPPORTERS</h2><pre>%1</pre>").arg(sup.toHtmlEscaped());
        }
        g_acknowledgementsWidget->setHtml(html);
    }
    g_acknowledgementsWidget->show();
    g_acknowledgementsWidget->activateWindow();
    g_acknowledgementsWidget->raise();
}

void MainWindow::showAbout()
{
    if (!g_aboutWidget) {
        g_aboutWidget = new AboutWidget;
    }
    g_aboutWidget->show();
    g_aboutWidget->activateWindow();
    g_aboutWidget->raise();
}

void MainWindow::showHowToUse()
{
    if (!g_howToUseWidget) {
        g_howToUseWidget = new HowToUseWidget;
    }
    g_howToUseWidget->show();
    g_howToUseWidget->activateWindow();
    g_howToUseWidget->raise();
}

void MainWindow::showEvent(QShowEvent *event)
{
    updateChromeVisibility();
    event->accept();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_saved) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to close while there are unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    
    if (m_inProgress) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to close while there are in progress operations?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    
    QSize saveSize;
    if (!isMaximized())
        saveSize = size();
    Preferences::instance().setMainWindowSize(saveSize);

    event->accept();
}

void MainWindow::checkRenderQueue()
{
    if (nullptr != m_renderMeshGenerator)
        return;

    if (m_renderQueue.empty())
        return;

    qDebug() << "Generate render mesh...";

    QThread *thread = new QThread;

    ResultMesh item = std::move(m_renderQueue.front());
    m_renderQueue.pop();
    m_activeRenderDestination = item.destination;
    m_activeRenderJobSceneGeneration = item.sceneGeneration;

    m_renderMeshGenerator = new RenderMeshGenerator(item.vertices, item.faces,
        item.useSharedNormalization,
        item.sharedOrigin,
        item.sharedMaxLength);

    m_renderMeshGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_renderMeshGenerator, &RenderMeshGenerator::process);
    connect(m_renderMeshGenerator, &RenderMeshGenerator::finished, this, &MainWindow::renderMeshReady);
    connect(m_renderMeshGenerator, &RenderMeshGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MainWindow::renderMeshReady()
{
    PbrShaderMesh *renderMesh = m_renderMeshGenerator->takeRenderMesh();
    const ResultMesh::Slot destination = m_activeRenderDestination;
    const uint64_t jobSceneGeneration = m_activeRenderJobSceneGeneration;

    delete m_renderMeshGenerator;
    m_renderMeshGenerator = nullptr;

    if (jobSceneGeneration != m_sceneGeneration) {
        delete renderMesh;
        checkRenderQueue();
        return;
    }

    if (destination == ResultMesh::CompareMesh) {
        if (m_modelRenderWidget)
            m_modelRenderWidget->setCompareMesh(renderMesh);
    } else {
        if (m_modelRenderWidget)
            m_modelRenderWidget->updateMesh(renderMesh);
    }

    syncCompareModeFromUi();
    checkRenderQueue();
}

void MainWindow::generateQuadMesh()
{
    if (nullptr != m_quadMeshGenerator) {
        m_quadMeshResultIsDirty = true;
        return;
    }
    
    m_quadMeshResultIsDirty = false;
    m_saved = true;
    m_inProgress = true;
    if (m_cancelRemeshButton)
        m_cancelRemeshButton->button()->setEnabled(true);
    updateChromeVisibility();

    QThread *thread = new QThread;
    
    QuadMeshGenerator::Parameters parameters;
    
    {
        const int base = 100000;
        const int range = 500000;
        parameters.targetTriangleCount = base + range * m_targetDensity;
    }
    {
        parameters.scaling = m_targetScaling;
    }
    parameters.modelType = m_modelType;
    parameters.threadCount = Preferences::instance().threadCount();
    
    m_quadMeshGenerator = new QuadMeshGenerator(m_originalVertices, m_originalTriangles);
    connect(m_quadMeshGenerator, &QuadMeshGenerator::reportProgress, this, &MainWindow::updateProgress);
    m_quadMeshGenerator->setParameters(parameters);
    m_quadMeshGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_quadMeshGenerator, &QuadMeshGenerator::process);
    connect(m_quadMeshGenerator, &QuadMeshGenerator::finished, this, &MainWindow::quadMeshReady);
    connect(m_quadMeshGenerator, &QuadMeshGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
    
    updateButtonStates();
    updateTitle();
}

void MainWindow::quadMeshReady()
{
    qDebug() << "DONE DONE SUPER DONE";
    qDebug() << "quadMeshReady called, m_originalVertices.size=" << m_originalVertices.size();
    delete m_remeshedVertices;
    m_remeshedVertices = m_quadMeshGenerator->takeRemeshedVertices();
    
    delete m_remeshedQuads;
    m_remeshedQuads = m_quadMeshGenerator->takeRemeshedQuads();
    
    m_saved = false;
    m_inProgress = false;

    qDebug() << "quadMeshReady: processing done";

    QApplication::processEvents();
    update();

    delete m_quadMeshGenerator;
    m_quadMeshGenerator = nullptr;

    m_remeshCompletedSuccessfully = (m_remeshedVertices != nullptr && m_remeshedQuads != nullptr
        && !m_remeshedVertices->empty() && !m_remeshedQuads->empty());

    if (m_remeshCompletedSuccessfully) {
        AutoRemesher::Vector3 normOrigin;
        double normLen = 1.0;
        RenderMeshGenerator::combinedBoundingNormalizationFactors(m_originalVertices, *m_remeshedVertices,
            &normOrigin, &normLen);

        ResultMesh compareJob;
        compareJob.vertices = m_originalVertices;
        compareJob.faces = m_originalTriangles;
        compareJob.useSharedNormalization = true;
        compareJob.sharedOrigin = normOrigin;
        compareJob.sharedMaxLength = normLen;
        compareJob.destination = ResultMesh::CompareMesh;
        compareJob.sceneGeneration = m_sceneGeneration;
        m_renderQueue.push(std::move(compareJob));

        ResultMesh primaryJob;
        primaryJob.vertices = *m_remeshedVertices;
        primaryJob.faces = *m_remeshedQuads;
        primaryJob.useSharedNormalization = true;
        primaryJob.sharedOrigin = normOrigin;
        primaryJob.sharedMaxLength = normLen;
        primaryJob.destination = ResultMesh::PrimaryMesh;
        primaryJob.sceneGeneration = m_sceneGeneration;
        m_renderQueue.push(std::move(primaryJob));

        checkRenderQueue();
    }

    if (m_quadMeshResultIsDirty)
        generateQuadMesh();

    if (m_remeshCompletedSuccessfully && Preferences::instance().showCompareBar() && m_modelRenderWidget)
        m_modelRenderWidget->setCompareSplit(0.f);

    if (m_cancelRemeshButton)
        m_cancelRemeshButton->button()->setEnabled(true);

    updateButtonStates();
    updateTitle();

    updateStatsViewer();
}

void MainWindow::setThreadCountFromSpinBox(int value)
{
    Preferences::instance().setThreadCount(value);
}

void MainWindow::toggleStatsViewer(bool checked)
{
    if (m_statsPanel) {
        m_statsPanel->setVisible(checked);
    }
    syncStatsButtonFromPanel(checked);
    if (m_showStatsAction) {
        m_showStatsAction->setChecked(checked);
    }
}

void MainWindow::showBackgroundColorDialog()
{
    QColorDialog colorDialog(this);
    colorDialog.setWindowTitle(tr("Select Background Color"));
    if (colorDialog.exec() == QDialog::Accepted) {
        QColor color = colorDialog.currentColor();
        if (m_modelRenderWidget) {
            m_modelRenderWidget->setBackgroundColor(QVector3D(color.redF(), color.greenF(), color.blueF()));
            m_modelRenderWidget->reRender();
        }
    }
}

void MainWindow::showModelColorDialog()
{
    QColorDialog colorDialog(this);
    colorDialog.setWindowTitle(tr("Select Model Color"));
    if (colorDialog.exec() == QDialog::Accepted) {
        QColor color = colorDialog.currentColor();
        if (m_modelRenderWidget) {
            m_modelRenderWidget->setModelDiffuseColor(QVector3D(color.redF(), color.greenF(), color.blueF()));
            m_modelRenderWidget->reRender();
        }
    }
}

void MainWindow::cancelRemesh()
{
    if (!m_quadMeshGenerator)
        return;
    m_quadMeshGenerator->requestCancel();
    m_quadMeshResultIsDirty = false;
    if (m_cancelRemeshButton)
        m_cancelRemeshButton->button()->setEnabled(false);
    QMessageBox::information(this, APP_NAME,
        tr("Remesh cancellation requested."));
    updateChromeVisibility();
}

void MainWindow::restartRemesh()
{
    if (m_originalVertices.empty() || nullptr != m_quadMeshGenerator || m_inProgress)
        return;
    generateQuadMesh();
}

void MainWindow::resetToDefaultView()
{
    if (m_modelRenderWidget) {
        m_modelRenderWidget->resetToDefaultView();
    }
}

void MainWindow::toggleShowStats(bool checked)
{
    if (m_statsPanel) {
        m_statsPanel->setVisible(checked);
    }
    syncStatsButtonFromPanel(checked);
}

void MainWindow::updateStatsViewer()
{
    bool hasModel = !m_originalVertices.empty();

    if (m_showStatsAction) {
        m_showStatsAction->setChecked(hasModel);
    }

    if (!hasModel) {
        return;
    }
    
    if (m_originalVertCountLabel) {
        m_originalVertCountLabel->setText(QString::number(Preferences::instance().trackVertCount()));
    }
    if (m_originalFaceCountLabel) {
        m_originalFaceCountLabel->setText(QString::number(Preferences::instance().trackFaceCount()));
    }
    if (m_remeshedVertCountLabel) {
        if (m_inProgress) {
            m_remeshedVertCountLabel->setText("...");
        } else if (m_remeshedVertices) {
            m_remeshedVertCountLabel->setText(QString::number(m_remeshedVertices->size()));
        } else {
            m_remeshedVertCountLabel->setText("-");
        }
    }
    if (m_remeshedFaceCountLabel) {
        if (m_inProgress) {
            m_remeshedFaceCountLabel->setText("...");
        } else if (m_remeshedQuads) {
            m_remeshedFaceCountLabel->setText(QString::number(m_remeshedQuads->size()));
        } else {
            m_remeshedFaceCountLabel->setText("-");
        }
    }
    
    if (m_vertChangeLabel && m_faceChangeLabel) {
        if (!m_inProgress && m_remeshedVertices && m_remeshedQuads) {
            const int origVert = Preferences::instance().trackVertCount();
            const int newVert = static_cast<int>(m_remeshedVertices->size());
            const int origFace = Preferences::instance().trackFaceCount();
            const int newFace = static_cast<int>(m_remeshedQuads->size());
            m_vertChangeLabel->setText(statsPercentChangeBracket(origVert, newVert));
            m_faceChangeLabel->setText(statsPercentChangeBracket(origFace, newFace));
        } else {
            m_vertChangeLabel->setText("");
            m_faceChangeLabel->setText("");
        }
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (mimeHasObjUrl(event->mimeData())) {
        event->acceptProposedAction();
        if (m_dragDropOverlay) {
            m_dragDropOverlay->show();
            m_dragDropOverlay->raise();
        }
        m_dragHighlightActive = true;
    } else {
        event->ignore();
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (mimeHasObjUrl(event->mimeData())) {
        event->acceptProposedAction();
        if (m_dragDropOverlay) {
            m_dragDropOverlay->show();
            m_dragDropOverlay->raise();
        }
        m_dragHighlightActive = true;
    } else {
        event->ignore();
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    if (m_dragDropOverlay)
        m_dragDropOverlay->hide();
    m_dragHighlightActive = false;
}

void MainWindow::dropEvent(QDropEvent *event)
{
    auto hideDragOverlay = [this]() {
        if (m_dragDropOverlay)
            m_dragDropOverlay->hide();
        m_dragHighlightActive = false;
    };

    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        hideDragOverlay();
        event->acceptProposedAction();
        return;
    }

    QString filename = urls.first().toLocalFile();
    if (filename.isEmpty() || !filename.endsWith(".obj", Qt::CaseInsensitive)) {
        hideDragOverlay();
        event->acceptProposedAction();
        return;
    }
    
    if (!m_originalVertices.empty()) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to load another file and lose the unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            hideDragOverlay();
            event->acceptProposedAction();
            return;
        }

        if (m_inProgress) {
            QMessageBox::StandardButton answer2 = QMessageBox::question(this,
                APP_NAME,
                tr("Do you really want to load another file and lose the in progress operations?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (answer2 != QMessageBox::Yes) {
                hideDragOverlay();
                event->acceptProposedAction();
                return;
            }
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool objLoaded = loadObj(filename);
    QApplication::restoreOverrideCursor();

    hideDragOverlay();

    if (objLoaded) {
        setCurrentFilename(filename);
        Preferences::instance().setTrackVertCount(m_originalVertices.size());
        Preferences::instance().setTrackFaceCount(m_originalTriangles.size());
        updateStatsViewer();
        updateButtonStates();
        generateQuadMesh();
    }

    event->acceptProposedAction();
}

void MainWindow::applyPreferencesFromDialog(SettingsDialog &dlg)
{
    setThreadCountFromSpinBox(dlg.threadCountValue());
    m_targetDensity = dlg.densityValue();
    m_targetScaling = float(dlg.edgeScalingIndex() + 1);
    Preferences::instance().setShowCompareBar(dlg.compareBarEnabledValue());
    dlg.applyPendingColorsTo(m_modelRenderWidget);
    dlg.syncBaselineFromApplied();
    updateChromeVisibility();
}

void MainWindow::showPreferencesDialog()
{
    SettingsDialog dlg(this, this);
    dlg.exec();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_compareSplitHandle) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton && m_modelRenderWidget) {
                m_compareHandleDragging = true;
                m_compareSplitHandle->grabMouse();
                updateCompareSplitFromViewportX(m_modelRenderWidget->mapFromGlobal(me->globalPos()).x());
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (m_compareHandleDragging && (me->buttons() & Qt::LeftButton) && m_modelRenderWidget) {
                updateCompareSplitFromViewportX(m_modelRenderWidget->mapFromGlobal(me->globalPos()).x());
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton && m_compareHandleDragging) {
                m_compareHandleDragging = false;
                m_compareSplitHandle->releaseMouse();
                return true;
            }
            break;
        }
        default:
            break;
        }
    }

    if (watched == m_viewportWrap && event->type() == QEvent::Resize)
        syncCompareHandleGeometry();

    const bool overlayDropSurface = watched == m_emptyStateOverlay || watched == m_dragDropOverlay
        || watched == m_viewportWrap || watched == centralWidget();

    if (overlayDropSurface) {
        switch (event->type()) {
        case QEvent::DragEnter:
            dragEnterEvent(static_cast<QDragEnterEvent *>(event));
            return true;
        case QEvent::DragMove:
            dragMoveEvent(static_cast<QDragMoveEvent *>(event));
            return true;
        case QEvent::DragLeave:
            dragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
            return true;
        case QEvent::Drop:
            dropEvent(static_cast<QDropEvent *>(event));
            return true;
        default:
            break;
        }
    }
    return QObject::eventFilter(watched, event);
}

void MainWindow::updateChromeVisibility()
{
    bool hasModel = !m_originalVertices.empty();
    menuBar()->setVisible(true);
    if (m_mainToolBar)
        m_mainToolBar->setVisible(true);
    bool remeshActive = m_inProgress || (m_quadMeshGenerator != nullptr);
    bool remeshDone = hasModel && m_remeshCompletedSuccessfully && !remeshActive;
    const bool showComparePref = Preferences::instance().showCompareBar();

    auto setSpinToolVisible = [](SpinnableAwesomeButton *btn, QAction *act, bool vis) {
        if (act)
            act->setVisible(vis);
        if (!btn)
            return;
        btn->setVisible(vis);
        btn->button()->setVisible(vis);
        if (!vis && btn->isSpinning())
            btn->showSpinner(false);
    };

    if (m_loadModelButton)
        m_loadModelButton->setVisible(hasModel);

    setSpinToolVisible(m_cancelRemeshButton, m_toolbarCancelAction, remeshActive);
    setSpinToolVisible(m_restartRemeshButton, m_toolbarRestartAction,
        hasModel && !remeshActive && !m_remeshCompletedSuccessfully);
    setSpinToolVisible(m_saveMeshButton, m_toolbarSaveAction, remeshDone);
    setSpinToolVisible(m_resetViewButton, m_toolbarResetAction, remeshDone);
    setSpinToolVisible(m_toggleStatsButton, m_toolbarStatsAction, remeshDone);

    if (m_emptyStateOverlay) {
        m_emptyStateOverlay->setVisible(!hasModel);
        if (!hasModel)
            m_emptyStateOverlay->raise();
    }

    if (m_modelRenderWidget) {
        if (!remeshDone) {
            m_modelRenderWidget->setCompareModeEnabled(false);
            m_modelRenderWidget->clearCompareMesh();
        } else if (!showComparePref) {
            m_modelRenderWidget->setCompareModeEnabled(false);
        }
    }

    syncCompareModeFromUi();
}

bool MainWindow::shouldShowCompareControls() const
{
    if (!m_modelRenderWidget || m_originalVertices.empty())
        return false;
    const bool remeshActive = m_inProgress || (m_quadMeshGenerator != nullptr);
    if (!m_remeshCompletedSuccessfully || remeshActive)
        return false;
    if (!Preferences::instance().showCompareBar())
        return false;
    return m_modelRenderWidget->compareSplitReady();
}

void MainWindow::syncCompareHandleGeometry()
{
    if (!m_compareSplitHandle || !m_viewportWrap || !m_modelRenderWidget)
        return;
    QWidget *glHost = m_modelRenderWidget->parentWidget();
    if (!glHost)
        return;

    const bool showHandle = shouldShowCompareControls();

    if (!showHandle) {
        m_compareSplitHandle->hide();
        if (m_compareLabelBefore)
            m_compareLabelBefore->hide();
        if (m_compareLabelAfter)
            m_compareLabelAfter->hide();
        return;
    }

    const int handleW = 8;
    const float t = m_modelRenderWidget->compareSplit();
    const int mw = m_modelRenderWidget->width();
    const int mh = m_modelRenderWidget->height();
    if (mw < 1 || mh < 1) {
        m_compareSplitHandle->hide();
        return;
    }

    const int splitPx = qBound(0, int(qRound(t * float(mw))), mw);
    const QPoint hostTopLeft = glHost->mapTo(m_viewportWrap, QPoint(0, 0));
    const int cx = hostTopLeft.x() + splitPx - handleW / 2;

    m_compareSplitHandle->setGeometry(cx, hostTopLeft.y(), handleW, mh);
    m_compareSplitHandle->show();

    if (m_compareLabelAfter && m_compareLabelBefore) {
        m_compareLabelAfter->adjustSize();
        m_compareLabelBefore->adjustSize();
        const int pad = 6;
        const int gap = 4;
        const int ay = hostTopLeft.y() + pad;
        const int vx0 = hostTopLeft.x();
        const int lineX = vx0 + splitPx;
        const int dockEps = 3;

        if (splitPx <= dockEps) {
            m_compareLabelAfter->move(vx0 + gap, ay);
            m_compareLabelAfter->show();
            m_compareLabelBefore->hide();
        } else if (splitPx >= mw - dockEps) {
            m_compareLabelBefore->move(vx0 + mw - m_compareLabelBefore->width() - gap, ay);
            m_compareLabelBefore->show();
            m_compareLabelAfter->hide();
        } else {
            const int xOriginal = lineX - gap - m_compareLabelBefore->width();
            const int xRemeshed = lineX + gap;
            m_compareLabelBefore->move(qBound(vx0 + 2, xOriginal, vx0 + mw - m_compareLabelBefore->width() - 2), ay);
            m_compareLabelAfter->move(qBound(vx0 + 2, xRemeshed, vx0 + mw - m_compareLabelAfter->width() - 2), ay);
            m_compareLabelBefore->show();
            m_compareLabelAfter->show();
        }
    }

    if (m_compareLabelAfter)
        m_compareLabelAfter->raise();
    if (m_compareLabelBefore)
        m_compareLabelBefore->raise();
    m_compareSplitHandle->raise();
}

void MainWindow::updateCompareSplitFromViewportX(int xInModelPixels)
{
    if (!m_modelRenderWidget)
        return;

    const int mw = m_modelRenderWidget->width();
    if (mw < 1)
        return;

    const int x = qBound(0, xInModelPixels, mw);
    const float t = float(x) / float(mw);
    m_modelRenderWidget->setCompareSplit(t);
    syncCompareHandleGeometry();
}

void MainWindow::syncStatsButtonFromPanel(bool visible)
{
    if (m_toggleStatsButton) {
        m_toggleStatsButton->setChecked(visible);
        m_toggleStatsButton->setStrikeThrough(visible);
        m_toggleStatsButton->setToolTip(visible ? tr("Hide stats") : tr("Show stats"));
    }
}

void MainWindow::syncCompareModeFromUi()
{
    if (m_modelRenderWidget)
        m_modelRenderWidget->setCompareModeEnabled(shouldShowCompareControls());
    syncCompareHandleGeometry();
}
