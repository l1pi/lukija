Name:       harbour-lukija
Summary:    Reads PDF files aloud
Version:    0.1.0
Release:    1
License:    GPL-3.0-or-later
URL:        https://github.com/l1pi/lukija
Source0:    %{name}-%{version}.tar.bz2

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
BuildRequires:  pkgconfig(poppler-qt5)
# For %%check: the tests run on Qt 5.6, catching use of newer Qt APIs.
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  qt5-plugin-platform-minimal
BuildRequires:  sailfish-fonts

%description
Lukija reads PDF files aloud with an offline neural voice (Piper). It is made
for ebooks and academic articles in Finnish and English.

%package cli
Summary:    Command-line tool for Lukija's text pipeline and voices
Requires:   %{name} = %{version}-%{release}

%description cli
lukija-cli prints the text Lukija would read from a PDF, shows its layout
analysis, and reads PDFs aloud or into a WAV file. Meant for testing.

%prep
%setup -q -n %{name}-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release -DLUKIJA_BUILD_APP=ON -DLUKIJA_BUILD_TESTS=ON \
       -DLUKIJA_TEST_QPA_PLATFORM=minimal:enable_fonts
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
