#ifndef AUTO_REMESHER_SETTINGS_DIALOG_H
#define AUTO_REMESHER_SETTINGS_DIALOG_H

#include <QDialog>
#include <QVector3D>
#include <QCloseEvent>

class MainWindow;
class QLabel;
class QSlider;
class QComboBox;
class QSpinBox;
class QPushButton;
class QCheckBox;
class PbrShaderWidget;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(MainWindow *mainWindow, QWidget *parent = nullptr);

    int threadCountValue() const;
    float densityValue() const;
    int edgeScalingIndex() const;
    bool compareBarEnabledValue() const;

    void applyPendingColorsTo(PbrShaderWidget *gl);
    void syncBaselineFromApplied();

protected:
    void reject() override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void pickBackgroundColor();
    void pickModelColor();
    void resetBackgroundColor();
    void resetModelColor();
    void updateBackgroundSwatch();
    void updateModelSwatch();
    void updateDirtyState();

private:
    void captureBaselineFromWidgets();
    bool isDirty() const;

    MainWindow *m_mainWindow = nullptr;
    QSpinBox *m_threadSpin = nullptr;
    QSlider *m_densitySlider = nullptr;
    QLabel *m_densityValueLabel = nullptr;
    QComboBox *m_edgeScalingCombo = nullptr;
    QLabel *m_bgSwatch = nullptr;
    QLabel *m_modelSwatch = nullptr;
    QPushButton *m_applyButton = nullptr;
    QCheckBox *m_compareBarCheck = nullptr;

    QVector3D m_pendingBackground;
    QVector3D m_pendingModelDiffuse;
    bool m_pendingModelDiffuseCustom = false;

    int m_baseThreadCount = 1;
    float m_baseDensity = 0.f;
    int m_baseEdgeScalingIndex = 0;
    QVector3D m_baseBackground;
    QVector3D m_baseModelDiffuse;
    bool m_baseModelDiffuseCustom = false;
    bool m_baseShowCompareBar = true;
};

#endif
