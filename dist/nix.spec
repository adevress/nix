# unversionned doc dir F20 change https://fedoraproject.org/wiki/Changes/UnversionedDocdirs
%{!?_pkgdocdir: %global _pkgdocdir %{_docdir}/%{name}-%{version}}

Name:				nix
Version:			1.11.2
Release:			1%{?dist}
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

## systemd for > EL7 
%if 0%{?el6} == 0
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

%configure \
	--localstatedir=/nix/var \
        --docdir=%{_defaultdocdir}/%{name}-%{version} 
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
%if 0%{?el6}
rm -rf %{buildroot}/usr/lib/systemd
%endif

## pkgconfig files need to migrate to lib64 if it exist
mkdir -p %{buildroot}/%{_libdir}/pkgconfig
mv %{buildroot}/usr/lib/pkgconfig/*.pc %{buildroot}/%{_libdir}/pkgconfig/ 2> /dev/null  || true

%pre daemon
useradd --system nix-daemon &> /dev/null || true

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

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
%if 0%{?el6} ==0  
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

