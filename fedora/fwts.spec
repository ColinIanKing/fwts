# Only these 4 values need to change for package version control
%global major 16
%global minor 12
%global subminor 00
%global fedora_version 0

%global tarversion V%{major}.%{minor}.%{subminor}

Summary: Firmware Test Suite

Name:    fwts
Version: %{major}
Release: %{minor}.%{subminor}.%{fedora_version}%{?dist}
License: GPLv2, LGPL
Source0: http://fwts.ubuntu.com/release/fwts-%{tarversion}.tar.gz
BuildRequires: acpica-tools glib-devel glib2-devel glib json-c-devel libtool automake autoconf dkms kernel-devel git bison flex

%description
Firmware Test Suite (FWTS) is a test suite that performs sanity checks on
Intel/AMD PC firmware. It is intended to identify BIOS and ACPI errors and if
appropriate it will try to explain the errors and give advice to help
workaround or fix firmware bugs. It is primarily intended to be a Linux-centric
firmware troubleshooting tool.

%prep
%setup -a 0 -n fwts-%{tarversion} -c
git init
git config user.email "example@example.com"
git config user.name "RHEL Ninjas"
git add .
git commit -a -q -m "fwts %{tarversion} baseline."
# uncomment if patches need to be applied
#git am %{patches}

# *** This is useful if testing patches against latest upstream version ***
# %setup -T -c -n fwts-%{tarversion}
# git clone git://kernel.ubuntu.com/hwe/fwts.git ./
#  uncomment if patches need to be applied
# git am %{patches}
# ***

%build
autoreconf -ivf
./configure
make %{?_smp_mflags}

%install
mkdir -p $RPM_BUILD_ROOT/%{_bindir}
mkdir -p $RPM_BUILD_ROOT/%{_datarootdir}/fwts
mkdir -p $RPM_BUILD_ROOT/%{_lib}
mkdir -p $RPM_BUILD_ROOT/%{_mandir}/man1
mkdir -p $RPM_BUILD_ROOT/usr/local/share/fwts

install -m 755 src/.libs/fwts $RPM_BUILD_ROOT/%{_bindir}
install -m 755 live-image/fwts-frontend-text $RPM_BUILD_ROOT/%{_bindir}
install -m 755 scripts/fwts-collect $RPM_BUILD_ROOT/%{_bindir}

install -m 644 live-image/fwts-live-dialogrc $RPM_BUILD_ROOT/%{_datarootdir}/fwts
install -m 777 src/lib/src/.libs/libfwts.so.* $RPM_BUILD_ROOT/%{_lib}
install -m 777 src/acpica/.libs/libfwtsacpica.so* $RPM_BUILD_ROOT/%{_lib}
install -m 777 src/acpica/source/compiler/.libs/libfwtsiasl.so* $RPM_BUILD_ROOT/%{_lib}
install -m 644 doc/fwts-frontend-text.1 $RPM_BUILD_ROOT/%{_mandir}/man1
install -m 644 doc/fwts.1 $RPM_BUILD_ROOT/%{_mandir}/man1
install -m 644 doc/fwts-collect.1 $RPM_BUILD_ROOT/%{_mandir}/man1
install -m 644 data/klog.json $RPM_BUILD_ROOT/usr/local/share/fwts
install -m 644 data/syntaxcheck.json $RPM_BUILD_ROOT/usr/local/share/fwts

%clean
%post
%postun

%files
%{_bindir}/*
%{_datarootdir}/fwts
/%{_lib}/*
%{_mandir}/*/*
/usr/local/share/fwts/*

%changelog
* Thu Jan 19 2017 Prarit Bhargava <prarit@redhat.com> 16.12.00.0
- fix location of klog.json
- add fedora version to differentiate between fedora builds

* Thu Jan 12 2017 Prarit Bhargava <prarit@redhat.com> 16.12.00
- initial specfile creation
- sync to stable V16.12.00
