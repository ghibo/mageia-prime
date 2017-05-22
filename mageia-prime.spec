Name:		mageia-prime
Version:	0.2
Release:	%mkrel 1
Summary:	An NVidia Prime configuration tool
Group:		Development/X11
URL:		http://www.github.com/ghibo/mageia-prime
Source0:	https://github.com/ghibo/mageia-prime/archive/v%{version}.tar.gz
License:	GPLv3
Requires:	pciutils
Requires:	urpmi
Requires:	kmod

%description
A tool for easily configuring NVidia Prime under Mageia GNU/Linux.

%prep
%setup -q

%build
%make_build CFLAGS="%{optflags} -Wall -pedantic -D_GNU_SOURCE"

%install
mkdir -p %{buildroot}%{_sbindir}
%make_install

%files
%doc README.md
%license LICENSE-GPL-3.0.txt
%{_sbindir}/mageia-prime-install
%{_sbindir}/mageia-prime-uninstall

%changelog
* Mon May 22 2017 Giuseppe Ghibò <ghibo> 0.2-1.mga6
- Release 0.2.

* Wed May 10 2017 Giuseppe Ghibò <ghibo> 0.1-1.mga6
- Initial release.
