#include "settingsdialog.h"
#include "mainwindow.h"
#include "preferences.h"
#include "pbrshaderwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QSlider>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QColorDialog>
#include <QThread>
#include <QVector3D>
#include <QMessageBox>
#include <QtGlobal>

#include "version.h"

static QString vectorToStyleSheet(const QVector3D &c)
{
    return QString("background-color: rgb(%1, %2, %3); border: 1px solid #222;")
        .arg(qRound(c.x() * 255.0))
        .arg(qRound(c.y() * 255.0))
        .arg(qRound(c.z() * 255.0));
}

SettingsDialog::SettingsDialog(MainWindow *mainWindow, QWidget *parent) :
    QDialog(parent),
    m_mainWindow(mainWindow)
{
    setWindowTitle(tr("Preferences — %1").arg(APP_NAME));
    setModal(true);
    setMinimumWidth(420);

    m_pendingBackground = PbrShaderWidget::defaultBackgroundColor();
    m_pendingModelDiffuse = QVector3D(0.85f, 0.85f, 0.85f);
    m_pendingModelDiffuseCustom = false;

    if (m_mainWindow && m_mainWindow->modelRenderWidget()) {
        PbrShaderWidget *gl = m_mainWindow->modelRenderWidget();
        QVector3D bg = gl->backgroundColor();
        if (!bg.isNull())
            m_pendingBackground = bg;
        m_pendingModelDiffuseCustom = gl->isModelDiffuseColorEnabled();
        if (m_pendingModelDiffuseCustom)
            m_pendingModelDiffuse = gl->modelDiffuseColor();
    }

    QVBoxLayout *root = new QVBoxLayout(this);

    QGroupBox *procGroup = new QGroupBox(tr("Processing"), this);
    QFormLayout *procForm = new QFormLayout(procGroup);
    m_threadSpin = new QSpinBox;
    int maxThreadCount = QThread::idealThreadCount();
    if (maxThreadCount <= 0)
        maxThreadCount = 1;
    m_threadSpin->setRange(1, maxThreadCount);
    m_threadSpin->setValue(Preferences::instance().threadCount());
    procForm->addRow(tr("Thread count:"), m_threadSpin);

    m_densitySlider = new QSlider(Qt::Horizontal);
    m_densitySlider->setRange(0, 100);
    m_densitySlider->setSingleStep(1);
    m_densitySlider->setPageStep(1);
    m_densityValueLabel = new QLabel;
    QHBoxLayout *densRow = new QHBoxLayout;
    densRow->addWidget(m_densitySlider, 1);
    densRow->addWidget(m_densityValueLabel);
    int densityInt = qRound(mainWindow->targetDensity() * 100.0);
    m_densitySlider->setValue(densityInt);
    m_densityValueLabel->setText(QString::number(m_densitySlider->value() / 100.0, 'f', 2));
    connect(m_densitySlider, &QSlider::valueChanged, this, [this](int v) {
        m_densityValueLabel->setText(QString::number(v / 100.0, 'f', 2));
        updateDirtyState();
    });
    procForm->addRow(tr("Density:"), densRow);

    m_edgeScalingCombo = new QComboBox;
    m_edgeScalingCombo->addItem(QStringLiteral("1.0"));
    m_edgeScalingCombo->addItem(QStringLiteral("2.0"));
    m_edgeScalingCombo->addItem(QStringLiteral("3.0"));
    m_edgeScalingCombo->addItem(QStringLiteral("4.0"));
    {
        int idx = qBound(0, (int)mainWindow->targetScaling() - 1, 3);
        m_edgeScalingCombo->setCurrentIndex(idx);
    }
    procForm->addRow(tr("Edge scaling:"), m_edgeScalingCombo);

    connect(m_threadSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::updateDirtyState);
    connect(m_edgeScalingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::updateDirtyState);

    root->addWidget(procGroup);

    QGroupBox *uiGroup = new QGroupBox(tr("Interface"), this);
    QVBoxLayout *uiLay = new QVBoxLayout(uiGroup);
    m_compareBarCheck = new QCheckBox(tr("Show before/after comparison bar"));
    m_compareBarCheck->setChecked(Preferences::instance().showCompareBar());
    uiLay->addWidget(m_compareBarCheck);
    connect(m_compareBarCheck, &QCheckBox::toggled, this, &SettingsDialog::updateDirtyState);
    root->addWidget(uiGroup);

    QGroupBox *colorGroup = new QGroupBox(tr("Colors"), this);
    QVBoxLayout *colorLayout = new QVBoxLayout(colorGroup);

    {
        QHBoxLayout *row = new QHBoxLayout;
        m_bgSwatch = new QLabel;
        m_bgSwatch->setFixedSize(20, 20);
        row->addWidget(m_bgSwatch);
        row->addWidget(new QLabel(tr("Background")));
        row->addStretch();
        QPushButton *pickBtn = new QPushButton(tr("Choose…"));
        QPushButton *resetBtn = new QPushButton(tr("Reset"));
        row->addWidget(pickBtn);
        row->addWidget(resetBtn);
        colorLayout->addLayout(row);
        connect(pickBtn, &QPushButton::clicked, this, &SettingsDialog::pickBackgroundColor);
        connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::resetBackgroundColor);
    }
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_modelSwatch = new QLabel;
        m_modelSwatch->setFixedSize(20, 20);
        row->addWidget(m_modelSwatch);
        row->addWidget(new QLabel(tr("Model")));
        row->addStretch();
        QPushButton *pickBtn = new QPushButton(tr("Choose…"));
        QPushButton *resetBtn = new QPushButton(tr("Reset"));
        row->addWidget(pickBtn);
        row->addWidget(resetBtn);
        colorLayout->addLayout(row);
        connect(pickBtn, &QPushButton::clicked, this, &SettingsDialog::pickModelColor);
        connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::resetModelColor);
    }

    root->addWidget(colorGroup);

    updateBackgroundSwatch();
    updateModelSwatch();

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    m_applyButton = buttons->button(QDialogButtonBox::Apply);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (m_mainWindow)
            m_mainWindow->applyPreferencesFromDialog(*this);
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    if (m_applyButton) {
        connect(m_applyButton, &QPushButton::clicked, this, [this]() {
            if (m_mainWindow)
                m_mainWindow->applyPreferencesFromDialog(*this);
        });
    }
    root->addWidget(buttons);

    captureBaselineFromWidgets();
    updateDirtyState();
}

int SettingsDialog::threadCountValue() const
{
    return m_threadSpin ? m_threadSpin->value() : 1;
}

float SettingsDialog::densityValue() const
{
    return m_densitySlider ? (m_densitySlider->value() / 100.0f) : 0.0f;
}

int SettingsDialog::edgeScalingIndex() const
{
    return m_edgeScalingCombo ? m_edgeScalingCombo->currentIndex() : 0;
}

bool SettingsDialog::compareBarEnabledValue() const
{
    return m_compareBarCheck && m_compareBarCheck->isChecked();
}

void SettingsDialog::applyPendingColorsTo(PbrShaderWidget *gl)
{
    if (!gl)
        return;
    gl->setBackgroundColor(m_pendingBackground);
    if (m_pendingModelDiffuseCustom)
        gl->setModelDiffuseColor(m_pendingModelDiffuse);
    else
        gl->clearModelDiffuseColor();
    gl->reRender();
}

void SettingsDialog::updateBackgroundSwatch()
{
    if (!m_bgSwatch)
        return;
    m_bgSwatch->setStyleSheet(vectorToStyleSheet(m_pendingBackground));
}

void SettingsDialog::updateModelSwatch()
{
    if (!m_modelSwatch)
        return;
    QVector3D c = m_pendingModelDiffuseCustom ? m_pendingModelDiffuse : QVector3D(0.85f, 0.85f, 0.85f);
    m_modelSwatch->setStyleSheet(vectorToStyleSheet(c));
}

void SettingsDialog::pickBackgroundColor()
{
    QColor initial = QColor::fromRgbF(m_pendingBackground.x(), m_pendingBackground.y(), m_pendingBackground.z());
    QColorDialog dlg(initial, this);
    dlg.setWindowTitle(tr("Background Color"));
    if (dlg.exec() == QDialog::Accepted) {
        QColor color = dlg.currentColor();
        m_pendingBackground = QVector3D(color.redF(), color.greenF(), color.blueF());
        updateBackgroundSwatch();
        updateDirtyState();
    }
}

void SettingsDialog::pickModelColor()
{
    QVector3D cur = m_pendingModelDiffuseCustom ? m_pendingModelDiffuse : QVector3D(0.85f, 0.85f, 0.85f);
    QColor initial = QColor::fromRgbF(cur.x(), cur.y(), cur.z());
    QColorDialog dlg(initial, this);
    dlg.setWindowTitle(tr("Model Color"));
    if (dlg.exec() == QDialog::Accepted) {
        QColor color = dlg.currentColor();
        m_pendingModelDiffuse = QVector3D(color.redF(), color.greenF(), color.blueF());
        m_pendingModelDiffuseCustom = true;
        updateModelSwatch();
        updateDirtyState();
    }
}

void SettingsDialog::resetBackgroundColor()
{
    m_pendingBackground = PbrShaderWidget::defaultBackgroundColor();
    updateBackgroundSwatch();
    updateDirtyState();
}

void SettingsDialog::resetModelColor()
{
    m_pendingModelDiffuseCustom = false;
    m_pendingModelDiffuse = QVector3D(0.85f, 0.85f, 0.85f);
    updateModelSwatch();
    updateDirtyState();
}

void SettingsDialog::captureBaselineFromWidgets()
{
    if (!m_threadSpin || !m_densitySlider || !m_edgeScalingCombo)
        return;
    m_baseThreadCount = m_threadSpin->value();
    m_baseDensity = m_densitySlider->value() / 100.0f;
    m_baseEdgeScalingIndex = m_edgeScalingCombo->currentIndex();
    m_baseBackground = m_pendingBackground;
    m_baseModelDiffuse = m_pendingModelDiffuse;
    m_baseModelDiffuseCustom = m_pendingModelDiffuseCustom;
    m_baseShowCompareBar = m_compareBarCheck && m_compareBarCheck->isChecked();
}

bool SettingsDialog::isDirty() const
{
    if (!m_threadSpin || !m_densitySlider || !m_edgeScalingCombo)
        return false;
    if (m_compareBarCheck && m_compareBarCheck->isChecked() != m_baseShowCompareBar)
        return true;
    if (m_threadSpin->value() != m_baseThreadCount)
        return true;
    const float d = m_densitySlider->value() / 100.0f;
    if (!qFuzzyCompare(d + 1.0f, m_baseDensity + 1.0f))
        return true;
    if (m_edgeScalingCombo->currentIndex() != m_baseEdgeScalingIndex)
        return true;
    if (!qFuzzyCompare(m_pendingBackground.x(), m_baseBackground.x())
        || !qFuzzyCompare(m_pendingBackground.y(), m_baseBackground.y())
        || !qFuzzyCompare(m_pendingBackground.z(), m_baseBackground.z()))
        return true;
    if (m_pendingModelDiffuseCustom != m_baseModelDiffuseCustom)
        return true;
    if (!qFuzzyCompare(m_pendingModelDiffuse.x(), m_baseModelDiffuse.x())
        || !qFuzzyCompare(m_pendingModelDiffuse.y(), m_baseModelDiffuse.y())
        || !qFuzzyCompare(m_pendingModelDiffuse.z(), m_baseModelDiffuse.z()))
        return true;
    return false;
}

void SettingsDialog::updateDirtyState()
{
    if (m_applyButton)
        m_applyButton->setEnabled(isDirty());
}

void SettingsDialog::syncBaselineFromApplied()
{
    captureBaselineFromWidgets();
    updateDirtyState();
}

void SettingsDialog::reject()
{
    if (!isDirty()) {
        QDialog::reject();
        return;
    }
    const QMessageBox::StandardButton ans = QMessageBox::question(this,
        tr("Preferences"),
        tr("You have unsaved changes. Discard them?"),
        QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (ans == QMessageBox::Discard)
        QDialog::reject();
}

void SettingsDialog::closeEvent(QCloseEvent *event)
{
    if (!isDirty()) {
        QDialog::closeEvent(event);
        return;
    }
    const QMessageBox::StandardButton ans = QMessageBox::question(this,
        tr("Preferences"),
        tr("You have unsaved changes. Discard them?"),
        QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (ans == QMessageBox::Discard)
        event->accept();
    else
        event->ignore();
}
