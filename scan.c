#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libusb.h>

#define VID 0x04f9
#define PID 0x0107
#define PAGE_WIDTH 816
#define PAGE_HEIGHT 1376
#define TIMEOUT 1000

#define MAX(a,b) (a>b?a:b)
#define MIN(a,b) (a<b?a:b)

libusb_device_handle *device_handle;

void control_in_vendor_device(int request, int value, int index, int length) {
    unsigned char *buf = calloc(length, sizeof(unsigned char));
    int n = libusb_control_transfer(device_handle,
            LIBUSB_ENDPOINT_IN |
            LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_DEVICE,
            request, value, index, buf, length, TIMEOUT);
    if(n < 0) {
        fprintf(stderr, "Control transfer failed: ");
        switch(n) {
            case LIBUSB_ERROR_TIMEOUT:
                fprintf(stderr, "transfer timed out"); break;
            case LIBUSB_ERROR_PIPE:
                fprintf(stderr, "control request was not supported "
                                "by the device");
                break;
            case LIBUSB_ERROR_NO_DEVICE:
                fprintf(stderr, "device has been disconnected"); break;
            default:
                fprintf(stderr, "error %d", n); break;
        }
        fprintf(stderr, "\n");
    } else if(n > 0) {
        fprintf(stderr, "Received %d bytes:", n);
        for(int i = 0; i < n; i++)
            fprintf(stderr, " %02x", buf[i]);
        fprintf(stderr, "\n");
    }
    free(buf);
}

void bulk_transfer_error(int code) {
    fprintf(stderr, "Bulk transfer failed: ");
    switch(code) {
        case LIBUSB_ERROR_TIMEOUT:
            fprintf(stderr, "transfer timed out"); break;
        case LIBUSB_ERROR_PIPE:
            fprintf(stderr, "endpoint halted"); break;
        case LIBUSB_ERROR_OVERFLOW:
            fprintf(stderr, "device sent too much data"); break;
        case LIBUSB_ERROR_NO_DEVICE:
            fprintf(stderr, "device has been disconnected"); break;
        default:
            fprintf(stderr, "error %d", code); break;
    }
    fprintf(stderr, "\n");
}

int bulk_read(int endpoint, unsigned char *buf, int length) {
    int ret, n;
    if((ret = libusb_bulk_transfer(device_handle,
                    LIBUSB_ENDPOINT_IN | endpoint,
                    buf, length, &n, TIMEOUT)) < 0)
        bulk_transfer_error(ret);
    return n;
}

int bulk_write(int endpoint, unsigned char *buf, int length) {
    int ret, n;
    if((ret = libusb_bulk_transfer(device_handle,
                    LIBUSB_ENDPOINT_OUT | endpoint,
                    buf, length, &n, TIMEOUT)) < 0)
        bulk_transfer_error(ret);
    if(n) fprintf(stderr, "Sent %d bytes\n", n);
    return n;
}

char *build_config(const char *mode, int resX, int resY, int w, int h) {
    char *data = calloc(255, sizeof(char));
    snprintf(data, 255,
            "R=%d,%d\nM=%s\nC=NONE\nB=100\nN=100\nU=OFF\nA=0,0,%d,%d\n",
            resX, resY, mode, w, h);
    return data;
}

void send_config(const char *data) {
    if(*data) fprintf(stderr, "Sending configuration data:\n%s", data);
    unsigned char buf[255];
    int length = snprintf(buf, 255, "\x1bX\n%s\x80", data);
    bulk_write(3, buf, length);
}

int main(int argc, char **argv) {
    const char *mode = "CGRAY";
    int resolution = 100;
    if(argc > 1) {
        switch(*argv[1]) {
            case 'c': mode = "CGRAY";  break;
            case 'g': mode = "GRAY64"; break;
            case 't': mode = "TEXT";   break;
            case '-': mode = NULL;     break;
            default:
                fprintf(stderr, "ERROR: unrecognised mode, "
                                "should be one of [cgt]\n");
                return 1;
        }
    }
    if(argc > 2) {
        resolution = atoi(argv[2]);
        if(resolution < 100 || resolution > 600 || resolution % 100) {
            fprintf(stderr, "ERROR: resolution must be a positive "
                            "multiple of 100 no greater than 600\n");
            return 1;
        }
    }

    libusb_context *context;
    libusb_init(&context);
    device_handle = libusb_open_device_with_vid_pid(context, VID, PID);
    if(!device_handle) {
        fprintf(stderr, "ERROR: Unable to find device "
                        "(Vendor ID = %04x, Product ID = %04x)\n",
                VID, PID);
        return 1;
    }
    libusb_claim_interface(device_handle, 0);
    libusb_set_interface_alt_setting(device_handle, 0, 0);

    control_in_vendor_device(1, 2, 0, 255); /* returns 05 10 01 02 00 */
    if(mode) {
        char *config = build_config(
                mode, MIN(resolution, 300), resolution,
                PAGE_WIDTH * MIN(resolution, 300)/100,
                PAGE_HEIGHT * resolution/100);
        send_config(config); free(config);
    }
    int page = 1;
    FILE *fp = NULL;
    int sleep_time = 0;
    while(1) {
        unsigned char buf[0x1000];
        int num_bytes = bulk_read(4, buf, 0x1000);
        if(num_bytes) sleep_time = 0;
        if(num_bytes > 2) {
            if(!fp) {
                char fname[8];
                snprintf(fname, 8, "%03d.raw", page);
                fprintf(stderr, "Opening '%s'...\n", fname);
                fp = fopen(fname, "wb");
            }
            fwrite(buf, 1, num_bytes, fp);
        } else if(num_bytes == 0 && sleep_time < 10) {
#ifdef DEBUG
            fprintf(stderr, "Sleeping\n");
#endif
            sleep_time++;
            usleep(200 * 1000);
        } else if(num_bytes == 2 && buf[0] == 0xc2 && buf[1] == 0x00) {
            fprintf(stderr, "ERROR: Nothing to scan\n"); break;
        } else if(num_bytes == 2 && buf[0] == 0xc3 && buf[1] == 0x00) {
            fprintf(stderr, "ERROR: Paper jam\n"); break;
        } else if(num_bytes == 1 && buf[0] == 0x80) {
            fprintf(stderr, "No more pages\n"); break;
        } else if((num_bytes == 1 && buf[0] == 0x81) || sleep_time >= 10) {
            fprintf(stderr, "Feeding in another page");
            if(sleep_time >= 10) fprintf(stderr, " (timeout)");
            fprintf(stderr, "\n");
            fclose(fp); fp = NULL;
            send_config("");
            page++;
            sleep_time = 0;
        } else if(num_bytes == 1 && buf[0] == 0xc3) {
            fprintf(stderr, "Paper jam\n"); break;
        } else if(num_bytes == 1 && buf[0] == 0xc4) {
            fprintf(stderr, "Scan aborted\n"); break;
        } else {
            fprintf(stderr, "Received unknown data: %02x", buf[0]);
            if(num_bytes == 2) fprintf(stderr, " %02x", buf[1]);
            fprintf(stderr, "\n");
            break;
        }
    }
    if(fp) fclose(fp);
    control_in_vendor_device(2, 2, 0, 255); /* returns 05 10 02 02 00 */

    libusb_release_interface(device_handle, 0);
    libusb_close(device_handle);
    libusb_exit(context);
    return 0;
}
