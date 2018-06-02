%define version	%(cat %{_topdir}/version.txt)

Name:		librhashmap
Version:	%{version}
Release:	1%{?dist}
Summary:	Robin Hood hash map library
Group:		System Environment/Libraries
License:	BSD
URL:		https://github.com/rmind/rhashmap
Source0:	librhashmap.tar.gz

BuildRequires:	make
BuildRequires:	libtool

%description

Robin Hood hash map library -- a general purpose hash table, using open
addressing with linear probing and Robin Hood hashing for the collision
resolution algorithm.  Optimal for solving the _dictionary problem_.
The library provides support for the SipHash and Murmurhash3 algorithms.
The implementation is written in C99 and distributed under the 2-clause
BSD license.

%prep
%setup -q -n src

%check
make tests

%build
make %{?_smp_mflags} lib \
    LIBDIR=%{_libdir}

%install
make install \
    DESTDIR=%{buildroot} \
    LIBDIR=%{_libdir} \
    INCDIR=%{_includedir} \
    MANDIR=%{_mandir}

%files
%{_libdir}/*
%{_includedir}/*
#%{_mandir}/*

%changelog
