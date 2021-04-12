Name:       sailfish-components-accounts-qt5
Summary:    Sailfish Accounts Components
Version:    0.2.49
Release:    1
Group:      System/Libraries
License:    Proprietary
URL:        https://bitbucket.org/jolla/ui-sailfish-components-accounts
Source0:    %{name}-%{version}.tar.bz2
Source1:    %{name}.privileges
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5OpenGL)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(libsignon-qt5)
BuildRequires:  pkgconfig(accounts-qt5) >= 1.16
BuildRequires:  pkgconfig(libsailfishkeyprovider)
BuildRequires:  pkgconfig(buteosyncfw5) >= 0.9.6
BuildRequires:  pkgconfig(qt5-boostable)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(sailfishaccesscontrol)
BuildRequires:  ssu-devel
BuildRequires:  qt5-qttools-linguist
BuildRequires:  qt5-qttools
Requires:  mapplauncherd-qt5
Requires:  sailfishsilica-qt5 >= 1.1.53
Requires:  buteo-syncfw-qt5 >= 0.9.6

%description
Sailfish Accounts UI Components

%package tests
Summary:    Unit tests for Sailfish Accounts UI Components
Group:      System/Libraries
BuildRequires:  pkgconfig(Qt5Test)
Requires:   %{name} = %{version}-%{release}
Requires:   qt5-qtdeclarative-devel-tools
Requires:   blts-tools

%description tests
This package contains QML unit tests for Sailfish Accounts UI Components

%package devel
Summary:    Development package for Sailfish Accounts
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Development package which provides libsailfishaccounts (package config and headers)

%package ts-devel
Summary:   Translation source for sailfish-components-accounts
Group:     System/Libraries

%description ts-devel
Translation source for sailfish-components-accounts

%package doc
Summary: Documentation for %{name}
Group: Documentation
BuildRequires: qt5-qttools-qthelp-devel
BuildRequires: qt5-tools
BuildRequires: qt5-plugin-platform-minimal
BuildRequires: qt5-plugin-sqldriver-sqlite

%description doc
%{summary}.

%package tools
Summary: Development tools for %{name}
Group: Applications/System
Requires: %{name} = %{version}-%{release}

%description tools
%{summary}.


%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}" 
make %{?_smp_mflags}
make docs

%install
rm -rf %{buildroot}
%qmake5_install
mkdir -p %{buildroot}/%{_docdir}/%{name}
cp -R doc/html/* %{buildroot}/%{_docdir}/%{name}/

mkdir -p %{buildroot}%{_datadir}/mapplauncherd/privileges.d
install -m 644 -p %{SOURCE1} %{buildroot}%{_datadir}/mapplauncherd/privileges.d/

%files
%defattr(-,root,root,-)
%{_libdir}/libsailfishaccounts.so.*
%{_libdir}/qt5/qml/Sailfish/Accounts
%{_datadir}/translations/*.qm

%files tests
%defattr(-,root,root,-)
/opt/tests/Sailfish/Accounts
%{_datadir}/accounts/providers/test-provider.provider
%{_datadir}/accounts/services/test-service2.service
%{_datadir}/accounts/services/test-service-oauth.service
%{_datadir}/accounts/service_types/test-service-type2.service-type

%files devel
%defattr(-,root,root,-)
%{_libdir}/libsailfishaccounts.so
%{_libdir}/pkgconfig/sailfishaccounts.pc
%{_includedir}/libsailfishaccounts

%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/*.ts

%files doc
%defattr(-,root,root,-)
%{_docdir}/%{name}

%files tools
%defattr(-,root,root,-)
%{_bindir}/sailfish-accounts-tool
%{_datadir}/mapplauncherd/privileges.d/*

%post
/sbin/ldconfig

%postun
/sbin/ldconfig
