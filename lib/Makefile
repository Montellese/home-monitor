include ../Makefile.include

DEPENDS += crafter

.PHONY: $(DEPENDS) all clean distclean install

all: $(DEPENDS)

$(DEPENDS):
	$(MAKE) -C $@

install: $(DEPENDS)
	$(MAKE) -C $< install

clean:
	for l in $(DEPENDS); do $(MAKE) -C $$l clean; done

distclean:
	for l in $(DEPENDS); do $(MAKE) -C $$l distclean; done

