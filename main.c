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

  mpfr_t top, left, width;
  mpfr_init2(top, MY_PRECISION);
  mpfr_init2(left, MY_PRECISION);
  mpfr_init2(width, MY_PRECISION);
  int read_status;

  mpfr_strtofr(left, *(argv + 1), NULL, 10, MPFR_RNDD);
  mpfr_strtofr(top, *(argv + 2), NULL, 10, MPFR_RNDD);
  mpfr_strtofr(width, *(argv + 3), NULL, 10, MPFR_RNDD);

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
