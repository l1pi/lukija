#pragma once

#include <QObject>
#include <QString>

namespace lukija {

// Follows where the system sends audio, as reported by its route manager.
//
// When headphones or a Bluetooth device such as a car's audio go away, the
// system moves the stream to the phone's speaker without telling the stream.
// This is how the app finds out.
class AudioRouteMonitor : public QObject
{
    Q_OBJECT

public:
    // Route type bits, from ohm-ext/route.h in ohm-plugins-misc.
    enum RouteType : uint {
        Output = 1u << 0,
        Wired = 1u << 3,
        Wireless = 1u << 4,
    };

    explicit AudioRouteMonitor(QObject *parent = nullptr);

    // Connects to the route manager on the system bus, which the Sailjail Audio
    // permission allows. Where there is no route manager, nothing ever changes.
    void start();

signals:
    // Audio was going to an accessory and now goes to the phone itself.
    void accessoryLost();

public slots:
    void onRouteChanged(const QString &device, uint type);

private:
    bool m_onAccessory = false;
};

} // namespace lukija
