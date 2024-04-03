#ifndef IMAGE_GEN_H
#define IMAGE_GEN_H

#include "my_utils.h"
#include <gmp.h>
#include <mpfr.h>
#include <threads.h>
// Context for the threads
typedef struct image_params {
  int y_pixels;
  int x_pixels;
  int *row_count;
  unsigned char *image_data;
  mpfr_t *x_array;
  mpfr_t *y_array;
  mtx_t *mutex_ptr;
  double *red_coefficients;
  double *green_coefficients;
  double *blue_coefficients;
  double *nodes;
  long int precision;
} Image_params;

// This is a wrapper around the arguments common to all threads.
// This allows the threads to identify themselves when logging their progress.
typedef struct thrd_awrp {
  char thread_name[20];
  int thread_index;
  Image_params *args;
} Thrd_awrp;

// Worker function spawned on each thread. Workfunction calls use a mutex locked
// integer to allocate which column in the image matrix they will be
// calculating. When the mutex locked integer reaches the number of columns in
// the image the threads begin to terminate.
int worker_function(void *wrapper_arg);

// The fuction spawns `thread_count` number of threads to caculate the escape
// times for the points on the complex plane implied by x_pixels, y_pixels,
// left, top and width. `image_data` has length x_pixels*y_pixels*3
int make_image(int x_pixels, int y_pixels, int thread_count, long int precision,
               mpfr_t left, mpfr_t top, mpfr_t width,
               unsigned char *image_data);

#endif
