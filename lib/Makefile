include ../Makefile.include

DEPENDS += crafter

.PHONY: $(DEPENDS) clean distclean

all: .installed

.installed: $(DEPENDS)
	touch $@

$(DEPENDS):
	$(MAKE) -C $@

clean:
	for l in $(DEPENDS); do $(MAKE) -C $$l clean; done
	rm -f .installed

distclean:
	for l in $(DEPENDS); do $(MAKE) -C $$l distclean; done
	rm -f .installed

