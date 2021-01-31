CFLAGS = -O2 -Wall -pedantic -D_GNU_SOURCE # -Wno-stringop-truncation -Wno-stringop-overflow

all: mageia-prime-install mageia-prime-uninstall

mageia-prime-install: mageia-prime-install.c
	$(CC) $(CFLAGS) -o mageia-prime-install mageia-prime-install.c

mageia-prime-uninstall: mageia-prime-uninstall.c
	$(CC) $(CFLAGS) -o mageia-prime-uninstall mageia-prime-uninstall.c

install: mageia-prime-install mageia-prime-uninstall
	test -d $(DESTDIR)/usr/sbin || mkdir -p $(DESTDIR)/usr/sbin
	test -d $(DESTDIR)/usr/bin || mkdir -p $(DESTDIR)/usr/bin
	install -m 755 mageia-prime-install mageia-prime-uninstall $(DESTDIR)/usr/sbin
	install -m 755 mageia-prime-offload mageia-prime-offload-run $(DESTDIR)/usr/bin
	install -m 755 mageia-prime-install-packages-large $(DESTDIR)/usr/bin
	
clean:
	rm -f mageia-prime-install mageia-prime-uninstall
