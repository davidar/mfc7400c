*Update: also see this more recent [Linux Voice article](http://www.linuxvoice.com/drive-it-yourself-usb-car-6/)*

I had an old Brother MFC-7400C, which didn't work as a printer or fax machine anymore, but the document feeder made it useful for scanning multi-page documents. Unfortunately, it didn't seem to be supported under Linux, so I figured I'd try writing my own implementation by reverse engineering the scanning protocol.

[SniffUSB][1] was used for analysing the USB traffic. By obtaining logs whilst various operations are performed with the scanner, it is possible to determine the characteristics of the protocol relatively easily.

### Reading the SniffUSB Logs

![SniffUSB screenshot](https://raw.githubusercontent.com/davidar/mfc7400c/master/usb-logs/screenshot.png)

After installing the filter, logs were obtained for device initialisation, scanning with default settings with 0, 1, and 2 pages in the document feeder, as well as scanning with various other setting configurations with no pages in the feeder. After this, it is a matter of extracting the relevant information (the [libusb Documentation][2] and the [USB 2.0 Specification][3], particularly sections 9.3 and 9.6, come in handy here). For example, in the following snippet of a control transfer:

```
[136533 ms]  >>>  URB 277 going down  >>> 
-- URB_FUNCTION_VENDOR_DEVICE:
  TransferFlags          = 00000003 (USBD_TRANSFER_DIRECTION_IN, USBD_SHORT_TRANSFER_OK)
  TransferBufferLength = 000000ff
  Request                 = 00000001
  Value                   = 00000002
  Index                   = 00000000
[136535 ms]  <<<  URB 277 coming back  <<< 
-- URB_FUNCTION_CONTROL_TRANSFER:
  TransferBufferMDL    = 81d6bc88
    00000000: 05 10 01 02 00
```

The following request characteristics can be extracted:

 - Data transfer direction = Device-to-host (IN)
 - Type = Vendor
 - Recipient = Device
 - Request = 0x01
 - Value = 0x02
 - Index = 0x00
 - Length = 0xff
 - Device responds 0x05 0x10 0x01 0x02 0x00

With libusb, this can be performed via the following call:

```c
NUM_BYTES_READ = libusb_control_transfer(DEVICE_HANDLE,
        LIBUSB_ENDPOINT_IN |
        LIBUSB_REQUEST_TYPE_VENDOR |
        LIBUSB_RECIPIENT_DEVICE,
        0x01, 0x02, 0x00, BUFFER, 0xff, TIMEOUT);
/* BUFFER = { 0x05, 0x10, 0x01, 0x02, 0x00 } */
```

Similarly, the following outbound bulk transfer to endpoint number 3, with equivalent C code:

```
[692026 ms]  >>>  URB 3034 going down  >>> 
-- URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
  PipeHandle           = 81d0d24c [endpoint 0x00000003]
  TransferFlags        = 00000002 (USBD_TRANSFER_DIRECTION_OUT, USBD_SHORT_TRANSFER_OK)
  TransferBufferLength = 00000004
  TransferBuffer       = 00000000
  TransferBufferMDL    = 81e773b8
    00000000: 1b 58 0a 80
```

```c
BUFFER = { 0x1b, 0x58, 0x0a, 0x80 };
libusb_bulk_transfer(DEVICE_HANDLE, LIBUSB_ENDPOINT_OUT | 0x03,
        BUFFER, 0x04, &NUM_BYTES_SENT, TIMEOUT);
```

And an inbound bulk transfer on endpoint number 4:

```
[137544 ms]  >>>  URB 284 going down  >>> 
-- URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
  PipeHandle           = 81d0d26c [endpoint 0x00000084]
  TransferFlags        = 00000003 (USBD_TRANSFER_DIRECTION_IN, USBD_SHORT_TRANSFER_OK)
  TransferBufferLength = 00001000
[137545 ms]  <<<  URB 284 coming back  <<< 
-- URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
  TransferBufferLength = 00000002
  TransferBufferMDL    = 81cf9dd0
    00000000: c2 00
```

```c
libusb_bulk_transfer(DEVICE_HANDLE, LIBUSB_ENDPOINT_IN | 0x04,
        BUFFER, 0x1000, &NUM_BYTES_READ, TIMEOUT);
/* BUFFER = { 0xc2, 0x00 } */
```

From this, the following protocol characteristics can be determined:

 1. While idling a control transfer is sent every 500ms (type=3,value=0,index=0,length=255), to which the device responds 0x04100300
 2. Scanning is initiated with control transfer (type=1,value=2,index=0,length=255), device responds 0x0510010200
 3. Configuration data sent to scanner with outbound bulk transfer (endpoint=3)
 4. Image data is read from the scanner with inbound bulk transfer (endpoint=4,length=0x1000)
 5. Scanning is ended with control transfer (type=2,value=2,index=0,length=255), device responds 0x0510020200

### Configuration Data

The configuration data is an ASCII key=value string, which can be decoded with a few lines of Python:

```python
# set dump to hexdump of configuration data
out = ''
for line in dump.strip().split('\n'):
    line = line[line.find(': ')+2:].strip()
    for byte in line.split():
        out = out + chr(int(byte, 16))
print repr(out)
```

Correlating various configurations with their respective configuration strings yields the following pattern: the string always begins with "\x1bX\n", followed by 7 key=value pairs each terminated by "\n", followed by "\x80". The following key=value combinations are available:

 - R=X_RESOLUTION,Y_RESOLUTION (where both resolutions must be a multiple of 100, with the x- and y-resolutions capped at 300 and 600 respectively)
 - M=CGRAY (color mode), M=GRAY64 (grey-scale mode), M=TEXT (text mode)
 - C=RLENGTH (run-length encoding?), C=NONE (uncompressed)
 - B=100
 - N=100
 - U=OFF
 - A=0,0,WIDTH,HEIGHT

### Raw Image Data

Inbound bulk transfers usually contain a payload of raw image data, but the following message can also be received:

 - empty payload: wait 200ms and try again
 - 0xc200: there is nothing to scan
 - 0x80: finished scanning the page, no more pages
 - 0x81: finished scanning the page, another page ready (in which case an empty configuration string is sent ("\x1bX\n\x80"), and the next page is scanned)

The uncompressed raw image data is relatively easy to decode. The following snippet shows a color scan of an 816 (0x330) pixel wide plain white image. 

```
00000000: 44 30 03 fd fd fd fd fd fd fd fd fd fd fd fd fd
00000010: fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd
...
00000320: fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd
00000330: fd fd fd 48 30 03 fd fd fd fd fd fd fd fd fd fd
00000340: fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd
...
00000650: fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd
00000660: fd fd fd fd fd fd 4c 30 03 fc fc fc fc fc fc fc
00000670: fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc
...
00000980: fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc
00000990: fc fc fc fc fc fc fc fc fc 44 30 03 fd fd fd fd
000009a0: fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd
...
00000cb0: fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd
00000cc0: fd fd fd fd fd fd fd fd fd fd fd fd 48 30 03 fd
00000cd0: fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd fd
...
```

It can be seen that the data consists of a series of 816-byte rows each prefixed with a 3 byte header. The first byte of the header describes the type of the row (0x40=gray, 0x44=red, 0x48=green, 0x4c=blue), and the following two bytes describe the length of the row in little-endian format.

 [1]: http://pcausa.com/Utilities/UsbSnoop/
 [2]: http://libusb.sourceforge.net/api-1.0/
 [3]: http://www.usb.org/developers/docs/
