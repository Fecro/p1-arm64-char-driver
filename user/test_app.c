#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/p1_char"
#define TEST_STRING "Hello ARM64 Kernel! Message sent from User Space."

int main(void) {
  int fd;
  ssize_t bytes_written, bytes_read;
  char read_buffer[256];

  printf("=== [USER SPACE TEST APP] ===\n");
  printf("Opening device: %s\n", DEVICE_PATH);

  fd = open(DEVICE_PATH, O_RDWR);
  if (fd < 0) {
    perror("Error opening device");
    return EXIT_FAILURE;
  }

  /* 1. Write Test */
  printf("Writing data to Kernel: '%s'\n", TEST_STRING);
  bytes_written = write(fd, TEST_STRING, strlen(TEST_STRING));
  if (bytes_written < 0) {
    perror("Error writing to device");
    close(fd);
    return EXIT_FAILURE;
  }
  printf("Successfully wrote: %zd bytes\n", bytes_written);

  /* 2. Read Test */
  memset(read_buffer, 0, sizeof(read_buffer));
  printf("Reading data stored in the Kernel...\n");
  bytes_read = read(fd, read_buffer, sizeof(read_buffer) - 1);
  if (bytes_read < 0) {
    perror("Error reading from device");
    close(fd);
    return EXIT_FAILURE;
  }

  printf("Successfully read: %zd bytes\n", bytes_read);
  printf("Content received from Kernel: \"%s\"\n", read_buffer);

  /* 3. Close the descriptor */
  close(fd);
  printf("Device closed. Test completed successfully.\n");

  return EXIT_SUCCESS;
}
