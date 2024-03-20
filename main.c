#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gmp.h>
#include <mpfr.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "my_utils.h"

#include <threads.h>

#include <time.h>
#include <sys/time.h>

#define MY_PRECISION 665
#define THREAD_NUMBER 4

typedef struct image_params 
{
	int y_pixels;
	int x_pixels;
	int * row_count;
	unsigned char * image_data;
	mpfr_t * x_array;
	mpfr_t * y_array;
	mtx_t * mutex_ptr;
	double * red_coefficients;
	double * green_coefficients;
	double * blue_coefficients;		
	double * nodes;
} image_params;

/* This is a wrapper around the arguments common to all threads. This allows the threads to identify themselves 
when logging their progress.*/
typedef struct thrd_awrp
{
	char thread_name[20];
	int thread_index;
	image_params * args;
} thrd_awrp;


int worker_function( void * wrapper_arg /* void pointer to pointer to struct*/)
{   /*******************************************************************************************
	* So its outputs go in image_data
	* It can be fed a void pointer which it can cast to an int pointer. It reads this int pointer so it knows 
	* what row it is working on. It increments this pointer once it is done. 
	*
	* Now it also has to take in the y value somehow. It then has to iterate over the x values. 
	********************************************************************************************/

	// Declare local copies of necessary variables.
	int current_row;
	mpfr_t current_y, current_x;
	int r, g, b;
	double speed, inner_product;
	int max_iter=4000;
	int iter_count=0;
	mpfr_t real_component, imaginary_component, x_square, y_square, real_temp;
	mpfr_t c_x, c_y;
	mpfr_init2(real_component, MY_PRECISION);
	mpfr_init2(imaginary_component, MY_PRECISION);
	mpfr_init2(real_temp, MY_PRECISION);
	mpfr_init2(x_square, MY_PRECISION);
	mpfr_init2(y_square, MY_PRECISION);
	mpfr_init2(current_x,MY_PRECISION);
	mpfr_init2(current_y,MY_PRECISION);

	void free_mpfr_vars(void)
	{
		mpfr_clear(real_component);
		mpfr_clear(imaginary_component);
		mpfr_clear(real_temp );
		mpfr_clear(x_square);
		mpfr_clear(y_square);
		mpfr_clear(current_x);
		mpfr_clear(current_y);
	}

	thrd_awrp thread_argument_wrapper = *(thrd_awrp*)wrapper_arg;	

	image_params *args = thread_argument_wrapper.args;
	printf("Thread %d is active\n", thread_argument_wrapper.thread_index);
	
	int count = 0;
	printf("args->ypixels = %d\n", args->y_pixels);
	while (count < args->y_pixels)
	{

		mtx_lock(args->mutex_ptr);
		current_row = *(args->row_count); // read current row
		//printf("current row = %d\n", current_row);
		if (current_row >= args->y_pixels)
		{	
			printf("Worker function is returning\n");
			free_mpfr_vars();		
			mtx_unlock(args->mutex_ptr);
			return 0;
		} else
		{
			*(args->row_count) = current_row+1;
			// printf("incrementing\n");
		}
		mtx_unlock(args->mutex_ptr);

		mpfr_set(current_y, args->y_array[current_row], MPFR_RNDD);

		// mpfr_fprintf(stdout,"%5.50Rf\n",current_y);
		int j;
		for(j = 0; j< args->x_pixels; j++)
		{	

			//mpfr_fprintf(stdout,"%5.50Rf\n",args->x_array[j]);
			mpfr_set(current_x, args->x_array[j], MPFR_RNDD);
			mpfr_set(real_component, current_x, MPFR_RNDD); //  Initialise x value
			mpfr_set(imaginary_component, current_y, MPFR_RNDD); // Initialise y value
			// run time escape algorithm
			inner_product=0;
			iter_count=0;
			while( (iter_count < max_iter)&&(inner_product<4))
			{
				// f(z) = z^2 + c
				// (x+yi)(x+yi) = x^2 - y^2 + (2xy)i
				//Real component calculation store in real_temp
				mpfr_pow_si(x_square, real_component, 2, MPFR_RNDD);
				mpfr_pow_si(y_square, imaginary_component, 2, MPFR_RNDD);
				mpfr_sub(real_temp, x_square, y_square, MPFR_RNDD);
				//imaginary_component
				mpfr_mul(imaginary_component, imaginary_component, real_component, MPFR_RNDD);
				mpfr_mul_si(imaginary_component, imaginary_component, 2, MPFR_RNDD); // imaginary component calculate
				// add c
				mpfr_add(real_component, real_temp, current_x, MPFR_RNDD);
				mpfr_add(imaginary_component, imaginary_component, current_y, MPFR_RNDD);
				//find the inner product
				mpfr_pow_si(x_square, real_component, 2, MPFR_RNDD);
				mpfr_pow_si(y_square, imaginary_component, 2, MPFR_RNDD);
				mpfr_add(real_temp, x_square, y_square, MPFR_RNDD);
				inner_product=mpfr_get_d(real_temp, MPFR_RNDD);

				iter_count++;

			}
			speed = (double)iter_count/max_iter;
			if (speed==1)
			{
				// Does nothing. Pixel is initialised as black.
				// This is because calloc is zero initialised. (RGB 000 000 000 is black)
			}else
			{
				r = floor(interp_poly(args->red_coefficients, args->nodes, speed));
				g = floor(interp_poly(args->green_coefficients, args->nodes, speed));
				b = floor(interp_poly(args->blue_coefficients, args->nodes, speed));

				if( r > 255){ r=255;}
				if( g > 255){ g=255;}
				if( b > 255){ b=255;}
				if( r < 0){ r=0;}
				if( g < 0){ g=0;}
				if( b < 0){ b=0;}

				*(args->image_data+(args->y_pixels - 1 - current_row)*args->x_pixels*3 + 3*j+0)=(unsigned char)r;
				*(args->image_data+(args->y_pixels - 1 - current_row)*args->x_pixels*3 + 3*j+1)=(unsigned char)g;
				*(args->image_data+(args->y_pixels - 1 - current_row)*args->x_pixels*3 + 3*j+2)=(unsigned char)b;

			}
		}
		count++;
	}
	free_mpfr_vars();
	fprintf(stderr, "Thread function loop maxed out");
	return -99;
}
// Instead maybe it reads targets from a file.



int main(int argc, char ** argv)
{

	// Read from file the coordinates

	//Wall clock time
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	// Times main()
	clock_t begin = clock();
	

	mpfr_t x_coordinate, y_coordinate, width;
	int precision=1065;
	mpfr_init2(x_coordinate, MY_PRECISION);
	mpfr_init2(y_coordinate, MY_PRECISION);
	mpfr_init2(width, MY_PRECISION);
	int read_status;

    mpfr_strtofr(x_coordinate, *(argv+1), NULL, 10, MPFR_RNDD);
    mpfr_strtofr(y_coordinate, *(argv+2), NULL, 10, MPFR_RNDD);
    mpfr_strtofr(width, *(argv+3), NULL, 10, MPFR_RNDD);

	// Load the coefficients and nodes for the colouring spline//
	int coefficent_number = count_doubles_in_file("./interpolation/colour_parameters/blue_coefficients");
	double * red_coefficients=malloc(coefficent_number*sizeof(double));
	double * green_coefficients=malloc(coefficent_number*sizeof(double));
	double * blue_coefficients=malloc(coefficent_number*sizeof(double));
	double * nodes= malloc(coefficent_number*sizeof(double));

	fill_double_array_from_file("./interpolation/colour_parameters/red_coefficients",\
															coefficent_number,red_coefficients);
	fill_double_array_from_file("./interpolation/colour_parameters/green_coefficients",\
															coefficent_number,green_coefficients);
	fill_double_array_from_file("./interpolation/colour_parameters/blue_coefficients",\
															coefficent_number,blue_coefficients);
	fill_double_array_from_file("./interpolation/colour_parameters/nodes",\
															coefficent_number,nodes);

	// declare and initialise mutex
	mtx_t  mutex;
	if (mtx_init(&mutex, mtx_plain) != thrd_success)
	{
		fprintf(stderr, "Error initialising mutex (for image) on line %d\n", __LINE__);
		return -1;
	}
	fprintf(stdout, "Mutex (for image) successfully initialised at line %d\n", __LINE__);

	//define integer representing number of rows in image processed. Cast this as 
	//a void pointer. 
	int rows_processed = 0;
	void * thread_argument = (void*)&rows_processed;

	//resolution
	int x_pixels = 1280;
	int y_pixels = 720;

	//aspect ratio
	mpfr_t aspect_ratio;
	mpfr_init2(aspect_ratio, MY_PRECISION);
	mpfr_set_si(aspect_ratio, y_pixels, MPFR_RNDD);
	mpfr_div_si(aspect_ratio, aspect_ratio, x_pixels, MPFR_RNDD);

	// delare and initialse variables for boundarys and nudges
	mpfr_t left, top, hoz_nudge, ver_nudge, half_height;
	mpfr_init2(left, MY_PRECISION);
	mpfr_init2(top, MY_PRECISION);
	mpfr_init2(hoz_nudge, MY_PRECISION);
	mpfr_init2(ver_nudge, MY_PRECISION);
	mpfr_init2(half_height, MY_PRECISION);

	mpfr_div_ui(hoz_nudge, width, 2, MPFR_RNDD); // use hoz nudge to store half width
	mpfr_sub(left, x_coordinate, hoz_nudge, MPFR_RNDD);  // left calculated
	mpfr_div_si(hoz_nudge, width, (x_pixels-1), MPFR_RNDD); //Now calculate the horizontal nudge

	mpfr_mul(half_height, aspect_ratio, width, MPFR_RNDD); //height calculated
	mpfr_div_si(ver_nudge, half_height, (y_pixels-1), MPFR_RNDD); // vertical nudge calculated
	mpfr_div_si(half_height, half_height, 2, MPFR_RNDD);
	mpfr_add(top, y_coordinate, half_height, MPFR_RNDD); // Top boundary calculated

	// Populate y values used. //Array is backwards atm :'-(

	printf("top is:\n");
	mpfr_fprintf(stdout,"%5.50Rf\n",top);
	int index;
	mpfr_t y_val, y_increment;
	mpfr_t y_values[y_pixels];
	mpfr_init2(y_val, MY_PRECISION);
	mpfr_init2(y_increment, MY_PRECISION);
	for (index = y_pixels - 1; index >= 0 ; index -- )
	{
		mpfr_mul_si(y_increment, ver_nudge, (y_pixels -1 - index), MPFR_RNDD);
		mpfr_sub(y_val, top, y_increment, MPFR_RNDD);
		mpfr_init2(y_values[index], MY_PRECISION);
		mpfr_set(y_values[index], y_val, MPFR_RNDD);
		// mpfr_fprintf(stdout,"%5.50Rf\n",y_values[index]);
	}
	// Populate x values used. (Not yet utilised)
	mpfr_t x_val, x_increment;
	mpfr_t x_values[x_pixels];
	mpfr_init2(x_val, MY_PRECISION);
	mpfr_init2(x_increment, MY_PRECISION);
	for (index = 0 ; index < x_pixels; index ++ )
	{
		mpfr_mul_si(x_increment, hoz_nudge, index, MPFR_RNDD);
		mpfr_add(x_val, left, x_increment, MPFR_RNDD);
		mpfr_init2(x_values[index], MY_PRECISION);
		mpfr_set(x_values[index], x_val, MPFR_RNDD);
		// mpfr_fprintf(stdout,"%5.50Rf\n",x_values[index]);
	}

	mpfr_clear(half_height);
	mpfr_clear(aspect_ratio);
	mpfr_clear(x_coordinate);
	mpfr_clear(y_coordinate);
	mpfr_clear(width);

	printf("okay thus far\n");

	// define image_data array
	unsigned char * image_data = calloc(y_pixels*x_pixels*3, sizeof(unsigned char));

	thrd_t threads[THREAD_NUMBER];

	image_params thread_parameters; // thread_parameters is basically how the main function can interface with the
									// worker_function. Most information passed into the structure should only be 
									// read. However row_count, mutex_ptr, and image_data will have write operations 
									// performed on them. 
	thread_parameters.x_pixels = x_pixels;
	thread_parameters.y_pixels = y_pixels;
	thread_parameters.row_count = &rows_processed; // Threads will increment this to let others know row is complete
	thread_parameters.mutex_ptr = &mutex;  // Makes rows_processed increment thread safe
	thread_parameters.image_data = image_data;
	thread_parameters.y_array = y_values;
	thread_parameters.x_array = x_values;
	thread_parameters.red_coefficients=  red_coefficients;  // Possibly I should consider this being put into a substruct. 
	thread_parameters.green_coefficients = green_coefficients; 
	thread_parameters.blue_coefficients = blue_coefficients;
	thread_parameters.nodes = nodes;	

	int thread_index;
	thrd_awrp wrapper_arg[THREAD_NUMBER];
	char * thread_int_str[20];
	char * thread_name_str[THREAD_NUMBER][20];
	
	for (thread_index = 0; thread_index < THREAD_NUMBER; thread_index++)
	{	
		sprintf((char*)thread_name_str[thread_index], "Thread_%d", thread_index);
		wrapper_arg[thread_index].thread_index = thread_index;
		strcpy(wrapper_arg[thread_index].thread_name, (char*)thread_name_str[thread_index]);
		wrapper_arg[thread_index].args = &thread_parameters;

		if (thrd_create(&threads[thread_index],worker_function, (void*) &wrapper_arg[thread_index]) != thrd_success)
		{
			fprintf(stderr, "Error creating thread %d.\n", thread_index);
			return -99;
		}
		else {
			printf("Successfully started thread %d.\nWrapper arg thread index is", thread_index);
		}
	}

	for (thread_index = 0; thread_index < THREAD_NUMBER; thread_index++)
	{
		thrd_join(threads[thread_index], NULL);
		printf("Thread %d joined.\n", thread_index);
	}

	stbi_write_jpg("image.jpg",x_pixels,y_pixels,3,image_data,100);
	free((void*)image_data);
	mpfr_clear(hoz_nudge);
	mpfr_clear(left);
	mpfr_clear(top);
	mpfr_clear(ver_nudge);
	mpfr_free_cache();
	mtx_destroy(&mutex);

	clock_t end = clock();

	double time_spent = (double)(end-begin)/CLOCKS_PER_SEC;
	printf("Main() CPU time used was %lf seconds\n", time_spent);

	struct timeval tv2;
	struct timezone tz2;
	gettimeofday(&tv2, &tz2);

	printf("Wall clock time elapsed is %lf seconds.\n", (double)(tv2.tv_sec - tv.tv_sec)+(double)(tv2.tv_usec - tv.tv_usec)/1000000);

	return 0;
}
















