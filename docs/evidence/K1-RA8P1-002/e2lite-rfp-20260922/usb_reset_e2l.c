#include <stdio.h>
#include <libusb.h>

/* Software stand-in for unplugging the E2 Lite. Vendor 0x045B product 0x82A0. */
int main(void) {
    libusb_context *ctx = NULL;
    libusb_device_handle *handle;
    int rc = libusb_init(&ctx);
    if (rc) {
        printf("init %d\n", rc);
        return 1;
    }
    handle = libusb_open_device_with_vid_pid(ctx, 0x045b, 0x82a0);
    if (!handle) {
        printf("open_fail\n");
        libusb_exit(ctx);
        return 2;
    }
    rc = libusb_reset_device(handle);
    printf("reset %d\n", rc);
    libusb_close(handle);
    libusb_exit(ctx);
    return rc ? 3 : 0;
}
