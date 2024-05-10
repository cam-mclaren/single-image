#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <gmp.h>
#include <mpfr.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

#include "image_gen.h"
#include "log.h"
#include "server.h"
// #include "my_utils.h"

#include <unistd.h>

#include <threads.h>

#include <sys/time.h>
#include <time.h>

#define MY_PRECISION 665
#define THREAD_NUMBER 8
#define DEBUG_MAC printf("line: %d\n", __LINE__);

int main(int argc, char **argv) {

  // Wall clock time
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  // Times main()
  clock_t begin = clock();
  DEBUG_MAC
  set_log_level(DEBUG);
  // put ':' in the starting of the
  // string so that program can
  // distinguish between '?' and ':'
  int opt;
  bool is_server = false;
  int array_len = 70;
  char x_arg[array_len];
  long unsigned int x_arg_len;
  char y_arg[array_len];
  long unsigned int y_arg_len;
  char width_arg[array_len];
  long unsigned int width_arg_len;
  while ((opt = getopt(argc, argv, ":sx:y:w:")) != -1) {
    switch (opt) {
    case 's':
      loggf(INFO, "Command line option : %c\n", opt);
      is_server = true;
      loggf(INFO, "Running in server mode\n");
      break;
    case 'x':
      loggf(INFO, "x argument : %s\n", optarg);
      if (check_and_copy_input(optarg, x_arg, array_len, "x_arg") != 0) {
        return 208;
      }
      break;
    case 'y':
      loggf(INFO, "y argument : %s\n", optarg);
      if (check_and_copy_input(optarg, y_arg, array_len, "y_arg") != 0) {
        return 208;
      }
      break;
    case 'w':
      loggf(INFO, "width argument : %s\n", optarg);
      if (check_and_copy_input(optarg, width_arg, array_len, "width_arg") !=
          0) {
        return 208;
      }
      break;
    case ':':
      loggf(INFO, "option needs a value\n");
      break;
    case '?':
      loggf(INFO, "unknown option : %c\n", optopt);
      break;
    }
  }

  // optind is for the extra arguments
  // which are not parsed
  for (; optind < argc; optind++) {
    loggf(INFO, "extra arguments : %s\n", argv[optind]);
  }

  // resolution
  int x_pixels = 1280;
  int y_pixels = 720;

  // define image_data array

  if (is_server) {
    start_server(&array_len);
  } else {

    mpfr_t top, left, width;
    mpfr_init2(top, MY_PRECISION);
    mpfr_init2(left, MY_PRECISION);
    mpfr_init2(width, MY_PRECISION);

    mpfr_strtofr(left, x_arg, NULL, 10, MPFR_RNDD);
    mpfr_strtofr(top, y_arg, NULL, 10, MPFR_RNDD);
    mpfr_strtofr(width, width_arg, NULL, 10, MPFR_RNDD);

    unsigned char *image_data =
        calloc(y_pixels * x_pixels * 3, sizeof(unsigned char));
    make_image(x_pixels, y_pixels, THREAD_NUMBER, MY_PRECISION, left, top,
               width, image_data);
    stbi_write_jpg("image.jpg", x_pixels, y_pixels, 3, image_data, 100);
    free((void *)image_data);
    mpfr_clear(left);
    mpfr_clear(top);
    mpfr_clear(width);
  }

  mpfr_free_cache();
  clock_t end = clock();

  double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  loggf(INFO, "%s(): CPU time used was %lf seconds\n", __func__, time_spent);

  struct timeval tv2;
  struct timezone tz2;
  gettimeofday(&tv2, &tz2);

  loggf(INFO, "%s(): Wall clock time elapsed is %lf seconds.\n", __func__,
        (double)(tv2.tv_sec - tv.tv_sec) +
            (double)(tv2.tv_usec - tv.tv_usec) / 1000000);
  return 0;
}
