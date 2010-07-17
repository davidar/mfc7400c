all:
	gcc -std=gnu99 -I/usr/include/libusb-1.0 -o scan scan.c -lusb-1.0
	gcc -std=gnu99 -o raw2pnm raw2pnm.c
