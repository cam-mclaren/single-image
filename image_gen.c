#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include <gmp.h>
#include <mpfr.h>

#include "image_gen.h"
#include "log.h"
#include "my_utils.h"

int check_and_copy_input(char *input, char *output, int size, char *var_name) {
  int arg_len = strlen(input);
  if (arg_len > size) {
    loggf(ERROR, "Error. %s is too long\n", var_name);
    return 208;
  }
  strcpy(output, input);
  zero_fill(arg_len, size, output);
  return 0;
}

// Worker function spawned on each thread. Workfunction calls use a mutex locked
// integer to allocate which column in the image matrix they will be
// calculating. When the mutex locked integer reaches the number of columns in
// the image the threads begin to terminate.
int worker_function(void *wrapper_arg /* void pointer to pointer to struct*/) {
  Thrd_awrp thread_argument_wrapper = *(Thrd_awrp *)wrapper_arg;
  Image_params *args = thread_argument_wrapper.args;
  loggf(INFO, "Thread %d is active\n", thread_argument_wrapper.thread_index);

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
      loggf(DEBUG, "Worker function is returning\n");
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

      // mpfr_fprintf(stdout, "x value: %5.50Rf\n", args->x_array[j]);
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

// The fuction spawns `thread_count` number of threads to caculate the escape
// times for the points on the complex plane implied by x_pixels, y_pixels,
// left, top and width. `image_data` has length x_pixels*y_pixels*3
int make_image(int x_pixels, int y_pixels, int thread_count, long int precision,
               mpfr_t left, mpfr_t top, mpfr_t width,
               unsigned char *image_data) {

  // calculate nudge size
  mpfr_t nudge;
  mpfr_init2(nudge, precision);

  mpfr_div_si(nudge, width, x_pixels,
              MPFR_RNDD); // calculate the width of each pixel

  int index;

  // Populate y values
  mpfr_t y_val;
  mpfr_t y_values[y_pixels];
  mpfr_init2(y_val, precision);
  mpfr_set(y_val, top, MPFR_RNDD);
  for (index = y_pixels - 1; index >= 0; index--) {
    mpfr_init2(y_values[index], precision);
    mpfr_set(y_values[index], y_val, MPFR_RNDD);
    mpfr_sub(y_val, y_val, nudge, MPFR_RNDD);
  }

  // Populate x values
  mpfr_t x_val;
  mpfr_t x_values[x_pixels];
  mpfr_init2(x_val, precision);
  mpfr_set(x_val, left, MPFR_RNDD);
  for (index = 0; index < x_pixels; index++) {
    mpfr_init2(x_values[index], precision);
    mpfr_set(x_values[index], x_val, MPFR_RNDD);
    mpfr_add(x_val, x_val, nudge, MPFR_RNDD);
  }

  mpfr_clear(nudge);
  mpfr_clear(y_val);
  mpfr_clear(x_val);

  // declare and initialise mutex
  mtx_t mutex;
  if (mtx_init(&mutex, mtx_plain) != thrd_success) {
    fprintf(stderr, "Error initialising mutex (for image) on line %d\n",
            __LINE__);
    return -1;
  }
  loggf(DEBUG, "Mutex (for image) successfully initialised at line %d\n",
        __LINE__);

  thrd_t threads[thread_count];

  // define integer representing number of rows in image processed. Cast this as
  // a void pointer.
  int rows_processed = 0;
  void *thread_argument = (void *)&rows_processed;

  // Load the coefficients and nodes for the colouring spline//
  int coefficent_number = count_doubles_in_file(
      "./interpolation/colour_parameters/blue_coefficients");
  double *red_coefficients = malloc(coefficent_number * sizeof(double));
  double *green_coefficients = malloc(coefficent_number * sizeof(double));
  double *blue_coefficients = malloc(coefficent_number * sizeof(double));
  double *nodes = malloc(coefficent_number * sizeof(double));

  fill_double_array_from_file(
      "./interpolation/colour_parameters/red_coefficients", coefficent_number,
      red_coefficients);
  fill_double_array_from_file(
      "./interpolation/colour_parameters/green_coefficients", coefficent_number,
      green_coefficients);
  fill_double_array_from_file(
      "./interpolation/colour_parameters/blue_coefficients", coefficent_number,
      blue_coefficients);
  fill_double_array_from_file("./interpolation/colour_parameters/nodes",
                              coefficent_number, nodes);

  struct image_params
      thread_parameters; // thread_parameters is basically how the main function
                         // can interface with the worker_function. Most
                         // information passed into the structure should only be
                         // read. However row_count, mutex_ptr, and image_data
                         // will have write operations performed on them.
  thread_parameters.x_pixels = x_pixels;
  thread_parameters.y_pixels = y_pixels;
  thread_parameters.row_count =
      &rows_processed; // Threads will increment this to let others know row is
                       // complete
  thread_parameters.mutex_ptr =
      &mutex; // Makes rows_processed increment thread safe
  thread_parameters.image_data = image_data;
  thread_parameters.y_array = y_values;
  thread_parameters.x_array = x_values;
  thread_parameters.red_coefficients =
      red_coefficients; // Possibly I should consider this being put into a
                        // substruct.
  thread_parameters.green_coefficients = green_coefficients;
  thread_parameters.blue_coefficients = blue_coefficients;
  thread_parameters.nodes = nodes;
  thread_parameters.precision = precision;

  int thread_index;
  Thrd_awrp wrapper_arg[thread_count];
  char *thread_int_str[20];
  char *thread_name_str[thread_count][20];

  for (thread_index = 0; thread_index < thread_count; thread_index++) {
    sprintf((char *)thread_name_str[thread_index], "Thread_%d", thread_index);
    wrapper_arg[thread_index].thread_index = thread_index;
    strcpy(wrapper_arg[thread_index].thread_name,
           (char *)thread_name_str[thread_index]);
    wrapper_arg[thread_index].args = &thread_parameters;

    if (thrd_create(&threads[thread_index], worker_function,
                    (void *)&wrapper_arg[thread_index]) != thrd_success) {
      fprintf(stderr, "Error creating thread %d.\n", thread_index);
      return -99;
    } else {
      loggf(DEBUG, "Successfully started thread %d.\n", thread_index);
    }
  }

  for (thread_index = 0; thread_index < thread_count; thread_index++) {
    thrd_join(threads[thread_index], NULL);
    loggf(DEBUG, "Thread %d joined.\n", thread_index);
  }

  mtx_destroy(&mutex);

  return 0;
}
