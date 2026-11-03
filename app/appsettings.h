#pragma once

#include <QSettings>
#include <QStandardPaths>

namespace lukija {

// The user's preferences, in the app's own config directory. A default
// QSettings would use ~/.config/<org>/<app>.conf, next to that directory:
// Sailjail only keeps ~/.config/<org>/<app>/, so the file wouldn't persist.
class AppSettings : public QSettings
{
public:
    AppSettings()
        : QSettings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                        + QStringLiteral("/settings.conf"),
                    QSettings::IniFormat)
    {
    }
};

} // namespace lukija
