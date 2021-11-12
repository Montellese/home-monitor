include Makefile.include

CFLAGS = -g -Werror -Wno-error=deprecated-declarations -Wall -Wextra -Wconversion -Wpedantic -Wno-psabi
LDFLAGS = -L$(STAGING_LIBDIR) -Wl,-rpath,$(STAGING_LIBDIR)

INCLUDES = -Isrc -I$(STAGING_INCLUDEDIR)
LIBS = -lpthread -llog4cxx -lcrafter -lssh -lPocoFoundation -lPocoUtil -lPocoXML

SRCS = src/main.cpp \
       src/Configuration.cpp \
       src/Networking.cpp

OBJS = $(SRCS:.cpp=.o)

MAIN = home-monitor

.PHONY: all clean

all: $(MAIN)
	@echo $< successfully built.

$(MAIN): lib/.installed $(OBJS)
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
	sudo setcap 'cap_net_admin,cap_net_raw+ep' $@

lib/.installed:
	make -C lib

.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

install: all
	$(INSTALL) -c $(MAIN) $(PREFIX)/bin/
	mkdir -p /etc/$(MAIN)
	$(INSTALL) -c $(MAIN).xml.example /etc/$(MAIN)/
	$(INSTALL) -c $(STAGING_LIBDIR)/libcrafter.so* $(PREFIX)/lib/
	$(INSTALL) -c home-monitor.service /etc/systemd/system/
	systemctl daemon-reload

clean:
	$(RM) src/*.o *~ src/*~ $(MAIN)

