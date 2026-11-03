#include "audioroutemonitor.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>

namespace lukija {

namespace {

const QString Service = QStringLiteral("org.nemomobile.Route.Manager");
const QString Path = QStringLiteral("/org/nemomobile/Route/Manager");
const QString Interface = QStringLiteral("org.nemomobile.Route.Manager");

} // namespace

AudioRouteMonitor::AudioRouteMonitor(QObject *parent)
    : QObject(parent)
{
}

void AudioRouteMonitor::start()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected())
        return;

    bus.connect(Service, Path, Interface, QStringLiteral("AudioRouteChanged"), this,
                SLOT(onRouteChanged(QString,uint)));

    // The route right now. The reply starts with the output device and its
    // type, the same as the signal; the input device and its type follow.
    bus.callWithCallback(QDBusMessage::createMethodCall(Service, Path, Interface,
                                                        QStringLiteral("ActiveRoutes")),
                         this, SLOT(onRouteChanged(QString,uint)));
}

void AudioRouteMonitor::onRouteChanged(const QString &device, uint type)
{
    if (!(type & Output))
        return;

    qDebug() << "Audio output route:" << device << type;

    // A combined route such as "speakerandheadset" still counts as the
    // accessory: it's brief, e.g. for a ringtone, and the accessory is still there.
    const bool onAccessory = type & (Wired | Wireless);
    const bool lost = m_onAccessory && !onAccessory;
    m_onAccessory = onAccessory;
    if (lost)
        emit accessoryLost();
}

} // namespace lukija
