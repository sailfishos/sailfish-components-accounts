Name:       sailfish-components-accounts-qt5

Summary:    Sailfish Accounts UI Components
Version:    0.0.10
Release:    1
Group:      System/Libraries
License:    TBD
URL:        https://bitbucket.org/jolla/ui-sailfish-components-accounts
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5OpenGL)
BuildRequires:  pkgconfig(libsignon-qt5)
BuildRequires:  pkgconfig(accounts-qt5)
BuildRequires:  qt5-qttools
BuildRequires:  qt5-qttools-linguist

Requires:  sailfishsilica-qt5
Requires:  nemo-qml-plugin-accounts-qt5
Requires:  nemo-qml-plugin-signon-qt5
Requires:  jolla-signon-ui
Requires:  libjollasignonuiservice
Requires:  qca-ossl

%description
Sailfish Accounts UI Components

%package tests
Summary:    Unit tests for Sailfish Accounts UI Components
Group:      System/Libraries
BuildRequires:  pkgconfig(Qt5Test)
Requires:   %{name} = %{version}-%{release}
Requires:   qt5-qtdeclarative-devel-tools

%description tests
This package contains QML unit tests for Sailfish Accounts UI Components

%package ts-devel
Summary:   Translation source for sailfish-components-accounts
License:   TBD
Group:     System/Libraries

%description ts-devel
Translation source for sailfish-components-accounts

%prep
%setup -q -n %{name}-%{version}

%build

%qmake5

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%qmake5_install

%files
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/Sailfish/Accounts/*
%{_datadir}/translations/sailfish_components_accounts_qt5_eng_en.qm

%files tests
%defattr(-,root,root,-)
/opt/tests/Sailfish/Accounts/qt5/*
%{_datadir}/accounts/providers/test-provider.provider
%{_datadir}/accounts/services/test-service2.service
%{_datadir}/accounts/service_types/test-service-type2.service-type

%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/sailfish_components_accounts_qt5.ts

