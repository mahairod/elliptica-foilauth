Name:           elliptica-foilauth
Summary:        Console version of %{name}
Version:        1.1.10
Release:        1
License:        BSD
Group:          Applications/Internet
URL:            https://github.com/mahairod/elliptica-foilauth
Source0:        %{name}-%{version}.tar.gz

%define glib_version 2.32

Requires:       qt5-qtsvg-plugin-imageformat-svg
Requires:		glib2 >= %{glib_version}
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(keepalive)
BuildRequires:  pkgconfig(libcrypto)
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  qt5-qttools-linguist

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

%description
HMAC-Based One-Time Password generator compatible with Google
One-Time Password authentication mechanism.
This package contains a console-only version of %{name}

%if "%{?vendor}" == "chum"
Categories:
 - Utility
Icon: https://raw.githubusercontent.com/monich/harbour-foilauth/master/icons/harbour-foilauth.svg
Url:
  Homepage: https://openrepos.net/content/slava/foil-auth
%endif

%prep
%setup -q

%build
%qtc_qmake5 CONFIG+=openrepos cli-app.pro
%qtc_make %{?_smp_mflags}

%check
make %{?_smp_mflags} -C test test

%install
rm -rf %{buildroot}
%qmake5_install

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}-cli
%{_datadir}/translations/%{name}-cli*.qm
