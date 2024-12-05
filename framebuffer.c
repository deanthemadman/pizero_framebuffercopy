#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <string.h>
#include <errno.h>

#define FB0_PATH "/dev/fb0"
#define FB1_PATH "/dev/fb1"

int main() {
    int fb0 = open(FB0_PATH, O_RDONLY);
    int fb1 = open(FB1_PATH, O_WRONLY);

    if (fb0 == -1 || fb1 == -1) {
        perror("Error opening framebuffer devices");
        return 1;
    }

    struct fb_var_screeninfo vinfo0, vinfo1;
    
    // Get the variable information for fb0
    if (ioctl(fb0, FBIOGET_VSCREENINFO, &vinfo0) == -1) {
        perror("Error reading variable information from fb0");
        return 1;
    }

    // Get the variable information for fb1 (should be similar to fb0)
    if (ioctl(fb1, FBIOGET_VSCREENINFO, &vinfo1) == -1) {
        perror("Error reading variable information from fb1");
        return 1;
    }

    // Check if fb0 and fb1 resolutions and color depths are the same
    if (vinfo0.xres != vinfo1.xres || vinfo0.yres != vinfo1.yres || vinfo0.bits_per_pixel != vinfo1.bits_per_pixel) {
        fprintf(stderr, "Framebuffers have different resolutions or color depths\n");
        return 1;
    }

    // Calculate the size of the framebuffer (in bytes)
    int framebuffer_size = vinfo0.yres_virtual * vinfo0.xres_virtual * (vinfo0.bits_per_pixel / 8);

    // Allocate buffer to hold the framebuffer data
    unsigned char *buffer = (unsigned char *)malloc(framebuffer_size);
    if (buffer == NULL) {
        perror("Unable to allocate memory for framebuffer data");
        return 1;
    }

    // Read the framebuffer data from fb0
    if (read(fb0, buffer, framebuffer_size) != framebuffer_size) {
        perror("Error reading framebuffer data from fb0");
        free(buffer);
        return 1;
    }

    // Write the framebuffer data to fb1
    if (write(fb1, buffer, framebuffer_size) != framebuffer_size) {
        perror("Error writing framebuffer data to fb1");
        free(buffer);
        return 1;
    }

    // Cleanup
    free(buffer);
    close(fb0);
    close(fb1);

    printf("Framebuffer successfully copied from %s to %s\n", FB0_PATH, FB1_PATH);

    return 0;
}
