#include <stdio.h>
#include <stdlib.h>

#include <gmp.h>
#include <mpfr.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "image_gen.h"
#include "my_utils.h"

#include <threads.h>

#include <sys/time.h>
#include <time.h>

#define MY_PRECISION 665
#define THREAD_NUMBER 8

// some documentation
int make_image(int x_pixels, int y_pixels, int thread_count, long int precision,
               mpfr_t left, mpfr_t top, mpfr_t width,
               unsigned char *image_data) {

  // calculate nudge size
  mpfr_t nudge;
  mpfr_init2(nudge, MY_PRECISION);

  mpfr_div_si(nudge, width, x_pixels,
              MPFR_RNDD); // calculate the width of each pixel

  int index;

  // Populate y values
  mpfr_t y_val;
  mpfr_t y_values[y_pixels];
  mpfr_init2(y_val, MY_PRECISION);
  mpfr_set(y_val, top, MPFR_RNDD);
  for (index = y_pixels - 1; index >= 0; index--) {
    mpfr_init2(y_values[index], MY_PRECISION);
    mpfr_set(y_values[index], y_val, MPFR_RNDD);
    mpfr_sub(y_val, y_val, nudge, MPFR_RNDD);
  }

  // Populate x values
  mpfr_t x_val;
  mpfr_t x_values[x_pixels];
  mpfr_init2(x_val, MY_PRECISION);
  mpfr_set(x_val, left, MPFR_RNDD);
  for (index = 0; index < x_pixels; index++) {
    mpfr_init2(x_values[index], MY_PRECISION);
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
  fprintf(stdout, "Mutex (for image) successfully initialised at line %d\n",
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
  Thrd_awrp wrapper_arg[THREAD_NUMBER];
  char *thread_int_str[20];
  char *thread_name_str[THREAD_NUMBER][20];

  for (thread_index = 0; thread_index < THREAD_NUMBER; thread_index++) {
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
      printf("Successfully started thread %d.\nWrapper arg thread index is",
             thread_index);
    }
  }

  for (thread_index = 0; thread_index < THREAD_NUMBER; thread_index++) {
    thrd_join(threads[thread_index], NULL);
    printf("Thread %d joined.\n", thread_index);
  }

  mtx_destroy(&mutex);

  return 0;
}

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
