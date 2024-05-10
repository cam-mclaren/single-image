#include "server.h"
#include "image_gen.h"
#include "log.h"
#include <microhttpd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MY_PRECISION 665
#define THREAD_NUMBER 8
#define PORT 8888

#include "stb_image.h"
#include "stb_image_write.h"

struct server_data {
  bool busy_status;
};

static thrd_t image_request_thread;

int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key,
                  const char *value);
int answer_to_connection(void *cls, struct MHD_Connection *connection,
                         const char *url, const char *method,
                         const char *version, const char *upload_data,
                         unsigned long *upload_data_size, void **con_cls) {
  struct MHD_Response *response;
  int ret;
  const char *response_str = "Endpoint hit";
  loggf(INFO, "%s\n", (char *)cls);
  // Print to the console which endpoint and method was accessed
  if (strcmp(url, "/image") == 0) {
    if (strcmp(method, "GET") == 0) {

      // check if server is currently making an image
      struct server_data *serverData = (struct server_data *)cls;
      if (serverData->busy_status == true) {
      } else {
        serverData->busy_status = true;
      }

      loggf(DEBUG, "url sent was /image\n");

      // Read arguments from http request
      const char *x_value =
          MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "x");
      loggf(DEBUG, "%s: x_value: %s\n", __func__, x_value);
      const char *y_value =
          MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "y");
      loggf(DEBUG, "%s: y_value: %s\n", __func__, y_value);
      const char *width_value = MHD_lookup_connection_value(
          connection, MHD_GET_ARGUMENT_KIND, "width");
      loggf(DEBUG, "%s: width_value: %s\n", __func__, width_value);

      // temporary declaration/defintion of the length of supplied arguments.
      // Should eventually be supplied in the request
      int size = 70;
      char x[size];
      char y[size];
      char w[size];
      check_and_copy_input((char *)x_value, x, size, "x");
      check_and_copy_input((char *)y_value, y, size, "y");
      check_and_copy_input((char *)width_value, w, size, "width");

      // resolution
      // Should eventually be supplied in the request
      int x_pixels = 1280;
      int y_pixels = 720;

      mpfr_t top, left, width;
      mpfr_init2(top, MY_PRECISION);
      mpfr_init2(left, MY_PRECISION);
      mpfr_init2(width, MY_PRECISION);

      // Temporary debug statements
      mpfr_strtofr(left, x, NULL, 10, MPFR_RNDD);
      mpfr_strtofr(top, y, NULL, 10, MPFR_RNDD);
      mpfr_strtofr(width, w, NULL, 10, MPFR_RNDD);

      unsigned char *image_data =
          calloc(y_pixels * x_pixels * 3, sizeof(unsigned char));
      make_image(x_pixels, y_pixels, THREAD_NUMBER, MY_PRECISION, left, top,
                 width, image_data);
      stbi_write_jpg("image.jpg", x_pixels, y_pixels, 3, image_data, 100);
    }
  } else {
    loggf(WARN, "Unhandled method url combination\n");
    loggf(DEBUG, "Accessed %s with method %s\n", url, method);
    MHD_get_connection_values(connection, MHD_HEADER_KIND, &print_out_key,
                              NULL);
  }

  // Prepare a generic response
  response = MHD_create_response_from_buffer(
      strlen(response_str), (void *)response_str, MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);

  return ret;
}

int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key,
                  const char *value) {
  loggf(DEBUG, "%s: %s\n", key, value);
  return MHD_YES;
}

int start_server(int *arg_length) {

  //  Establish a pipe here somewhere.
  //  This pipe needs to be used by whatever calls make image
  //  What does the server do when it gets an image request? It starts a new
  //  thread that calls a function
  struct MHD_Daemon *daemon;

  int mypipe[2];
  if (pipe(mypipe) == -1) {
    loggf(ERROR, "%s: pipe() failed\n", __func__);
    return 1;
  }

  struct server_data serverData;
  serverData.busy_status = false;

  daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                            &answer_to_connection, (void *)&serverData,
                            MHD_OPTION_END);
  if (daemon == NULL) {
    return 1;
  }

  loggf(INFO, "%s: Server started on port %d\n", __func__, PORT);

  // To keep the server running until the /shutdown is received
  bool exit = false;
  while (!exit) {
    getchar(); // TODO : Replace with epoll and read switch statement
    exit = true;
  }

  MHD_stop_daemon(daemon);
  loggf(INFO, "%s: MHD_stop_daemon called\n", __func__);
  return 0;
}

struct server_irs {
  int pipe_write;
  int x_pixels;
  int y_pixels;
  int thread_count;
  long int precision;
  mpfr_t left;
  mpfr_t top;
  mpfr_t width;
  unsigned char *image_data;
};

int image_request(void *image_request_struct) {
  // Will need to take in a pipe fd descriptor that can be used to talk to
  // start_server (which will be called on the main thread
  //
  struct server_irs *args = (struct server_irs *)image_request_struct;

  loggf(DEBUG, "%s(): is calling make_image()\n", __func__);
  make_image(args->x_pixels, args->y_pixels, args->thread_count,
             args->precision, args->left, args->top, args->width,
             args->image_data);
  loggf(DEBUG, "%s(): make_image() returned\n", __func__);
  return 0;
}

// function wait and read input on the pipe?
