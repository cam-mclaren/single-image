#include <gmp.h>
#include <mpfr.h>
#include <stdio.h>

#define MY_PRECISION 665

int main(int argc, char **argv) {

  mpfr_t x_centre, y_centre, width;
  mpfr_init2(x_centre, MY_PRECISION);
  mpfr_init2(y_centre, MY_PRECISION);
  mpfr_init2(width, MY_PRECISION);

  mpfr_strtofr(x_centre, *(argv + 1), NULL, 10, MPFR_RNDD);
  mpfr_strtofr(y_centre, *(argv + 2), NULL, 10, MPFR_RNDD);
  mpfr_strtofr(width, *(argv + 3), NULL, 10, MPFR_RNDD);

  int x_pixels = 1280;
  int y_pixels = 720;

  mpfr_t aspect_ratio;
  mpfr_init2(aspect_ratio, MY_PRECISION);
  mpfr_set_si(aspect_ratio, y_pixels, MPFR_RNDD);
  mpfr_div_si(aspect_ratio, aspect_ratio, x_pixels, MPFR_RNDD);

  mpfr_t height;
  mpfr_init2(height, MY_PRECISION);
  mpfr_mul(height, aspect_ratio, width, MPFR_RNDD);

  mpfr_t half_height;
  mpfr_init2(half_height, MY_PRECISION);
  mpfr_div_ui(half_height, height, 2, MPFR_RNDD);

  mpfr_t half_width;
  mpfr_init2(half_width, MY_PRECISION);
  mpfr_div_ui(half_width, width, 2, MPFR_RNDD);

  mpfr_t y_corner;
  mpfr_init2(y_corner, MY_PRECISION);
  mpfr_sub(y_corner, y_centre, half_height, MPFR_RNDD);

  mpfr_t x_corner;
  mpfr_init2(x_corner, MY_PRECISION);
  mpfr_sub(x_corner, x_centre, half_width, MPFR_RNDD);

  mpfr_printf("x_corner: %0.50Rf\n", x_corner);
  mpfr_printf("y_corner: %0.50Rf\n", y_corner);
  return 0;
}
