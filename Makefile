CFLAGS = -O2 -Wall -pedantic -D_GNU_SOURCE

all: mageia-prime-install mageia-prime-uninstall

mageia-prime-install: mageia-prime-install.c
	$(CC) $(CFLAGS) -o mageia-prime-install mageia-prime-install.c

mageia-prime-uninstall: mageia-prime-uninstall.c
	$(CC) $(CFLAGS) -o mageia-prime-uninstall mageia-prime-uninstall.c

install: mageia-prime-install mageia-prime-uninstall
	install -m 755 mageia-prime-install mageia-prime-uninstall $(DESTDIR)/usr/sbin

clean:
	rm -f mageia-prime-install mageia-prime-uninstall
