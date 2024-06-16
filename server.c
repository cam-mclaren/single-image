#include "server.h"
#include "image_gen.h"
#include "log.h"
#include <microhttpd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#define MY_PRECISION 665
#define THREAD_NUMBER 8
#define PORT 8888

#include "stb_image.h"
#include "stb_image_write.h"

// client connection
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int image_request(void *);

int stream_data(unsigned char *image_data, size_t image_data_size,
                char *server_string, int server_port);

static thrd_t image_request_thread;

static bool server_is_busy = false;

struct server_irs {
  FILE *server_coms_write;
  int x_pixels;
  int y_pixels;
  int thread_count;
  long int precision;
  mpfr_t left;
  mpfr_t top;
  mpfr_t width;
};

int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key,
                  const char *value);
int answer_to_connection(void *cls, struct MHD_Connection *connection,
                         const char *url, const char *method,
                         const char *version, const char *upload_data,
                         unsigned long *upload_data_size, void **con_cls) {
  struct MHD_Response *response;
  int ret;
  char *response_str;
  // Print to the console which endpoint and method was accessed
  if (strcmp(url, "/image") == 0) {
    if (strcmp(method, "GET") == 0) {
      loggf(DEBUG, "url sent was /image\n");

      if (!server_is_busy) {
        response_str = "Server now processing\n";
        server_is_busy = true;
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

        // temporary declaration/defintion of the length of supplied
        // arguments. Should eventually be supplied in the request
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

        //      make_image(x_pixels, y_pixels, THREAD_NUMBER,
        //      MY_PRECISION, left, top,
        //                 width, image_data);
        loggf(DEBUG, "%s(): Line %d: cls = %p\n", __func__, __LINE__, cls);

        struct server_irs irs;
        irs.server_coms_write = (FILE *)cls;
        loggf(DEBUG, "%s():irs.server_coms_write = %p\n", __func__,
              irs.server_coms_write);
        irs.x_pixels = x_pixels;
        irs.y_pixels = y_pixels;
        irs.thread_count = THREAD_NUMBER;
        irs.precision = MY_PRECISION;
        mpfr_init2(irs.top, MY_PRECISION);
        mpfr_init2(irs.left, MY_PRECISION);
        mpfr_init2(irs.width, MY_PRECISION);
        loggf(DEBUG, "%s(): irs mpfr variables initialised\n", __func__);

        mpfr_strtofr(irs.left, x, NULL, 10, MPFR_RNDD);
        mpfr_strtofr(irs.top, y, NULL, 10, MPFR_RNDD);
        mpfr_strtofr(irs.width, w, NULL, 10, MPFR_RNDD);
        loggf(DEBUG, "%s(): run mpfr_strtofr on irs variables\n", __func__);

        // thrd_t image_request_thread;
        if (thrd_create(&image_request_thread, &image_request,
                        (void *)(&irs)) != thrd_success) {
          loggf(ERROR, "%s(): thrd_create returned a failure\n", __func__);
          return 500; // Return an Internal server error
        }
        loggf(DEBUG, "%s(): thrd_create returned sucessfully\n", __func__);
      } else {
        loggf(DEBUG, "Server is busy\n");
        response_str = "Server is already busy\n";
      }
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
  //  What does the server do when it gets an image request? It starts a new
  //  thread that calls a function
  struct MHD_Daemon *daemon;

  int server_coms_pipe[2]; // Pipe for epoll to wait on.
  if (pipe(server_coms_pipe) == -1) {
    loggf(ERROR,
          "%s: pipe() failed to create file descriptors used for thread "
          "communication\n",
          __func__);
    return 1;
  }
  FILE *server_coms_write = fdopen(server_coms_pipe[1], "w");
  FILE *server_coms_read = fdopen(server_coms_pipe[0], "r");
  loggf(DEBUG, "%s(): Line %d: server_coms_write = %p\n", __func__, __LINE__,
        server_coms_write);

  // Launches the daemon on another thread
  daemon = MHD_start_daemon(
      MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL, &answer_to_connection,
      (void *)server_coms_write /* extra args for awnswer_to_connection here */,
      MHD_OPTION_END);
  if (daemon == NULL) {
    return 1;
  }

  loggf(INFO, "%s: Server started on port %d\n", __func__, PORT);

  bool exit =
      false; // To keep the server running until the /shutdown is received
  int bytes_read, pipe_read_buff_size = 15;
  char *pipe_read_buff[pipe_read_buff_size];
  struct epoll_event event, events[1];
  event.events = EPOLLIN;
  event.data.fd = server_coms_pipe[0];
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    loggf(ERROR,
          "%s(): epoll_create1 failed to create epoll file descriptor.\n",
          __func__);
    return 1;
  }
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_coms_pipe[0], &event) == -1) {
    loggf(ERROR, "%s(): epoll_ctl returned an error.\n", __func__);
    return 1;
  }

  while (!exit) {
    loggf(DEBUG, "%s(): epoll_wait called\n", __func__);
    if (epoll_wait(epoll_fd, events, 1, -1) == -1) {
      loggf(ERROR, "%s(): epoll_wait() returned an error.\n", __func__);
    }
    loggf(DEBUG, "%s(): epoll_wait received an event\n", __func__);
    memset(pipe_read_buff, (int)'\0', pipe_read_buff_size);
    bytes_read = read(server_coms_pipe[0], pipe_read_buff, pipe_read_buff_size);
    if (bytes_read == -1) {
      loggf(ERROR, "%s(): read() returned an error.\n", __func__);
    }

    if (strcmp((const char *)pipe_read_buff, "thread_finished") == 0) {
      loggf(DEBUG,
            "%s(): received 'thread_finished' msg from "
            "server_coms_pipe[0]\n",
            __func__);
      loggf(DEBUG, "%s(): Calling thrd_join() on image_request_thread\n",
            __func__);
      thrd_join(image_request_thread, NULL);
      server_is_busy = false;
    } // Shutdown callback needs to be implemented

    loggf(DEBUG, "%s(): pipe_read_buff = %s\n", __func__, pipe_read_buff);
  }

  fclose(server_coms_write);
  fclose(server_coms_read);

  MHD_stop_daemon(daemon);
  loggf(INFO, "%s: MHD_stop_daemon called\n", __func__);
  return 0;
}

int image_request(void *image_request_struct) {
  // TODO error handling
  loggf(DEBUG, "%s(): called\n", __func__);
  // Will need to take in a pipe fd descriptor that can be used to talk to
  // start_server (which will be called on the main thread
  //
  struct server_irs *args = (struct server_irs *)image_request_struct;
  FILE *server_coms_write = args->server_coms_write;

  size_t image_data_size = args->y_pixels * args->x_pixels * 3;
  unsigned char *image_data = calloc(image_data_size, sizeof(unsigned char));
  loggf(DEBUG, "%s(): calloc() called to create buffer for image_data\n",
        __func__);

  loggf(DEBUG, "%s(): is calling make_image()\n", __func__);
  make_image(args->x_pixels, args->y_pixels, args->thread_count,
             args->precision, args->left, args->top, args->width, image_data);
  loggf(DEBUG, "%s(): make_image() returned\n", __func__);

  stbi_write_jpg("image.jpg", args->x_pixels, args->y_pixels, 3, image_data,
                 100);

  stream_data(image_data, image_data_size, "192.168.0.197",
              4321); // TODO: error Handling

  // One off write to file
  int write_status =
      write_uint8_to_file("./image_data_file", image_data, image_data_size);

  if (write_status != 0) {
    loggf(ERROR, "%s: write_uint8_to_file returned an unexpected error\n",
          __func__);
  }

  free(image_data);
  loggf(DEBUG, "%s(): free() called on data allocated to image_data\n",
        __func__);

  loggf(DEBUG, "%s(): calling fprintf to server_coms_pipe stream\n", __func__);
  fprintf(server_coms_write, "thread_finished"); // TODO: error handling
  loggf(DEBUG, "%s(): calling fflush to server_coms_pipe stream\n", __func__);
  fflush(server_coms_write);

  return 0;
}

int stream_data(unsigned char *image_data, size_t image_data_size,
                char *server_string, int server_port) {
  struct sockaddr_in client_socket, server_address_socket;

  // setup client socket
  client_socket.sin_family = AF_INET;
  int client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket_fd == -1) {
    loggf(ERROR, "%s(): socket() returned an error creating client_socket_fd\n",
          __func__);
    return -1;
  } else {
    loggf(DEBUG, "%s(): socket() returned a valid file descriptor", __func__);
  }

  // setup server address socket
  server_address_socket.sin_family = AF_INET;
  server_address_socket.sin_port = htons(server_port);
  server_address_socket.sin_addr.s_addr = inet_addr(server_string);

  //  // bind
  //  int bind_status = bind(client_socket_fd, (struct sockaddr
  //  *)&client_socket,
  //                         sizeof(client_socket));
  //  if (bind_status == -1) {
  //    loggf(ERROR, "%s():bind() returned an error on client_socket_fd\n",
  //          __func__);
  //    return -1;
  //  }

  // connect
  int connect_status =
      connect(client_socket_fd, (struct sockaddr *)&server_address_socket,
              sizeof(server_address_socket));
  if (connect_status == -1) {
    loggf(ERROR,
          "%s(): connect() returned an error attempting to connect to %s on "
          "port %d\n",
          __func__, server_string, server_port);
    return -1;
  } else {
    loggf(DEBUG,
          "%s(): connect() succesfully connect to %s on "
          "port %d\n",
          __func__, server_string, server_port);
  }

  // convert image data to expected data type
  uint8_t image_data_tranmission[image_data_size];
  int i;
  for (i = 0; i < image_data_size; i++) {
    image_data_tranmission[i] = (uint8_t)image_data[i];
  }
  size_t transmission_size = image_data_size * sizeof(uint8_t);

  // send()
  int send_status =
      send(client_socket_fd, image_data_tranmission, transmission_size, 0);
  if (send_status != transmission_size) {
    loggf(ERROR,
          "%s(): send() failed to send data to %s on "
          "port %d. Send status = %d\n",
          __func__, server_string, server_port, send_status);
    loggf(DEBUG, "Intended to send %zu bytes\n", transmission_size);
    return -1;
  } else if (send_status != transmission_size) {
    loggf(ERROR,
          "%s(): send() failed to send all data to %s on "
          "port %d\n",
          __func__, server_string, server_port);
    return -1;
  } else {
    loggf(DEBUG,
          "%s(): send() sent %d bytes of  data to %s on "
          "port %d\n",
          __func__, send_status, server_string, server_port);
  }
  close(client_socket_fd);
  return 0;
}
