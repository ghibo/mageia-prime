Name:		mageia-prime
Version:	0.2
Release:	%mkrel 1
Summary:	An NVidia Prime configuration tool
Group:		Development/X11
URL:		https://github.com/ghibo/%{name}
Source0:	%{url}/archive/v%{version}/%{name}-%{version}.tar.gz
License:	GPLv3
Requires:	pciutils
Requires:	kmod
Requires:	dnf
Requires:	dnf-command(install)
Requires:	dnf-command(config-manager)

%description
A tool for easily configuring NVidia Prime under Mageia GNU/Linux.

%prep
%setup -q

%build
%make_build CFLAGS="%{optflags} -Wall -pedantic -D_GNU_SOURCE"

%install
%make_install

%files
%doc README.md
%license LICENSE
%{_sbindir}/mageia-prime-install
%{_sbindir}/mageia-prime-uninstall

%changelog
* Mon May 22 2017 Giuseppe Ghibò <ghibo> 0.2-1.mga6
- Release 0.2.

* Wed May 10 2017 Giuseppe Ghibò <ghibo> 0.1-1.mga6
- Initial release.
