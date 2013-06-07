Name:       sailfish-components-accounts

Summary:    Sailfish Accounts UI Components
Version:    0.0.7
Release:    1
Group:      System/Libraries
License:    TBD
URL:        https://bitbucket.org/jolla/ui-sailfish-components-accounts
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(QtCore) >= 4.8.0
BuildRequires:  pkgconfig(QtDeclarative)
BuildRequires:  pkgconfig(QtGui)
BuildRequires:  pkgconfig(QtOpenGL)
BuildRequires:  pkgconfig(libsignon-qt)
BuildRequires:  pkgconfig(accounts-qt)
BuildRequires:  pkgconfig(libsailfishkeyprovider)

Requires:  sailfishsilica >= 0.8.33
Requires:  nemo-qml-plugins-accounts >= 0.2.1
Requires:  nemo-qml-plugins-signon >= 0.2.1
Requires:  jolla-signon-ui
Requires:  libbluez-qt
Requires:  libjollasignonuiservice

Obsoletes: sailfish-accounts <= 0.0.2
Provides:  sailfish-accounts > 0.0.2

%description
Sailfish Accounts UI Components

%package tests
Summary:    Unit tests for Sailfish Accounts UI Components
Group:      System/Libraries
BuildRequires:  pkgconfig(QtTest)
Requires:   %{name} = %{version}-%{release}
Requires:   qtest-qml

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

%qmake

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%qmake_install

#
# Jolla Components internal files
#
%files
%defattr(-,root,root,-)
%{_libdir}/qt4/imports/Sailfish/Accounts/*
%{_datadir}/translations/sailfish_components_accounts_eng_en.qm

#
# Jolla Components internal translation files
#
%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/sailfish_components_accounts.ts

