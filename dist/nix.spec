# unversionned doc dir F20 change https://fedoraproject.org/wiki/Changes/UnversionedDocdirs
%{!?_pkgdocdir: %global _pkgdocdir %{_docdir}/%{name}-%{version}}

Name:				nix
Version:			1.11.2
Release:			2%{?dist}
Summary:			Nix package manager
Group:				Applications/Internet
License:			LGPLv3
URL:				https://nixos.org/nix/
Source0:			http://nixos.org/releases/nix/nix-1.11.2/nix-1.11.2.tar.xz
BuildRoot:			%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

# C Libs
BuildRequires:			bzip2-devel
BuildRequires: 			libcurl-devel
BuildRequires:			openssl-devel
BuildRequires: 			sqlite-devel
BuildRequires:			xz-devel


# perl dependency
BuildRequires:			perl(DBD::SQLite)
BuildRequires:			perl(ExtUtils::ParseXS)
BuildRequires:			perl(WWW::Curl)

## systemd or systemV
%if 0%{?el6}

Requires(post): chkconfig
Requires(preun): chkconfig
# This is for /sbin/service
Requires(preun): initscripts

%else

BuildRequires:			systemd

%endif

## RHEL 6 compilation need to have
## the software collections enable
%if 0%{?el6}
BuildRequires:			devtoolset-3-binutils
BuildRequires:			devtoolset-3-gcc-c++
%endif

%description
Nix is a powerful package manager for Linux and other 
Unix systems that makes package management reliable and reproducible


%package libs
Summary:			Libraries for %{name}
Group:				Applications/Internet

%description libs
Libraries for %{name}


%package doc
Summary:                        Documentation for %{name}
Group:                          Applications/Internet

%description doc
Documentation for %{name}


%package devel
Summary:                        Development files for %{name}
Group:                          Applications/Internet
Requires:			%{name}%{?_isa} = %{version}-%{release}
Requires:			pkgconfig

%description devel
Development files for %{name}

%package daemon
Summary:                        Multi-user daemon for %{name}
Group:                          Applications/Internet
Requires:                       %{name}%{?_isa} = %{version}-%{release}
Requires(pre):			shadow-utils

%description daemon
Multi-user daemon and configuration for %{name}

%package -n emacs-nix
Summary:                        Emacs add-on package for nix
Group:                          Applications/Internet
BuildArch: noarch


%description -n emacs-nix
Emacs add-on package for nix



%clean
rm -rf %{buildroot}
make clean

%prep
%setup -q

%build
## import SCL in current environment for RHEL6 
%if 0%{?el6}
source /opt/rh/devtoolset-3/enable
%endif

export perllibdir=%{_libdir}/perl5/
%configure \
	--localstatedir=/nix/var \
        --docdir=%{_defaultdocdir}/%{name}-%{version} \
	--with-perl-libdir=%{_libdir}/perl5/vendor_perl
make %{?_smp_mflags}





%install
## import SCL in current environment for RHEL6 
%if 0%{?el6}
source /opt/rh/devtoolset-3/enable
%endif

rm -rf %{buildroot}
make DESTDIR=%{buildroot} install
install -m 0755 -D misc/conf/nix.conf %{buildroot}/%{_sysconfdir}/nix/nix.conf

 
## remove upstart file 
rm %{buildroot}/%{_sysconfdir}/init/nix-daemon.conf
rm %{buildroot}/%{_sysconfdir}/profile.d/nix.sh

## remove the systemd files in case of RHEL6
## and install systemv files
%if 0%{?el6}
rm -rf %{buildroot}/usr/lib/systemd
install -m 0755 -D misc/systemv/nix-daemon /etc/init.d/nix-daemon
%else
rm -rf %{buildroot}/%{_unitdir}/nix-daemon.socket*
%endif

## install nix config in daemon mode
##
install -m 0755 -D misc/conf/nix.conf %{buildroot}/etc/nix/nix.conf
install -m 0755 -D misc/conf/user-channels.list %{buildroot}/etc/nix/user-channels.list

## sysconfig
mkdir -p %{buildroot}/etc/sysconfig
touch %{buildroot}/etc/sysconfig/nix-daemon

## pkgconfig files need to migrate to lib64 if it exist
mkdir -p %{buildroot}/%{_libdir}/pkgconfig
mv %{buildroot}/usr/lib/pkgconfig/*.pc %{buildroot}/%{_libdir}/pkgconfig/ 2> /dev/null  || true

## create nix dir for daemon
mkdir -p %{buildroot}/nix
mkdir -m 0777 -p %{buildroot}/nix/var/nix/profiles/per-user
mkdir -m 0777 -p %{buildroot}/nix/var/nix/gcroots/per-user
mkdir -p %{buildroot}/nix/var/nix/empty

%pre daemon
useradd --system nix-daemon &> /dev/null || true
groupadd --system nix-admins &> /dev/null || true

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%post daemon
%if 0%{?el6}

/sbin/chkconfig --add nix-daemon

%else

%systemd_post nix-daemon.service

%endif

%preun daemon
%if 0%{?el6}

if [ $1 -eq 0 ] ; then
    /sbin/service nix-daemon stop >/dev/null 2>&1
    /sbin/chkconfig --del nix-daemon
fi

%else

%systemd_preun nix-daemon.service

%endif

%postun daemon
%systemd_postun_with_restart nix-daemon.service

%files devel
%defattr (-,root,root)
%{_includedir}/*
%{_libdir}/pkgconfig/*

%files libs
%defattr (-,root,root)
%{_libdir}/*.so*
%{_libexecdir}/nix
%{_datadir}/nix
%{_libdir}/perl5/*

%files doc
%defattr (-,root,root)
%{_pkgdocdir}/*

%files daemon
%defattr (-,root,root)
%{_bindir}/nix-daemon
%{_mandir}/man5/*.5*
%{_mandir}/man8/*.8*
%config(noreplace) %{_sysconfdir}/*
%attr(-, nix-daemon, nix-daemon) /nix
%attr(0777, nix-daemon, nix-daemon) /nix/var/nix/profiles/per-user
%attr(0777, nix-daemon, nix-daemon) /nix/var/nix/gcroots/per-user


%if 0%{?el6}
%{_sysconfdir}/init.d/*
%else
## unit file are useless under RHEL6
%{_unitdir}/*
%endif

%files -n emacs-nix
%defattr (-,root,root)
%{_datadir}/emacs/site-lisp/nix-mode.el

%files
%defattr (-,root,root)
%{_bindir}/*
%{_mandir}/man1/*.1*


%changelog
* Thu Mar 10 2016 Adrien Devresse <adevress at cern.ch> - 1.11-2.2
 - add nix-admins group by default

* Wed Mar 09 2016 Adrien Devresse <adevress at epfl.ch> - 1.11.2-1
 - Initial version
 - enhanced packaging adapted to multi-user usage as side packager
 - define execution of nix-daemon without root right ( nix-daemon user )
 - include multi user environment configuration script
 - support systemV and RHEL6 with software collections
 - support improved for systemd
 - reconfigure perl module directory for Fedora/RHEL conformance
 - support admin level configuration for channels

