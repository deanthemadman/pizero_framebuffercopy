#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#define FB_PATH "/dev/fb1"
#define SPI_DEV "/dev/spidev0.0"

// Define GPIO pins for the ST7789VW
#define DC_PIN 24  // Data/Command Pin (GPIO 24)
#define RESET_PIN 25  // Reset Pin (GPIO 25)
#define CS_PIN 8  // Chip Select Pin (GPIO 8)

// SPI speed and other parameters
#define SPI_SPEED_HZ 32000000  // 32 MHz, you can adjust this
#define SPI_MODE SPI_MODE_0  // SPI Mode 0 (CPOL=0, CPHA=0)
#define SPI_BITS_PER_WORD 8  // 8 bits per word (1 byte per transfer)

int spi_fd;
unsigned char *fb_buffer;
int fb_size = 0;

int open_spi_device() {
    spi_fd = open(SPI_DEV, O_RDWR);
    if (spi_fd < 0) {
        perror("Error opening SPI device");
        return -1;
    }

    // Set SPI mode
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &SPI_MODE) == -1) {
        perror("Error setting SPI mode");
        return -1;
    }

    // Set SPI bits per word
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &SPI_BITS_PER_WORD) == -1) {
        perror("Error setting bits per word");
        return -1;
    }

    // Set SPI speed (frequency)
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &SPI_SPEED_HZ) == -1) {
        perror("Error setting SPI speed");
        return -1;
    }

    return 0;
}

void close_spi_device() {
    close(spi_fd);
}

int open_framebuffer() {
    int fb_fd = open(FB_PATH, O_RDONLY);
    if (fb_fd == -1) {
        perror("Error opening framebuffer");
        return -1;
    }

    // Get framebuffer information (resolution, color depth, etc.)
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading framebuffer info");
        return -1;
    }

    fb_size = vinfo.yres_virtual * vinfo.xres_virtual * (vinfo.bits_per_pixel / 8);
    fb_buffer = (unsigned char *)malloc(fb_size);

    if (fb_buffer == NULL) {
        perror("Error allocating memory for framebuffer");
        return -1;
    }

    // Read framebuffer data
    if (read(fb_fd, fb_buffer, fb_size) != fb_size) {
        perror("Error reading framebuffer data");
        return -1;
    }

    close(fb_fd);
    return 0;
}

void send_spi_data(unsigned char *data, int len) {
    struct spi_ioc_transfer transfer = {
        .tx_buf = (unsigned long)data,
        .len = len,
        .speed_hz = SPI_SPEED_HZ,
        .bits_per_word = SPI_BITS_PER_WORD,
        .cs_change = 0,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer) < 0) {
        perror("Error sending SPI data");
    }
}

void reset_display() {
    // Reset the display by toggling the RESET pin (GPIO)
    // Implement the GPIO manipulation here to reset the display
    // This will depend on your specific GPIO library (e.g., wiringPi or bcm2835)
}

void initialize_display() {
    reset_display();

    // Send initialization commands to the display via SPI (ST7789 initialization commands)
    unsigned char init_cmds[] = {
        0x01,  // Software reset
        0x11,  // Sleep out
        0x29,  // Display on
    };

    // Send the initialization commands over SPI
    for (int i = 0; i < sizeof(init_cmds); i++) {
        unsigned char cmd = init_cmds[i];
        send_spi_data(&cmd, 1);
        usleep(10000);  // Small delay between commands
    }

    // Set rotation to 180 degrees (MADCTL register)
    unsigned char rotation_cmd[] = {0x36, 0xC0}; // 0xC0 sets 180° rotation
    send_spi_data(rotation_cmd, sizeof(rotation_cmd));
}

void display_framebuffer() {
    // Send framebuffer data to the ST7789VW display via SPI
    // The framebuffer data format (RGB565 or RGB888) should match the display's expected format

    // In this case, assuming framebuffer is in RGB888 format and display uses RGB565, conversion will be needed
    // If the framebuffer is already in RGB565, you can skip this step
    for (int i = 0; i < fb_size; i += 3) {
        unsigned char r = fb_buffer[i];
        unsigned char g = fb_buffer[i+1];
        unsigned char b = fb_buffer[i+2];

        // Convert RGB888 to RGB565
        unsigned short rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

        // Send the converted data to the display via SPI
        send_spi_data((unsigned char *)&rgb565, 2);
    }
}

int main() {
    // Open SPI and framebuffer
    if (open_spi_device() < 0) {
        return 1;
    }

    if (open_framebuffer() < 0) {
        close_spi_device();
        return 1;
    }

    // Initialize the display
    initialize_display();

    // Display the framebuffer data
    display_framebuffer();

    // Clean up
    free(fb_buffer);
    close_spi_device();

    return 0;
}
