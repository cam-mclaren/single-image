#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <threads.h>

#include <gmp.h>
#include <mpfr.h>

#include "image_gen.h"
#include "my_utils.h"

// Worker function spawned on each thread. Workfunction calls use a mutex locked
// integer to allocate which column in the image matrix they will be
// calculating. When the mutex locked integer reaches the number of columns in
// the image the threads begin to terminate.
int worker_function(void *wrapper_arg /* void pointer to pointer to struct*/) {
  Thrd_awrp thread_argument_wrapper = *(Thrd_awrp *)wrapper_arg;
  Image_params *args = thread_argument_wrapper.args;
  printf("Thread %d is active\n", thread_argument_wrapper.thread_index);

  // Declare local copies of necessary variables.
  int current_row;
  mpfr_t current_y, current_x;
  int r, g, b;
  double speed, inner_product;
  int max_iter = 4000;
  int iter_count = 0;
  mpfr_t real_component, imaginary_component, x_square, y_square, real_temp;
  mpfr_t c_x, c_y;
  mpfr_init2(real_component, args->precision);
  mpfr_init2(imaginary_component, args->precision);
  mpfr_init2(real_temp, args->precision);
  mpfr_init2(x_square, args->precision);
  mpfr_init2(y_square, args->precision);
  mpfr_init2(current_x, args->precision);
  mpfr_init2(current_y, args->precision);

  int count = 0;
  bool success = false;
  while (count < args->y_pixels) {

    mtx_lock(args->mutex_ptr);
    current_row = *(args->row_count); // read current row
    if (current_row >= args->y_pixels) {
      printf("Worker function is returning\n");
      mtx_unlock(args->mutex_ptr);
      success = true;
      break; // Exit While loop and return
    } else {
      *(args->row_count) = current_row + 1;
      // printf("incrementing\n");
    }
    mtx_unlock(args->mutex_ptr);

    mpfr_set(current_y, args->y_array[current_row], MPFR_RNDD);

    // mpfr_fprintf(stdout,"%5.50Rf\n",current_y);
    int j;
    for (j = 0; j < args->x_pixels; j++) {

      mpfr_fprintf(stdout, "x value: %5.50Rf\n", args->x_array[j]);
      mpfr_set(current_x, args->x_array[j], MPFR_RNDD);
      mpfr_set(real_component, current_x, MPFR_RNDD); //  Initialise x value
      mpfr_set(imaginary_component, current_y, MPFR_RNDD); // Initialise y value
      // run time escape algorithm
      inner_product = 0;
      iter_count = 0;
      while ((iter_count < max_iter) && (inner_product < 4)) {
        // f(z) = z^2 + c
        // (x+yi)(x+yi) = x^2 - y^2 + (2xy)i
        // Real component calculation store in real_temp
        mpfr_pow_si(x_square, real_component, 2, MPFR_RNDD);
        mpfr_pow_si(y_square, imaginary_component, 2, MPFR_RNDD);
        mpfr_sub(real_temp, x_square, y_square, MPFR_RNDD);
        // imaginary_component
        mpfr_mul(imaginary_component, imaginary_component, real_component,
                 MPFR_RNDD);
        mpfr_mul_si(imaginary_component, imaginary_component, 2,
                    MPFR_RNDD); // imaginary component calculate
        // add c
        mpfr_add(real_component, real_temp, current_x, MPFR_RNDD);
        mpfr_add(imaginary_component, imaginary_component, current_y,
                 MPFR_RNDD);
        // find the inner product
        mpfr_pow_si(x_square, real_component, 2, MPFR_RNDD);
        mpfr_pow_si(y_square, imaginary_component, 2, MPFR_RNDD);
        mpfr_add(real_temp, x_square, y_square, MPFR_RNDD);
        inner_product = mpfr_get_d(real_temp, MPFR_RNDD);

        iter_count++;
      }
      speed = (double)iter_count / max_iter;
      if (speed == 1) {
        // Does nothing. Pixel is initialised as black.
        // This is because calloc is zero initialised. (RGB 000 000 000 is
        // black)
      } else {
        r = floor(interp_poly(args->red_coefficients, args->nodes, speed));
        g = floor(interp_poly(args->green_coefficients, args->nodes, speed));
        b = floor(interp_poly(args->blue_coefficients, args->nodes, speed));

        if (r > 255) {
          r = 255;
        }
        if (g > 255) {
          g = 255;
        }
        if (b > 255) {
          b = 255;
        }
        if (r < 0) {
          r = 0;
        }
        if (g < 0) {
          g = 0;
        }
        if (b < 0) {
          b = 0;
        }

        *(args->image_data +
          (args->y_pixels - 1 - current_row) * args->x_pixels * 3 + 3 * j + 0) =
            (unsigned char)r;
        *(args->image_data +
          (args->y_pixels - 1 - current_row) * args->x_pixels * 3 + 3 * j + 1) =
            (unsigned char)g;
        *(args->image_data +
          (args->y_pixels - 1 - current_row) * args->x_pixels * 3 + 3 * j + 2) =
            (unsigned char)b;
      }
    }
    count++;
  }

  mpfr_clear(real_component);
  mpfr_clear(imaginary_component);
  mpfr_clear(real_temp);
  mpfr_clear(x_square);
  mpfr_clear(y_square);
  mpfr_clear(current_x);
  mpfr_clear(current_y);

  if (success) {
    return 0;
  } else {
    fprintf(stderr, "Thread function loop maxed out");
    return -99;
  }
}
