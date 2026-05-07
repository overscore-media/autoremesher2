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
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QGuiApplication>
#include <cmath>
#include <QVector4D>
#include <QSurfaceFormat>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <cstring>
#include "pbrshaderwidget.h"

bool PbrShaderWidget::m_transparent = false;
float PbrShaderWidget::m_minZoomRatio = 5.0;
float PbrShaderWidget::m_maxZoomRatio = 80.0;

int PbrShaderWidget::m_defaultXRotation = 30 * 16;
int PbrShaderWidget::m_defaultYRotation = -45 * 16;
int PbrShaderWidget::m_defaultZRotation = 0;
QVector3D PbrShaderWidget::m_defaultEyePosition = QVector3D(0, 0, -4.0);

PbrShaderWidget::PbrShaderWidget(QWidget *parent) :
    QOpenGLWidget(parent)
{
    if (m_transparent) {
        setAttribute(Qt::WA_AlwaysStackOnTop);
        setAttribute(Qt::WA_TranslucentBackground);
        QSurfaceFormat fmt = format();
        fmt.setAlphaBufferSize(8);
        fmt.setSamples(8);
        setFormat(fmt);
    } else {
        QSurfaceFormat fmt = format();
        fmt.setSamples(8);
        setFormat(fmt);
    }
    setContextMenuPolicy(Qt::CustomContextMenu);

    const qreal dpr = window() ? window()->devicePixelRatio() : devicePixelRatio();
    m_widthInPixels = qRound(width() * dpr);
    m_heightInPixels = qRound(height() * dpr);

    m_backgroundColor = defaultBackgroundColor();

    zoom(200);
}

const QVector3D &PbrShaderWidget::eyePosition()
{
	return m_eyePosition;
}

const QVector3D &PbrShaderWidget::moveToPosition()
{
    return m_moveToPosition;
}

void PbrShaderWidget::setEyePosition(const QVector3D &eyePosition)
{
    m_eyePosition = eyePosition;
    emit eyePositionChanged(m_eyePosition);
    update();
}

void PbrShaderWidget::reRender()
{
    emit renderParametersChanged();
    update();
}

int PbrShaderWidget::xRotation()
{
    return m_xRotation;
}

int PbrShaderWidget::yRotation()
{
    return m_yRotation;
}

int PbrShaderWidget::zRotation()
{
    return m_zRotation;
}

PbrShaderWidget::~PbrShaderWidget()
{
    cleanup();
}

void PbrShaderWidget::normalizeAngle(int &angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void PbrShaderWidget::setXRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_xRotation) {
        m_xRotation = angle;
        emit xRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void PbrShaderWidget::setYRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_yRotation) {
        m_yRotation = angle;
        emit yRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void PbrShaderWidget::setZRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_zRotation) {
        m_zRotation = angle;
        emit zRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void PbrShaderWidget::cleanup()
{
    if (m_program == nullptr)
        return;
    makeCurrent();
    m_meshBinder.cleanup();
    m_compareMeshBinder.cleanup();
    delete m_program;
    m_program = nullptr;
    delete m_lineProgram;
    m_lineProgram = nullptr;
    if (m_lineOverlayVao.isCreated())
        m_lineOverlayVao.destroy();
    if (m_lineOverlayBuffer.isCreated())
        m_lineOverlayBuffer.destroy();
    m_lineOverlayVaoConfigured = false;
    doneCurrent();
}

void PbrShaderWidget::initializeGL()
{
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &PbrShaderWidget::cleanup);

    initializeOpenGLFunctions();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	if (m_enableCullFace)
		glEnable(GL_CULL_FACE);
#ifdef GL_LINE_SMOOTH
	glEnable(GL_LINE_SMOOTH);
#endif
    if (m_transparent) {
        glClearColor(0, 0, 0, 0);
    } else {
        if (m_backgroundColor.isNull()) {
            QColor bgcolor = QWidget::palette().color(QWidget::backgroundRole());
            glClearColor(bgcolor.redF(), bgcolor.greenF(), bgcolor.blueF(), 1);
        } else {
            glClearColor(m_backgroundColor.x(), m_backgroundColor.y(), m_backgroundColor.z(), 1);
        }
    }
    
    bool isCoreProfile = false;
    const char *versionString = (const char *)glGetString(GL_VERSION);
    if (nullptr != versionString &&
            '\0' != versionString[0] &&
            0 == strstr(versionString, "Mesa")) {
        isCoreProfile = format().profile() == QSurfaceFormat::CoreProfile;
    }
    qDebug() << "isCoreProfile:" << isCoreProfile << "versionString:" << versionString;
    m_isCoreProfile = isCoreProfile;

    m_program = new PbrShaderProgram(isCoreProfile);

    m_meshBinder.initialize();
    m_compareMeshBinder.initialize();

    m_lineProgram = new QOpenGLShaderProgram;
    bool lineOk = false;
    if (m_isCoreProfile) {
        const char *vsCore = R"(#version 150 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
)";
        const char *fsCore = R"(#version 150 core
uniform vec3 uColor;
out vec4 fragColor;
void main() { fragColor = vec4(uColor, 1.0); }
)";
        m_lineProgram->bindAttributeLocation("aPos", 0);
        lineOk = m_lineProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vsCore) &&
            m_lineProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fsCore) &&
            m_lineProgram->link();
    } else {
        const char *vs = R"(#version 110
attribute vec3 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
)";
        const char *fs = R"(#version 110
uniform vec3 uColor;
void main() { gl_FragColor = vec4(uColor, 1.0); }
)";
        lineOk = m_lineProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vs) &&
            m_lineProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fs) &&
            m_lineProgram->link();
    }
    if (!lineOk) {
        qWarning() << "Line overlay shader failed:" << (m_lineProgram ? m_lineProgram->log() : QString());
        delete m_lineProgram;
        m_lineProgram = nullptr;
    } else {
        m_lineOverlayVao.create();
        m_lineOverlayBuffer.create();
    }

    m_program->release();
}

void PbrShaderWidget::disableCullFace()
{
    m_enableCullFace = false;
}

void PbrShaderWidget::setMoveToPosition(const QVector3D &moveToPosition)
{
    m_moveToPosition = moveToPosition;
}

void PbrShaderWidget::setBackgroundColor(const QVector3D &color)
{
    m_backgroundColor = color;
    QOpenGLContext *ctx = context();
    if (ctx != nullptr && ctx->isValid()) {
        makeCurrent();
        glClearColor(m_backgroundColor.x(), m_backgroundColor.y(), m_backgroundColor.z(), 1.0f);
        doneCurrent();
    }
    update();
}

void PbrShaderWidget::setModelDiffuseColor(const QVector3D &color)
{
    m_modelDiffuseColor = color;
    m_modelDiffuseColorEnabled = true;
    m_meshBinder.showWireframe();
    update();
}

void PbrShaderWidget::clearModelDiffuseColor()
{
    m_modelDiffuseColorEnabled = false;
    update();
}

bool PbrShaderWidget::isModelDiffuseColorEnabled() const
{
    return m_modelDiffuseColorEnabled;
}

QVector3D PbrShaderWidget::defaultBackgroundColor()
{
    static const float s = 1.0f / 255.0f;
    return QVector3D(0x26 * s, 0x26 * s, 0x26 * s);
}

const QVector3D &PbrShaderWidget::backgroundColor() const
{
    return m_backgroundColor;
}

const QVector3D &PbrShaderWidget::modelDiffuseColor() const
{
    return m_modelDiffuseColor;
}

void PbrShaderWidget::setCompareMesh(PbrShaderMesh *mesh)
{
    m_compareMeshBinder.updateMesh(mesh);
    m_hasCompareMesh = mesh != nullptr;
    if (mesh)
        m_compareMeshBinder.showWireframe();
    update();
}

void PbrShaderWidget::clearCompareMesh()
{
    m_compareMeshBinder.hideWireframe();
    m_compareMeshBinder.updateMesh(nullptr);
    m_hasCompareMesh = false;
    update();
}

void PbrShaderWidget::setCompareModeEnabled(bool enabled)
{
    m_compareModeEnabled = enabled;
    update();
}

bool PbrShaderWidget::compareModeEnabled() const
{
    return m_compareModeEnabled;
}

void PbrShaderWidget::setCompareSplit(float t)
{
    m_compareSplit = qBound(0.0f, t, 1.0f);
    update();
}

float PbrShaderWidget::compareSplit() const
{
    return m_compareSplit;
}

bool PbrShaderWidget::compareSplitReady() const
{
    return m_hasCompareMesh && m_meshBinder.hasTriangleGeometry();
}

void PbrShaderWidget::recenterOnModel()
{
    QVector3D center = m_meshBinder.modelCentroid();
    if (!center.isNull()) {
        m_moveToPosition = center;
        updateProjectionMatrix();
        emit moveToPositionChanged(m_moveToPosition);
        emit renderParametersChanged();
        update();
    }
}

QMatrix4x4 PbrShaderWidget::buildWorldMatrix() const
{
    QMatrix4x4 world;
    world.setToIdentity();

    world.rotate(m_xRotation / 16.0f, 1, 0, 0);
    world.rotate(m_yRotation / 16.0f, 0, 1, 0);
    world.rotate(m_zRotation / 16.0f, 0, 0, 1);
    return world;
}

void PbrShaderWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_world = buildWorldMatrix();

    m_camera.setToIdentity();
    m_camera.translate(m_eyePosition.x(), m_eyePosition.y(), m_eyePosition.z());

    m_program->bind();
    m_program->setEyePosValue(m_eyePosition);
    m_program->setProjectionMatrixValue(m_projection);
    m_program->setModelMatrixValue(m_world);
    QMatrix3x3 normalMatrix = m_world.normalMatrix();
    m_program->setNormalMatrixValue(normalMatrix);
    m_program->setViewMatrixValue(m_camera);
    m_program->setTextureEnabledValue(0);
    m_program->setNormalMapEnabledValue(0);

    if (m_modelDiffuseColorEnabled) {
        m_program->setModelDiffuseColorValue(m_modelDiffuseColor);
        m_program->setModelDiffuseColorEnabledValue(1);
    } else {
        m_program->setModelDiffuseColorEnabledValue(0);
    }

    if (m_mousePickingEnabled && !m_mousePickTargetPositionInModelSpace.isNull()) {
        m_program->setMousePickEnabledValue(1);
        m_program->setMousePickTargetPositionValue(m_world * m_mousePickTargetPositionInModelSpace);
    } else {
        m_program->setMousePickEnabledValue(0);
        m_program->setMousePickTargetPositionValue(QVector3D());
    }
    m_program->setMousePickRadiusValue(m_mousePickRadius);
    m_program->setMeshOpacityValue(1.0f);

    glViewport(0, 0, m_widthInPixels, m_heightInPixels);

    const bool compareActive = m_compareModeEnabled && m_hasCompareMesh;
    const float t = m_compareSplit;
    int dividerSplitPx = -1;

    if (!compareActive) {
        m_meshBinder.paint(m_program);
    } else {
        const int splitPx = qBound(0, qRound(t * float(m_widthInPixels)), m_widthInPixels);
        if (splitPx <= 0) {
            m_meshBinder.paint(m_program);
        } else if (splitPx >= m_widthInPixels) {
            glDisable(GL_BLEND);
            m_compareMeshBinder.paint(m_program);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            dividerSplitPx = splitPx;
            glDisable(GL_BLEND);
            glEnable(GL_SCISSOR_TEST);
            glScissor(0, 0, splitPx, m_heightInPixels);
            m_compareMeshBinder.paint(m_program);
            glScissor(splitPx, 0, m_widthInPixels - splitPx, m_heightInPixels);
            glClear(GL_DEPTH_BUFFER_BIT);
            m_meshBinder.paint(m_program);
            glDisable(GL_SCISSOR_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }

    m_program->release();

    glViewport(0, 0, m_widthInPixels, m_heightInPixels);

    if (dividerSplitPx >= 0 && m_lineProgram)
        drawCompareSplitDivider(dividerSplitPx);
}

void PbrShaderWidget::drawCompareSplitDivider(int splitPx)
{
    if (!m_lineProgram || splitPx <= 0 || splitPx >= m_widthInPixels)
        return;
    GLboolean depthWas = GL_TRUE;
    glGetBooleanv(GL_DEPTH_TEST, &depthWas);
    glDisable(GL_DEPTH_TEST);

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    const float x = float(splitPx) - 0.5f;
    const float h = float(m_heightInPixels);
    float verts[] = {
        x, 0.5f, 0.f,
        x, h - 0.5f, 0.f
    };

    m_lineOverlayBuffer.bind();
    m_lineOverlayBuffer.allocate(verts, sizeof(verts));

    if (!m_lineOverlayVaoConfigured) {
        QOpenGLVertexArrayObject::Binder vaob(&m_lineOverlayVao);
        const int posLoc = m_lineProgram->attributeLocation("aPos");
        if (posLoc < 0) {
            m_lineOverlayBuffer.release();
            if (depthWas)
                glEnable(GL_DEPTH_TEST);
            return;
        }
        f->glEnableVertexAttribArray(GLuint(posLoc));
        f->glVertexAttribPointer(GLuint(posLoc), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        m_lineOverlayVaoConfigured = true;
    }

    QMatrix4x4 pixelMvp;
    pixelMvp.ortho(0.0f, float(m_widthInPixels), 0.0f, float(m_heightInPixels), -1.0f, 1.0f);

    m_lineProgram->bind();
    m_lineProgram->setUniformValue("uMVP", pixelMvp);
    m_lineProgram->setUniformValue("uColor", QVector3D(0.95f, 0.95f, 0.96f));
    {
        QOpenGLVertexArrayObject::Binder vaoBinder(&m_lineOverlayVao);
        m_lineOverlayBuffer.bind();
        glDrawArrays(GL_LINES, 0, 2);
    }
    m_lineOverlayBuffer.release();
    m_lineProgram->release();

    if (depthWas)
        glEnable(GL_DEPTH_TEST);
}

void PbrShaderWidget::updateProjectionMatrix()
{
    m_projection.setToIdentity();
    m_projection.translate(m_moveToPosition.x(), m_moveToPosition.y(), m_moveToPosition.z());
    m_projection.perspective(45.0f, GLfloat(width()) / height(), 0.01f, 100.0f);
}

void PbrShaderWidget::resizeGL(int w, int h)
{
	m_widthInPixels = w * window()->devicePixelRatio();
	m_heightInPixels = h * window()->devicePixelRatio();
    updateProjectionMatrix();
    emit renderParametersChanged();
}

std::pair<QVector3D, QVector3D> PbrShaderWidget::mousePositionToMouseRay(const QPoint &mousePosition)
{
    auto modelView = m_camera * m_world;
    float x = qMax(qMin(mousePosition.x(), width() - 1), 0);
    float y = qMax(qMin(mousePosition.y(), height() - 1), 0);
    QVector3D nearScreen = QVector3D(x, height() - y, 0.0);
    QVector3D farScreen = QVector3D(x, height() - y, 1.0);
    auto viewPort = QRect(0, 0, width(), height());
    auto nearPosition = nearScreen.unproject(modelView, m_projection, viewPort);
    auto farPosition = farScreen.unproject(modelView, m_projection, viewPort);
    return std::make_pair(nearPosition, farPosition);
}

void PbrShaderWidget::toggleWireframe()
{
    if (m_meshBinder.isWireframeVisible())
        m_meshBinder.hideWireframe();
    else
        m_meshBinder.showWireframe();
    update();
}

bool PbrShaderWidget::isWireframeVisible()
{
    return m_meshBinder.isWireframeVisible();
}

void PbrShaderWidget::enableEnvironmentLight()
{
    m_meshBinder.enableEnvironmentLight();
    update();
}

bool PbrShaderWidget::isEnvironmentLightEnabled()
{
    return m_meshBinder.isEnvironmentLightEnabled();
}

void PbrShaderWidget::toggleRotation()
{
    if (nullptr != m_rotationTimer) {
        delete m_rotationTimer;
        m_rotationTimer = nullptr;
    } else {
        m_rotationTimer = new QTimer(this);
        m_rotationTimer->setInterval(42);
        m_rotationTimer->setSingleShot(false);
        connect(m_rotationTimer, &QTimer::timeout, this, [&]() {
            setYRotation(m_yRotation - 8);
        });
        m_rotationTimer->start();
    }
}

bool PbrShaderWidget::inputMousePressEventFromOtherWidget(QMouseEvent *event)
{
    bool shouldStartMove = false;
    bool shouldStartPan = false;
    bool shiftPressed = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier);
    
    if (event->button() == Qt::LeftButton) {
        if (shiftPressed && m_panEnabled) {
            shouldStartPan = true;
        } else if (m_moveEnabled) {
            shouldStartMove = true;
        }
        if (!shouldStartMove && !shouldStartPan)
            emit mousePressed(event->globalPos());
    } else if (event->button() == Qt::MiddleButton) {
        if (shiftPressed && m_panEnabled) {
            shouldStartPan = true;
        }
    }
    if (shouldStartPan || shouldStartMove) {
        m_lastPos = convertInputPosFromOtherWidget(event);
        if (!m_moveStarted) {
            m_moveStartPos = mapToParent(convertInputPosFromOtherWidget(event));
            m_moveStartGeometry = geometry();
            m_moveStarted = true;
        }
        return true;
    }
    return false;
}

bool PbrShaderWidget::inputMouseReleaseEventFromOtherWidget(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (m_moveStarted) {
        m_moveStarted = false;
        return true;
    }
    if (event->button() == Qt::LeftButton) {
        if (m_mousePickingEnabled)
            emit mouseReleased(event->globalPos());
    }
    return false;
}

void PbrShaderWidget::canvasResized()
{
    resize(parentWidget()->size());
}

void PbrShaderWidget::resetToDefaultView()
{
    setXRotation(m_defaultXRotation);
    setYRotation(m_defaultYRotation);
    setZRotation(m_defaultZRotation);
    setEyePosition(m_defaultEyePosition);
    m_moveToPosition = QVector3D(0, 0, 0);
    updateProjectionMatrix();
    emit renderParametersChanged();
    update();
}

bool PbrShaderWidget::inputMouseMoveEventFromOtherWidget(QMouseEvent *event)
{
    QPoint pos = convertInputPosFromOtherWidget(event);
    
    if (m_mousePickingEnabled) {
        auto segment = mousePositionToMouseRay(pos);
        emit mouseRayChanged(segment.first, segment.second);
    }

    if (!m_moveStarted) {
        return false;
    }
    
    int dx = pos.x() - m_lastPos.x();
    int dy = pos.y() - m_lastPos.y();

    bool shiftPressed = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier);

    const bool panning = m_moveStarted && shiftPressed && m_panEnabled
        && (event->buttons() & (Qt::LeftButton | Qt::MiddleButton));
    const bool orbiting = m_moveStarted && m_moveEnabled && !shiftPressed
        && (event->buttons() & Qt::LeftButton);

    if (panning || orbiting) {
        if (panning) {
            if (m_moveAndZoomByWindow) {
                QPoint posInParent = mapToParent(pos);
                QRect rect = m_moveStartGeometry;
                rect.translate(posInParent.x() - m_moveStartPos.x(), posInParent.y() - m_moveStartPos.y());
                setGeometry(rect);
            } else if (m_panEnabled) {
                m_moveToPosition.setX(m_moveToPosition.x() + (float)2 * dx / width());
                m_moveToPosition.setY(m_moveToPosition.y() + (float)2 * -dy / height());
                if (m_moveToPosition.x() < -1.5)
                    m_moveToPosition.setX(-1.5);
                if (m_moveToPosition.x() > 1.5)
                    m_moveToPosition.setX(1.5);
                if (m_moveToPosition.y() < -1.5)
                    m_moveToPosition.setY(-1.5);
                if (m_moveToPosition.y() > 1.5)
                    m_moveToPosition.setY(1.5);
                updateProjectionMatrix();
                emit moveToPositionChanged(m_moveToPosition);
                emit renderParametersChanged();
                update();
            }
        } else if (orbiting) {
            setXRotation(m_xRotation + 8 * dy);
            setYRotation(m_yRotation + 8 * dx);
        }
    }
    m_lastPos = pos;
    
    return true;
}

QPoint PbrShaderWidget::convertInputPosFromOtherWidget(QMouseEvent *event)
{
    return mapFromGlobal(event->globalPos());
}

bool PbrShaderWidget::inputWheelEventFromOtherWidget(QWheelEvent *event)
{
    if (m_moveStarted)
        return true;
    
    if (m_mousePickingEnabled) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
			if (event->delta() > 0)
				emit addMouseRadius(0.001f);
			else if (event->delta() < 0)
				emit addMouseRadius(-0.001f);
            return true;
        }
    }
    
    if (!m_zoomEnabled)
        return false;

    qreal delta = geometry().height() * 0.1f;
    if (event->delta() < 0)
        delta = -delta;
    zoom(delta);
    
    return true;
}

void PbrShaderWidget::zoom(float delta)
{
    if (m_moveAndZoomByWindow) {
        QMargins margins(delta, delta, delta, delta);
        if (0 == m_modelInitialHeight) {
            m_modelInitialHeight = height();
        } else {
            float ratio = (float)height() / m_modelInitialHeight;
            if (ratio <= m_minZoomRatio) {
                if (delta < 0)
                    return;
            } else if (ratio >= m_maxZoomRatio) {
                if (delta > 0)
                    return;
            }
        }
        setGeometry(geometry().marginsAdded(margins));
        emit renderParametersChanged();
        update();
        return;
    } else {
        m_eyePosition += QVector3D(0, 0, m_eyePosition.z() * (delta > 0 ? -0.1 : 0.1));
        if (m_eyePosition.z() < -15)
            m_eyePosition.setZ(-15);
        else if (m_eyePosition.z() > -0.1)
            m_eyePosition.setZ(-0.1f);
        emit eyePositionChanged(m_eyePosition);
        emit renderParametersChanged();
        update();
    }
}

void PbrShaderWidget::setMousePickTargetPositionInModelSpace(QVector3D position)
{
    m_mousePickTargetPositionInModelSpace = position;
    update();
}

void PbrShaderWidget::setMousePickRadius(float radius)
{
    m_mousePickRadius = radius;
    update();
}

void PbrShaderWidget::updateMesh(PbrShaderMesh *mesh)
{
    m_meshBinder.updateMesh(mesh);
    emit renderParametersChanged();
    update();
}

void PbrShaderWidget::fetchCurrentToonNormalAndDepthMaps(QImage *normalMap, QImage *depthMap)
{
    m_meshBinder.fetchCurrentToonNormalAndDepthMaps(normalMap, depthMap);
}

void PbrShaderWidget::updateToonNormalAndDepthMaps(QImage *normalMap, QImage *depthMap)
{
    m_meshBinder.updateToonNormalAndDepthMaps(normalMap, depthMap);
    update();
}

int PbrShaderWidget::widthInPixels()
{
	return m_widthInPixels;
}

int PbrShaderWidget::heightInPixels()
{
	return m_heightInPixels;
}

void PbrShaderWidget::enableMove(bool enabled)
{
    m_moveEnabled = enabled;
}

void PbrShaderWidget::enableZoom(bool enabled)
{
    m_zoomEnabled = enabled;
}

void PbrShaderWidget::enablePan(bool enabled)
{
    m_panEnabled = enabled;
}

void PbrShaderWidget::enableMousePicking(bool enabled)
{
    m_mousePickingEnabled = enabled;
}

void PbrShaderWidget::setMoveAndZoomByWindow(bool byWindow)
{
    m_moveAndZoomByWindow = byWindow;
}

void PbrShaderWidget::mousePressEvent(QMouseEvent *event)
{
    inputMousePressEventFromOtherWidget(event);
}

void PbrShaderWidget::mouseMoveEvent(QMouseEvent *event)
{
    inputMouseMoveEventFromOtherWidget(event);
}

void PbrShaderWidget::wheelEvent(QWheelEvent *event)
{
    inputWheelEventFromOtherWidget(event);
}

void PbrShaderWidget::mouseReleaseEvent(QMouseEvent *event)
{
    inputMouseReleaseEventFromOtherWidget(event);
}

