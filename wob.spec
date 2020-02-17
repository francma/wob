Name:     wob
Version:  0.6
Release:  1%{?dist}.wef
Summary:  A lightweight overlay volume/backlight/progress/anything bar for Wayland.
License:  MIT
URL:      https://github.com/francma/wob

# use this to fetch the source: spectool -g wob.spec
Source0:  %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires: gcc
BuildRequires: meson
BuildRequires: scdoc
BuildRequires: wayland-devel
BuildRequires: wayland-protocols-devel

%description
A lightweight overlay volume/backlight/progress/anything bar for Wayland. This project is inspired by xob - X Overlay Bar.
%prep
%autosetup

%build
%meson
%meson_build

%install
%meson_install

%files
%{_bindir}/%{name}

%doc README.md
%{_mandir}/man1/%{name}.1.*

%license LICENSE

%changelog
* Mon Feb 17 2020 Bob Hepple <bob.hepple@gmail.com> - 0.6-1.fc31.wef
- Initial version of the package
