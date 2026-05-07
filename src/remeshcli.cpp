/*
 *  Copyright (c) 2020 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved.
 */
#include "remeshcli.h"

#include "objmeshio.h"
#include "quadmeshgenerator.h"
#include "version.h"

#include <geogram/basic/logger.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QtGlobal>

#include <cstdio>

#if defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
#include <unistd.h>
#endif

namespace {

void cliMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
        return;
    default:
        fprintf(stderr, "%s\n", qPrintable(msg));
        break;
    }
}

struct SilenceDebug {
    bool active;
    explicit SilenceDebug(bool verbose)
        : active(!verbose)
    {
        if (active)
            qInstallMessageHandler(cliMessageHandler);
    }
    ~SilenceDebug()
    {
        if (active)
            qInstallMessageHandler(nullptr);
    }
};

/** Redirects libc stderr (POSIX only; no-op elsewhere). */
struct StderrSilencer {
    FILE *m_nullFile = nullptr;
    int m_savedErr = -1;
    bool m_ok = false;

    explicit StderrSilencer(bool enable)
    {
        if (!enable)
            return;
#if defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
        m_nullFile = fopen("/dev/null", "wb");
        if (!m_nullFile)
            return;
        fflush(stderr);
        const int errFd = fileno(stderr);
        const int nullFd = fileno(m_nullFile);
        m_savedErr = dup(errFd);
        if (m_savedErr < 0) {
            fclose(m_nullFile);
            m_nullFile = nullptr;
            return;
        }
        if (dup2(nullFd, errFd) < 0) {
            close(m_savedErr);
            m_savedErr = -1;
            fclose(m_nullFile);
            m_nullFile = nullptr;
            return;
        }
        m_ok = true;
#endif
    }

    ~StderrSilencer()
    {
#if defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
        if (!m_ok)
            return;
        fflush(stderr);
        const int errFd = fileno(stderr);
        dup2(m_savedErr, errFd);
        close(m_savedErr);
        fclose(m_nullFile);
#endif
    }
};

} // namespace

int runRemeshCli(int argc, char **argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QCoreApplication *app = QCoreApplication::instance();
    if (!app)
        return 1;

    QCommandLineParser parser;
    const int ideal = QThread::idealThreadCount();
    const int maxThreadsUi = ideal > 0 ? ideal : 1;
    parser.setApplicationDescription(QStringLiteral(
        "Remesh a Wavefront OBJ to quads.\n\n"
        "Processing options match the GUI Preferences ranges:\n"
        "  --threads      1 … %1\n"
        "  --density      0.0 … 1.0\n"
        "  --edge-scale   1, 2, 3, or 4")
        .arg(maxThreadsUi));
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringLiteral("cli"),
        QStringLiteral("Batch remesh without GUI.")));
    parser.addOption(QCommandLineOption({QStringLiteral("o"), QStringLiteral("output")},
        QStringLiteral("Output OBJ path. Use - for stdout."), QStringLiteral("path")));
    parser.addOption(QCommandLineOption(QStringLiteral("threads"),
        QStringLiteral("Worker thread count. Defaults to an automatic selection."), QStringLiteral("n")));
    parser.addOption(QCommandLineOption(QStringLiteral("density"),
        QStringLiteral("Density (0.0–1.0). Default: 0.0."), QStringLiteral("v")));
    parser.addOption(QCommandLineOption(QStringLiteral("edge-scale"),
        QStringLiteral("Edge scaling (1–4). Default: 2."), QStringLiteral("s")));
    parser.addOption(QCommandLineOption(QStringLiteral("verbose"),
        QStringLiteral("Verbose Qt log output.")));
    parser.addPositionalArgument(QStringLiteral("input"), QStringLiteral("Input OBJ file."));

    parser.process(*app);

    const QStringList posEarly = parser.positionalArguments();
    if (posEarly.isEmpty() && !parser.isSet(QStringLiteral("output"))) {
        QString ht = parser.helpText();
        QStringList lines = ht.split(QLatin1Char('\n'));
        QStringList filtered;
        filtered.reserve(lines.size());
        for (const QString &line : lines) {
            if (!line.contains(QStringLiteral("help-all"), Qt::CaseInsensitive))
                filtered.append(line);
        }
        QTextStream out(stdout);
        out << filtered.join(QLatin1Char('\n'));
        return 0;
    }

    const bool verbose = parser.isSet(QStringLiteral("verbose"));
    SilenceDebug silence(verbose);

    int threads = maxThreadsUi;
    if (parser.isSet(QStringLiteral("threads"))) {
        bool ok = false;
        threads = parser.value(QStringLiteral("threads")).toInt(&ok);
        if (!ok || threads < 1 || threads > maxThreadsUi) {
            fprintf(stderr, "Error: --threads must be between 1 and %d.\n", maxThreadsUi);
            return 1;
        }
    }

    double density = 0.0;
    if (parser.isSet(QStringLiteral("density"))) {
        bool ok = false;
        density = parser.value(QStringLiteral("density")).toDouble(&ok);
        if (!ok || density < 0.0 || density > 1.0) {
            fputs("Error: --density must be between 0.0 and 1.0.\n", stderr);
            return 1;
        }
    }

    int edgeScale = 2;
    if (parser.isSet(QStringLiteral("edge-scale"))) {
        bool ok = false;
        edgeScale = parser.value(QStringLiteral("edge-scale")).toInt(&ok);
        if (!ok || edgeScale < 1 || edgeScale > 4) {
            fputs("Error: --edge-scale must be 1, 2, 3, or 4.\n", stderr);
            return 1;
        }
    }

    const QStringList pos = parser.positionalArguments();
    if (pos.size() != 1) {
        fputs("Error: exactly one input OBJ path is required.\n", stderr);
        return 1;
    }

    if (!parser.isSet(QStringLiteral("output"))) {
        fputs("Error: -o/--output is required (use - for stdout).\n", stderr);
        return 1;
    }

    const QString inputPath = pos.at(0);
    const QString outputPath = parser.value(QStringLiteral("output"));

    std::vector<AutoRemesher::Vector3> vertices;
    std::vector<std::vector<size_t>> triangles;
    QString loadErr;
    if (!ObjMeshIo::loadWavefrontObj(inputPath, vertices, triangles, &loadErr)) {
        if (!loadErr.isEmpty())
            fprintf(stderr, "%s\n", qPrintable(loadErr));
        else
            fputs("Error: failed to load input OBJ.\n", stderr);
        return 1;
    }

    QuadMeshGenerator generator(vertices, triangles);
    QuadMeshGenerator::Parameters parameters;
    const int base = 100000;
    const int range = 500000;
    parameters.targetTriangleCount = size_t(double(base) + double(range) * density);
    parameters.scaling = double(edgeScale);
    parameters.modelType = AutoRemesher::ModelType::Organic;
    parameters.threadCount = threads;
    generator.setParameters(parameters);

    const bool geoQuietBefore = GEO::Logger::instance()->is_quiet();
    if (!verbose)
        GEO::Logger::instance()->set_quiet(true);
    {
        StderrSilencer stderrQuiet(!verbose);
        generator.generate();
    }
    if (!verbose)
        GEO::Logger::instance()->set_quiet(geoQuietBefore);

    std::vector<AutoRemesher::Vector3> *remeshVerts = generator.takeRemeshedVertices();
    std::vector<std::vector<size_t>> *remeshQuads = generator.takeRemeshedQuads();

    if (!remeshVerts || !remeshQuads || remeshVerts->empty() || remeshQuads->empty()) {
        fputs("Error: remesh failed or produced an empty mesh.\n", stderr);
        delete remeshVerts;
        delete remeshQuads;
        return 1;
    }

    bool written = false;
    QString werr;
    if (outputPath == QStringLiteral("-")) {
        QFile out;
        if (!out.open(stdout, QFile::WriteOnly)) {
            fputs("Error: could not open stdout for writing.\n", stderr);
            delete remeshVerts;
            delete remeshQuads;
            return 1;
        }
        written = ObjMeshIo::writeQuadObj(out, *remeshVerts, *remeshQuads, &werr);
    } else {
        QFile out(outputPath);
        if (!out.open(QIODevice::WriteOnly)) {
            fputs("Error: could not open output file for writing.\n", stderr);
            delete remeshVerts;
            delete remeshQuads;
            return 1;
        }
        written = ObjMeshIo::writeQuadObj(out, *remeshVerts, *remeshQuads, &werr);
    }

    delete remeshVerts;
    delete remeshQuads;

    if (!written) {
        if (!werr.isEmpty())
            fprintf(stderr, "%s\n", qPrintable(werr));
        return 1;
    }

    if (verbose)
        fprintf(stderr, "Remesh finished successfully.\n");

    return 0;
}
