#include "howtousewidget.h"
#include "version.h"
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QOpenGLFunctions>

HowToUseWidget::HowToUseWidget(QWidget *parent)
    : QDialog(parent)
{
    QTextBrowser *browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(true);
    browser->setHtml(
        QStringLiteral("<h3>%1 %2</h3>"
        "<p>Version: %3 (build: %4 %5)<br>"
        "OpenGL: %6 | Shader: %7 | Core Profile: %8</p>"
        "<p>Source code: <a href=\"https://github.com/overscore-media/autoremesher2\">https://github.com/overscore-media/autoremesher2</a></p>"
        "<p>Issues: <a href=\"https://github.com/overscore-media/autoremesher2/issues\">https://github.com/overscore-media/autoremesher2/issues</a></p>"
        "<h4>Usage</h4>"
        "<p><b>Open a model</b> — Use <i>Open File</i> in the toolbar or drag and drop a Wavefront .obj file onto the window.</p>"
        "<p><b>Remesh</b> — After loading, remeshing runs automatically using your density and edge scaling from "
        "<i>Settings → Preferences</i>. There are other settings you may want to take a look at there, including the number of threads to use for remeshing and some presentational options.</p>"
        "<p><b>Stats</b> — Toggle the stats panel from the toolbar to compare original versus remeshed vertex and face counts.</p>"
        "<p><b>Before / After Slider</b> — When remeshing finishes (if enabled in Preferences), drag the vertical divider on the "
        "viewport to compare the original mesh (Before) with the remeshed result (After).</p>"
        "<p><b>Navigate</b> — Drag with the left mouse button to orbit. Hold Shift and drag to pan. Use the mouse wheel to zoom.</p>"
        "<p><b>Export</b> — Save the remeshed quad mesh from the toolbar when remeshing has completed.</p>")
        .arg(APP_NAME)
        .arg(APP_HUMAN_VER)
        .arg(APP_VER)
        .arg(__DATE__)
        .arg(__TIME__)
        .arg((char *)glGetString(GL_VERSION))
        .arg((char *)glGetString(GL_SHADING_LANGUAGE_VERSION))
        .arg(QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile ? "true" : "false"));

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(browser);

    setWindowTitle(tr("Instructions"));
    resize(560, 420);
}
