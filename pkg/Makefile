PROJ=	rhashmap

all: rpm

rpm:
	mkdir -p SOURCES && tar czpvf SOURCES/lib$(PROJ).tar.gz ../src
	rpmbuild -ba -v --define "_topdir ${PWD}" SPECS/lib$(PROJ).spec
	@ echo && printf "\x1B[32mYour RPM packages:\033[0m\n" && ls -1 RPMS/*

clean:
	rm -rf BUILD BUILDROOT RPMS SOURCES SRPMS

.PHONY: all rpm clean
