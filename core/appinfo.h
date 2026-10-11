#pragma once

namespace lukija {

// Must match OrganizationName / ApplicationName in the Sailjail section of the
// .desktop file: they decide where the app's data lives (~/.local/share/<org>/<app>).
constexpr const char *OrganizationName = "io.github.l1pi";
constexpr const char *ApplicationName = "harbour-lukija";

} // namespace lukija
