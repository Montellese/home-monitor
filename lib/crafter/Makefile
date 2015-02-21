include ../../Makefile.include

LIBNAME = crafter
VERSION = 0.2
SOURCE = $(LIBNAME)-$(VERSION)
SUFFIX = tar.gz
ARCHIVE = $(SOURCE).$(SUFFIX)
BASE_URL = https://libcrafter.googlecode.com/files

LIB = $(SOURCE)/.libs/lib$(LIBNAME).so

all: .installed-$(SOURCE)

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$@

$(SOURCE): $(ARCHIVE)
	rm -rf $@/*; mkdir -p $@
	cd $@; $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) ../$<
	cd $@; $(CONFIGURE_TOOL) $(CONFIGURE_FLAGS)

$(LIB): $(SOURCE)
	$(MAKE) -C $<

.installed-$(SOURCE): $(LIB)
	$(MAKE) -C $(SOURCE) install
	touch $@

clean:
	@$(MAKE) -C $(SOURCE) clean
	@rm -f .installed-$(SOURCE)

distclean:
	@rm -rf $(SOURCE) .installed-$(SOURCE)
