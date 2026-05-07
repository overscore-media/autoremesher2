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
#ifndef AUTO_REMESHER_MAIN_WINDOW_H
#define AUTO_REMESHER_MAIN_WINDOW_H
#include <cstdint>
#include <QMainWindow>
#include <QCloseEvent>
#include <QShowEvent>
#include <QString>
#include <QSpinBox>
#include <QComboBox>
#include <QAction>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QLabel>
#include <queue>
#include <AutoRemesher/AutoRemesher>
#include <AutoRemesher/Vector3>
#include "pbrshaderwidget.h"

class QEvent;

class RenderMeshGenerator;
class QuadMeshGenerator;
class SpinnableAwesomeButton;
class QComboBox;
class QToolBar;

class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    struct ResultMesh
    {
        std::vector<AutoRemesher::Vector3> vertices;
        std::vector<std::vector<size_t>> faces;
        bool useSharedNormalization = false;
        AutoRemesher::Vector3 sharedOrigin;
        double sharedMaxLength = 1.0;
        enum Slot {
            PrimaryMesh,
            CompareMesh
        } destination = PrimaryMesh;
        uint64_t sceneGeneration = 0;
    };

    float targetDensity() const { return m_targetDensity; }
    float targetScaling() const { return m_targetScaling; }

    void applyPreferencesFromDialog(SettingsDialog &dlg);

    MainWindow();
    ~MainWindow();
    PbrShaderWidget *modelRenderWidget() const;
    static size_t total();
protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
private slots:
    void showAcknowledgements();
    void showAbout();
    void showHowToUse();
    void updateTitle();
    void loadModel();
    void saveMesh();
    bool loadObj(const QString &filename);
    void setCurrentFilename(const QString &filename);
    void checkRenderQueue();
    void renderMeshReady();
    void generateQuadMesh();
    void quadMeshReady();
    void updateButtonStates();
    void updateProgress(float progress);
    void setThreadCountFromSpinBox(int value);
    void toggleStatsViewer(bool checked);
    void showBackgroundColorDialog();
    void showModelColorDialog();
    void cancelRemesh();
    void restartRemesh();
    void resetToDefaultView();
    void toggleShowStats(bool checked);
    void updateStatsViewer();
    void showPreferencesDialog();
    void updateChromeVisibility();
    void syncStatsButtonFromPanel(bool visible);
    void syncCompareModeFromUi();
    void syncCompareHandleGeometry();
    void updateCompareSplitFromViewportX(int xInViewport);
    bool shouldShowCompareControls() const;
private:
    PbrShaderWidget *m_modelRenderWidget = nullptr;
    AutoRemesher::AutoRemesher *m_autoRemesher = nullptr;
    bool m_inProgress = false;
    bool m_saved = true;
    float m_targetDensity = 0.0;
    float m_targetScaling = 2.0;
    AutoRemesher::ModelType m_modelType = AutoRemesher::ModelType::Organic;
    std::vector<AutoRemesher::Vector3> m_originalVertices;
    std::vector<std::vector<size_t>> m_originalTriangles;
    std::vector<AutoRemesher::Vector3> *m_remeshedVertices = nullptr;
    std::vector<std::vector<size_t>> *m_remeshedQuads = nullptr;
    QString m_currentFilename;
    RenderMeshGenerator *m_renderMeshGenerator = nullptr;
    std::queue<ResultMesh> m_renderQueue;
    uint64_t m_sceneGeneration = 1;
    ResultMesh::Slot m_activeRenderDestination = ResultMesh::PrimaryMesh;
    uint64_t m_activeRenderJobSceneGeneration = 0;
    bool m_quadMeshResultIsDirty = false;
    QuadMeshGenerator *m_quadMeshGenerator = nullptr;
    bool m_remeshCompletedSuccessfully = false;
    SpinnableAwesomeButton *m_cancelRemeshButton = nullptr;
    SpinnableAwesomeButton *m_restartRemeshButton = nullptr;
    SpinnableAwesomeButton *m_loadModelButton = nullptr;
    SpinnableAwesomeButton *m_saveMeshButton = nullptr;
    SpinnableAwesomeButton *m_resetViewButton = nullptr;
    SpinnableAwesomeButton *m_toggleStatsButton = nullptr;
    QAction *m_toolbarStatsAction = nullptr;
    QAction *m_toolbarCancelAction = nullptr;
    QAction *m_toolbarRestartAction = nullptr;
    QAction *m_toolbarSaveAction = nullptr;
    QAction *m_toolbarResetAction = nullptr;
    QWidget *m_emptyStateOverlay = nullptr;
    QWidget *m_dragDropOverlay = nullptr;
    QWidget *m_viewportWrap = nullptr;
    QWidget *m_compareSplitHandle = nullptr;
    QLabel *m_compareLabelBefore = nullptr;
    QLabel *m_compareLabelAfter = nullptr;
    bool m_compareHandleDragging = false;
    bool m_dragHighlightActive = false;
    QAction *m_toggleStatsViewerAction = nullptr;
    QAction *m_showStatsAction = nullptr;
    QWidget *m_statsPanel = nullptr;
    QToolBar *m_mainToolBar = nullptr;
    QLabel *m_originalVertCountLabel = nullptr;
    QLabel *m_originalFaceCountLabel = nullptr;
    QLabel *m_remeshedVertCountLabel = nullptr;
    QLabel *m_remeshedFaceCountLabel = nullptr;
    QLabel *m_vertChangeLabel = nullptr;
    QLabel *m_faceChangeLabel = nullptr;
};

#endif
