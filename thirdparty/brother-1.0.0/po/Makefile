# Generated automatically from Makefile.in by configure.
# po/Makefile.in
# See po/README for documentation.
SHELL = /bin/sh
all: check-mo

# Create one line per backend with all files containing translatable text:
#epson.*.po: ../backend/epson.c ../backend/epson.h 
#mustek.*.po: ../backend/mustek.c
#mustek_usb.*.po: ../backend/mustek_usb.c
#plustek.*.po: ../backend/plustek.c ../backend/plustek.h
#pnm.*.po: ../backend/pnm.c
#umax.*.po: ../backend/umax.c

# end of configuration

prefix = /usr/local
exec_prefix = ${prefix}
datadir = ${prefix}/share
localedir = $(datadir)/locale
gnulocaledir = $(prefix)/share/locale
top_srcdir = ..
srcdir = .

PACKAGE = sane-backends
VERSION = 1.0.7
distdir = $(top_srcdir)/$(PACKAGE)-$(VERSION)

INSTALL_TRANSLATIONS = 
ifeq ($(INSTALL_TRANSLATIONS),install-translations)
UNINSTALL_TRANSLATIONS = uninstall-translations
UPDATE_MO = update-mo
endif

MKDIR = $(top_srcdir)/mkinstalldirs
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

MSGFMT = msgfmt
XGETTEXT = xgettext
MSGMERGE = msgmerge

TMP_FILE_DIR = .tmp

ALL_POS = $(wildcard *.po)
ALL_BACKENDS = $(sort $(basename $(basename $(ALL_POS))))
ALL_POTS = $(addsuffix .pot,$(ALL_BACKENDS))
ALL_MOS = $(filter-out saneopts.%.mo,$(addsuffix .mo,$(basename $(ALL_POS))))
ALL_LINGUAS = $(sort $(subst .,,$(suffix $(basename $(ALL_POS)))))

DISTFILES = Makefile.in README epson.de.po mustek.de.po \
  mustek_usb.de.po plustek.de.po pnm.de.po saneopts.de.po umax.de.po 

.PHONY: all clean depend dist distclean install install-translations \
  uninstall update-mo update-po generate-%

.SUFFIXES: .po .mo

saneopts.*.po: ../include/sane/saneopts.h

# backend.lang.po -> backend.lang.mo
.po.mo:
	@lang=$(subst .,,$(suffix $(basename $(subst install-,,$@)))) ; \
	 echo generating $@ from $< and saneopts.$${lang}.po ; \
	 $(MSGFMT) -o $@ $< saneopts.$${lang}.po

# Sourcefiles -> backend.lang.po (updating po)
$(foreach lang,$(ALL_LINGUAS),%.$(lang).po):
	@echo -n "updating $@ from $^ "
	@rm -rf $(TMP_FILE_DIR)
	@mkdir $(TMP_FILE_DIR) && \
	 for file in $^ ; do \
	   sed < $${file} -e 's/#define//g' \
	     > $(TMP_FILE_DIR)/`basename $${file}` ; \
	 done
	@$(XGETTEXT) -d$* -kSANE_I18N $(TMP_FILE_DIR)/*.*
	@mv $*.po $*.pot
	@cp $@ $@.old 
	@$(MSGMERGE) $@.old $*.pot -o $@ 
	@rm -f $@.old 

# Generate new po file
%-gen:
	@if test ! -e $* ; then \
	   touch $* -d "1980-01-01" ; \
	   echo created new file: $* ; \
	   $(MAKE) $* ; \
	 fi

update-po: $(ALL_POS)

update-mo: $(ALL_MOS)

check-mo: $(UPDATE_MO)

$(addprefix install-,$(ALL_MOS)):
	@mo_file=$(subst install-,,$@) ; \
	 backend=$(basename $(basename $(subst install-,,$@))) ; \
	 lang=$(subst .,,$(suffix $(basename $(subst install-,,$@)))) ; \
	 dir=$(gnulocaledir)/$${lang}/LC_MESSAGES ; \
	 echo installing  $${mo_file} to $${dir}/sane-$${backend}.mo ; \
	 $(MKDIR) $${dir} && \
	 $(INSTALL_DATA) $${mo_file} $${dir}/sane-$${backend}.mo ;

install-translations: $(addprefix install-,$(ALL_MOS))

install: $(INSTALL_TRANSLATIONS)

uninstall-translations:
	@for lang in $(ALL_LINGUAS) ; do \
	   dir=$(gnulocaledir)/$${lang}/LC_MESSAGES ; \
	   echo removing $${dir}/sane-*.mo ; \
	   rm -f $${dir}/sane-*.mo ; \
	 done

uninstall: $(UNINSTALL_TRANSLATIONS)

clean:
	rm -f *.mo
	rm -f *.old

distclean: clean
	rm -f Makefile
	rm -f *.pot
	rm -f *~
	rm -rf $(TMP_FILE_DIR)

depend:

dist: $(DISTFILES)
	for file in $(DISTFILES); do \
	  ln $$file $(distdir)/po 2> /dev/null \
	    || cp -p $$file $(distdir)/po ; \
	done


