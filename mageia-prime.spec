Name:		mageia-prime
Version:	0.4
Release:	%mkrel 7
Summary:	An NVidia Prime configuration tool
Group:		Development/X11
URL:		https://github.com/ghibo/%{name}
Source0:	%{url}/archive/v%{version}/%{name}-%{version}.tar.gz
License:	GPLv3

Requires:	kmod
Requires:	dnf
Requires:	dnf-command(install)
Requires:	dnf-command(config-manager)
Requires:	urpmi
Requires:	drakxtools-backend
# /bin/update-grub
Requires:	grub2-common
# /bin/lspci
Requires:	pciutils
# systemctl
Requires:	systemd-units
Requires:	xrandr

%description
A tool for easily configuring NVidia Prime under Mageia GNU/Linux.

%prep
%setup -q
%autopatch -p1

%build
%make_build CFLAGS="%{optflags} -Wall -pedantic -D_GNU_SOURCE"

%install
%make_install

%files
%doc README.md
%license LICENSE
%{_bindir}/mageia-prime-offload
%{_bindir}/mageia-prime-offload-run
%{_bindir}/mageia-prime-install-packages-large
%{_sbindir}/mageia-prime-install
%{_sbindir}/mageia-prime-uninstall

%changelog
* Fri Jun 02 2017 Giuseppe Ghibò <ghibo> 0.4-1.mga6
- Release 0.4.

* Fri May 23 2017 Giuseppe Ghibò <ghibo> 0.3-1.mga6
- Release 0.3.

* Mon May 22 2017 Giuseppe Ghibò <ghibo> 0.2-1.mga6
- Release 0.2.

* Wed May 10 2017 Giuseppe Ghibò <ghibo> 0.1-1.mga6
- Initial release.
