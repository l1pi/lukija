#include "appinfo.h"
#include "audioroutemonitor.h"
#include "library.h"
#include "librarymodel.h"
#include "reader.h"
#include "voicelibrary.h"

#include <sailfishapp.h>

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QScopedPointer>
#include <QtQml>

using namespace lukija;

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setOrganizationName(QString::fromLatin1(OrganizationName));
    app->setApplicationName(QString::fromLatin1(ApplicationName));
    app->setApplicationVersion(QStringLiteral(LUKIJA_VERSION));

    qmlRegisterUncreatableType<Reader>("Lukija", 1, 0, "Reader", QStringLiteral("Use the reader context property"));
    qmlRegisterUncreatableType<PlaybackController>("Lukija", 1, 0, "PlaybackController",
                                                   QStringLiteral("Use reader.player"));

    Library library;
    LibraryModel libraryModel(&library);
    Reader reader(&library);
    QObject::connect(&reader, &Reader::opened, &libraryModel, &LibraryModel::refresh);
    QObject::connect(app.data(), &QCoreApplication::aboutToQuit, &reader, &Reader::savePosition);

    // Stop when headphones or Bluetooth audio go away, rather than carrying on
    // through the phone's speaker.
    AudioRouteMonitor audioRoutes;
    QObject::connect(&audioRoutes, &AudioRouteMonitor::accessoryLost, reader.player(),
                     &PlaybackController::pause);
    audioRoutes.start();

    // A PDF passed on the command line, e.g. through "Open with" in another app.
    QString initialFile;
    const QStringList arguments = app->arguments();
    for (int i = 1; i < arguments.size(); ++i) {
        if (!arguments.at(i).startsWith(QLatin1Char('-')))
            initialFile = arguments.at(i);
    }

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    QQmlContext *context = view->rootContext();
    context->setContextProperty(QStringLiteral("appVersion"), app->applicationVersion());
    context->setContextProperty(QStringLiteral("library"), &libraryModel);
    context->setContextProperty(QStringLiteral("reader"), &reader);
    context->setContextProperty(QStringLiteral("initialFile"), initialFile);
    context->setContextProperty(QStringLiteral("voicesDirectory"), VoiceLibrary::voicesDirectory());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    return app->exec();
}
