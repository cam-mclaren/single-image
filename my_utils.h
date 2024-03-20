#ifndef MY_UTILS_H
#define MY_UTILS_H

// A function to print arrays of doubles
void print_double_array(double *array, int size);

void fill_double_array_from_file(const char * path, int size, double * target_array);

// Function to count the number of lines in a file of doubles (Assumes one double per line)
int count_doubles_in_file(const char * path);

// This function supplied with the right coefficients and node arguments should give the value of the cubic B spline at x
double interp_poly(double * coefficients, double * nodes, double x);
#endif
