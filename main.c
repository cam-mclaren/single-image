#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <gmp.h>
#include <mpfr.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "image_gen.h"
// #include "my_utils.h"

#include <unistd.h>

#include <threads.h>

#include <sys/time.h>
#include <time.h>

#define MY_PRECISION 665
#define THREAD_NUMBER 8

int main(int argc, char **argv) {

  // Wall clock time
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  // Times main()
  clock_t begin = clock();

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
      printf("option : %c\n", opt);
      is_server = true;
      printf("Running in server mode\n");
      break;
    case 'x':
      printf("x argument : %s\n", optarg);
      x_arg_len = strlen(optarg);
      if (x_arg_len > 59) {
        printf("Error. x_arg is too long\n");
        return 208;
      } else {
        strcpy(x_arg, optarg);
        zero_fill(x_arg_len, array_len, x_arg);
      }
      break;
    case 'y':
      printf("y argument : %s\n", optarg);
      y_arg_len = strlen(optarg);
      if (y_arg_len > 59) {
        printf("Error. y_arg is too long\n");
        return 208;
      } else {
        strcpy(y_arg, optarg);
        zero_fill(y_arg_len, array_len, y_arg);
      }
      break;
    case 'w':
      printf("width argument : %s\n", optarg);
      width_arg_len = strlen(optarg);
      if (width_arg_len > 59) {
        printf("Error. width_arg is too long\n");
        return 208;
      } else {
        strcpy(width_arg, optarg);
        zero_fill(width_arg_len, array_len, width_arg);
      }
      break;
    case ':':
      printf("option needs a value\n");
      break;
    case '?':
      printf("unknown option : %c\n", optopt);
      break;
    }
  }

  // optind is for the extra arguments
  // which are not parsed
  for (; optind < argc; optind++) {
    printf("extra arguments : %s\n", argv[optind]);
  }

  printf("%s\n%s\n%s\n", x_arg, y_arg, width_arg);

  mpfr_t top, left, width;
  mpfr_init2(top, MY_PRECISION);
  mpfr_init2(left, MY_PRECISION);
  mpfr_init2(width, MY_PRECISION);

  mpfr_strtofr(left, x_arg, NULL, 10, MPFR_RNDD);
  mpfr_strtofr(top, y_arg, NULL, 10, MPFR_RNDD);
  mpfr_strtofr(width, width_arg, NULL, 10, MPFR_RNDD);

  // resolution
  int x_pixels = 1280;
  int y_pixels = 720;

  // define image_data array
  unsigned char *image_data =
      calloc(y_pixels * x_pixels * 3, sizeof(unsigned char));

  make_image(x_pixels, y_pixels, THREAD_NUMBER, MY_PRECISION, left, top, width,
             image_data);

  stbi_write_jpg("image.jpg", x_pixels, y_pixels, 3, image_data, 100);

  free((void *)image_data);
  mpfr_clear(left);
  mpfr_clear(top);
  mpfr_clear(width);
  mpfr_free_cache();

  clock_t end = clock();

  double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Main() CPU time used was %lf seconds\n", time_spent);

  struct timeval tv2;
  struct timezone tz2;
  gettimeofday(&tv2, &tz2);

  printf("Wall clock time elapsed is %lf seconds.\n",
         (double)(tv2.tv_sec - tv.tv_sec) +
             (double)(tv2.tv_usec - tv.tv_usec) / 1000000);
  return 0;
}
