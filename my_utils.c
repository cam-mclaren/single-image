/*
 *TODO:
 main _
      |
      |
      draw_image(corner, width, resolution, threads, precision)
              |
              |
              |______ returns a pointer to a buffer

*/
#include "my_utils.h"
#include "log.h"
#include <math.h>
#include <stdio.h>

int zero_fill(int arg_len, int array_len, char array[]) {
  int i;
  for (i = arg_len; i < (array_len - 1); i++) {
    array[i] = '0';
  }
  array[array_len - 1] = 0;
  return 0;
}

void print_double_array(double *array, int size) {
  int index;
  for (index = 0; index < size; index++) {
    fprintf(stdout, "%f\n", array[index]);
  }
}

void fill_double_array_from_file(const char *path, int size,
                                 double *target_array) {
  loggf(DEBUG, "%s: Reading array from file %s\n", __func__, path);
  FILE *file_pointer = fopen(path, "r");
  int index;
  double num;
  for (index = 0; index < size; index++) {
    fscanf(file_pointer, "%lf", &num);
    target_array[index] = num;
  }
  fclose(file_pointer);
}

// Function to count the number of lines in a file of doubles (Assumes one
// double per line)
int count_doubles_in_file(const char *path) {
  loggf(DEBUG, "%s: Counting doubles from file %s\n", __func__, path);
  FILE *file_pointer = fopen(path, "r");
  if (file_pointer == NULL) {
    fprintf(stderr,
            "Error in count_lines_in_file().\n%s failed to open file.\n",
            __func__);
    return -1;
  }
  int line_count = 0;
  double line_contents;
  while (fscanf(file_pointer, "%lf", &line_contents) == 1) {
    line_count++;
  }
  fclose(file_pointer);
  return line_count;
}

// This function supplied with the right coefficients and node arguments should
// give the value of the cubic B spline at x
double interp_poly(double *coefficients, double *nodes, double x) {

  // this function ... seems a bit wild. There are no length limits on accessing
  // the array. Array limits are somehow infered from the increments in nodes.

  double h = nodes[1] - nodes[0];
  int k = floor((x - nodes[1]) / h) +
          2; // We add two instead of one because arrays begin their index at 0.
             // In the textbook we used indexing starting at -1.

  return coefficients[k - 2] * pow((2 - (x - nodes[k - 2]) / h), 3) +
         coefficients[k - 1] * (1 + 3 * pow((1 - (x - nodes[k - 1]) / h), 1) +
                                3 * pow((1 - (x - nodes[k - 1]) / h), 2) -
                                3 * pow(1 - (x - nodes[k - 1]) / h, 3)) +
         coefficients[k] * (1 + 3 * pow(1 + (x - nodes[k]) / h, 1) +
                            3 * pow(1 + (x - nodes[k]) / h, 2) -
                            3 * pow(1 + (x - nodes[k]) / h, 3)) +
         coefficients[k + 1] * pow(2 + (x - nodes[k + 1]) / h, 3);
}
