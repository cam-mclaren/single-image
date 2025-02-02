#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <gmp.h>
#include <mpfr.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb_image_write.h"
#endif

#include "image_gen.h"
#include "log.h"
#include "server.h"
// #include "my_utils.h"

#include <unistd.h>

#include <threads.h>

#include <sys/time.h>
#include <time.h>

#define MY_PRECISION 500
#define THREAD_NUMBER 4
#define DEBUG_MAC printf("line: %d\n", __LINE__);

int main(int argc, char **argv) {

  init_logger();

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
  int image_file_path_len = 100;
  char *image_file_path = (char *)malloc(image_file_path_len * sizeof(char));
  int opt;
  bool is_server = false;
  bool run_local = false;
  int array_len = 70;
  char x_arg[array_len]; memset(x_arg, 0, array_len);
  char y_arg[array_len]; memset(y_arg, 0, array_len);
  char width_arg[array_len]; memset(width_arg, 0, array_len); 
  while ((opt = getopt(argc, argv, ":sx:y:w:f:")) != -1) {
    switch (opt) {
    case 's':
      loggf(INFO, "Command line option : %c\n", opt);
      is_server = true;
      loggf(INFO, "Running in server mode\n");
      break;
    case 'x':
      run_local = true;
      loggf(INFO, "x argument : %s\n", optarg);
      if (check_and_copy_input(optarg, x_arg, array_len, "x_arg") != 0) {
        return 208;
      }
      break;
    case 'y':
      run_local = true;
      loggf(INFO, "y argument : %s\n", optarg);
      if (check_and_copy_input(optarg, y_arg, array_len, "y_arg") != 0) {
        return 208;
      }
      break;
    case 'w':
      run_local = true;
      loggf(INFO, "width argument : %s\n", optarg);
      if (check_and_copy_input(optarg, width_arg, array_len, "width_arg") !=
          0) {
        return 208;
      }
      break;
    case 'f':
      run_local = true;
      loggf(INFO, "image file argument : %s\n", optarg);
      if (strlen(optarg) > image_file_path_len) {
        loggf(ERROR, "Image file path is too long\n");
        return 208;
      } else {
        strcpy(image_file_path, optarg);
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
  int x_pixels = 640; //1280;
  int y_pixels = 360; //720;


  // catch missing arguments in run_local workflow
  if (run_local) {
    char null_array[array_len]; memset(null_array, 0, array_len);
    if (memcmp(x_arg, null_array, array_len) == 0) {
      loggf(ERROR, "Missing x argument in run_local workflow\n");
      return 208;
    }
    if (memcmp(y_arg, null_array, array_len) == 0) {
      loggf(ERROR, "Missing y argument in run_local workflow\n");
      return 208;
    }
    if (memcmp(width_arg, null_array, array_len) == 0) {
      loggf(ERROR, "Missing width argument in run_local workflow\n");
      return 208;
    }
    char image_file_path_null_array[image_file_path_len]; memset(image_file_path_null_array, 0, image_file_path_len);
    if (memcmp(image_file_path, image_file_path_null_array, image_file_path_len) == 0) {
      loggf(ERROR, "Missing image file argument in run_local workflow\n");
      return 208;
    }

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
    stbi_write_jpg(image_file_path, x_pixels, y_pixels, 3, image_data, 100);
    free((void *)image_data);
    mpfr_clear(left);
    mpfr_clear(top);
    mpfr_clear(width);
  } else if (is_server) {
    start_server(&array_len);
  } else {
    loggf(ERROR, "No arguments provided\n");
    return 208;
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
