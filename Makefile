WAF=./waf

.PHONY: all build configure clean dist distclean

build: fl_autogen fl_configure
	${WAF} build

fl_configure: fl_autogen
	${WAF} configure
	touch fl_configure

configure: fl_autogen
	rm -f fl_configure
	make fl_configure

fl_autogen: $(wildcard autogen/*.auto)
	python -c 'import build_misc; build_misc.autogen ()'
	touch fl_autogen

dist: fl_autogen
	# Exclude flags
	if [ -e fl_autogen ]; then FL_AUTOGEN=1; rm fl_autogen; \
		else FL_AUTOGEN=0; fi; \
	if [ -e fl_configure ]; then FL_CONFIGURE=1; rm fl_configure; \
		else FL_CONFIGURE=0; fi; \
	${WAF} dist; \
	if [ $$FL_AUTOGEN = 1 ]; then touch fl_autogen; fi; \
	if [ $$FL_CONFIGURE = 1 ]; then touch fl_configure; fi

# Tricky part - we need to delete the distribution tarball if it was generated
# by 'make dist', but its name comes from variables in autogen.conf
distclean: fl_autogen
	${WAF} distclean
	python -c 'import build_misc; build_misc.autogen_clean ()'
	rm -f build_misc.pyc
	rm fl_autogen fl_configure
	PKGNAME=$$(grep ^PKGNAME autogen.conf | sed -e 's/[^=]*=\s*//' -e 's/\s*$$//'); \
	VERSION=$$(grep ^VERSION autogen.conf | sed -e 's/[^=]*=\s*//' -e 's/\s*$$//'); \
	rm -f $$PKGNAME-$$VERSION.tar.bz2

clean: fl_autogen
	${WAF} clean
	python -c 'import build_misc; build_misc.autogen_clean ()'
	rm -f build_misc.pyc
	rm -f fl_autogen fl_configure
