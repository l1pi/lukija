Name:       harbour-lukija
Summary:    Reads PDF files aloud
Version:    0.1.0
Release:    6
License:    GPL-3.0-or-later
URL:        https://github.com/l1pi/lukija
Source0:    %{name}-%{version}.tar.bz2
# Third-party sources, pinned by checksum in 3rdparty/CMakeLists.txt. The
# build downloads them unless they're here, for builds without network
# access (rpm/fetch-sources.sh).
Source1:    https://github.com/OHF-Voice/piper1-gpl/archive/refs/tags/v1.8.0.tar.gz#/piper1-gpl-1.8.0.tar.gz
Source2:    https://github.com/espeak-ng/espeak-ng/archive/212928b394a96e8fd2096616bfd54e17845c48f6.tar.gz#/espeak-ng-212928b394a9.tar.gz
Source3:    https://github.com/waywardgeek/sonic/archive/fbf75c3d6d846bad3bb3d456cbc5d07d9fd8c104.tar.gz#/sonic-fbf75c3d6d84.tar.gz
Source4:    https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-aarch64-1.22.0.tgz
# The bundled onnxruntime is its official prebuilt aarch64 release.
ExclusiveArch: aarch64

# libpiper and onnxruntime are bundled privately under %%{_datadir}/%%{name}/lib.
%global __provides_exclude_from ^%{_datadir}/%{name}/lib/.*$
%global __requires_exclude ^lib(piper|onnxruntime)\\.so.*$

Requires:   sailfishsilica-qt5 >= 0.10.9
# QML modules and plugins loaded at run time.
Requires:   sailfish-components-pickers-qt5
Requires:   amber-qml-plugin-mpris
Requires:   libkeepalive
Requires:   nemo-qml-plugin-policy-qt5
Requires:   qt5-plugin-sqldriver-sqlite
BuildRequires:  cmake >= 3.26
BuildRequires:  desktop-file-utils
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(poppler-qt5)
# For %%check: the tests run on Qt 5.6, catching use of newer Qt APIs.
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  qt5-plugin-platform-minimal
BuildRequires:  sailfish-fonts

%description
Lukija reads PDF files aloud with offline neural voices (Piper). It is made
for ebooks and academic articles in Finnish and English. It remembers where
you stopped in each document, and keeps reading with the screen off, with
controls on the lock screen and on headsets.

Voices are downloaded in the app, about 60 MB each.

%package cli
Summary:    Command-line tool for Lukija's text pipeline and voices
Requires:   %{name} = %{version}-%{release}

%description cli
lukija-cli prints the text Lukija would read from a PDF, shows its layout
analysis, and reads PDFs aloud or into a WAV file. Meant for testing.

%prep
%setup -q -n %{name}-%{version}

%build
if [ -f %{SOURCE1} ]; then
    sources="-DLUKIJA_PIPER_URL=%{SOURCE1} -DLUKIJA_ESPEAK_URL=%{SOURCE2}
             -DLUKIJA_SONIC_URL=%{SOURCE3} -DLUKIJA_ONNXRUNTIME_URL=%{SOURCE4}"
fi
%cmake -DCMAKE_BUILD_TYPE=Release -DLUKIJA_BUILD_APP=ON -DLUKIJA_BUILD_TESTS=ON \
       -DLUKIJA_TEST_QPA_PLATFORM=minimal:enable_fonts $sources
%make_build

%check
ctest --output-on-failure

%install
%make_install

# rpmbuild strips only executables; libpiper was built with -g.
strip --strip-debug %{buildroot}%{_datadir}/%{name}/lib/libpiper.so

desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/*.desktop

%files
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png

%files cli
%{_bindir}/lukija-cli
