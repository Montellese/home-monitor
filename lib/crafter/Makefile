include ../../Makefile.include

LIBNAME = crafter
VERSION = 0.3
PKG = $(LIBNAME)-$(VERSION)
SOURCE = $(PKG)/libcrafter
SUFFIX = tar.gz
ARCHIVE = version-$(VERSION).$(SUFFIX)
BASE_URL = https://github.com/pellegre/libcrafter/archive/refs/tags/

LIB = $(SOURCE)/.libs/lib$(LIBNAME).so

.PHONY: all clean distclean install

all: $(LIB)

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$@

$(PKG): $(ARCHIVE)
	rm -rf $@/*; mkdir -p $@
	cd $@; $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) ../$<
	cd $(SOURCE); ./autogen.sh

$(LIB): $(PKG)
	$(MAKE) -C $(SOURCE)

.installed-$(PKG): $(LIB)
	$(MAKE) -C $(SOURCE) install
	touch $@

install: .installed-$(PKG)

clean:
	@$(MAKE) -C $(SOURCE) clean
	@rm -f .installed-$(PKG)

distclean:
	@rm -rf $(PKG) .installed-$(PKG)
