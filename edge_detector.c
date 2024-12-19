/*  CSCI 347
 *  Project 3 - "Edge Detector"
 *  Max Widjaja
 *  2 December 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define LAPLACIAN_THREADS 4     //change the number of threads as you run your concurrency experiment

/* Laplacian filter is 3 by 3 */
#define FILTER_WIDTH 3       
#define FILTER_HEIGHT 3      

#define RGB_COMPONENT_COLOR 255

typedef struct {
    unsigned char r, g, b;
} PPMPixel;

struct parameter {
    PPMPixel *image;         //original image pixel data
    PPMPixel *result;        //filtered image pixel data
    unsigned long int w;     //width of image
    unsigned long int h;     //height of image
    unsigned long int start; //starting point of work
    unsigned long int size;  //equal share of work (almost equal if odd)
};


struct file_name_args {
    char *input_file_name;      //e.g., file1.ppm
    char output_file_name[20];  //will take the form laplaciani.ppm, e.g., laplacian1.ppm
};

/*The total_elapsed_time is the total time taken by all threads 
to compute the edge detection of all input images .
*/
double total_elapsed_time = 0; 


/*This is the thread function. It will compute the new values for the region of image specified in params (start to start+size) using convolution.
    For each pixel in the input image, the filter is conceptually placed on top ofthe image with its origin lying on that pixel.
    The  values  of  each  input  image  pixel  under  the  mask  are  multiplied  by the corresponding filter values.
    Truncate values smaller than zero to zero and larger than 255 to 255.
    The results are summed together to yield a single output value that is placed in the output image at the location of the pixel being processed on the input.
 
 */

void *compute_laplacian_threadfn(void *params) {
    struct parameter *p = (struct parameter *)params;

    int laplacian[FILTER_WIDTH][FILTER_HEIGHT] = {
        {-1, -1, -1},
        {-1,  8, -1},
        {-1, -1, -1}
    };

    for (unsigned long int i = p->start; i < p->start + p->size; i++) {
        unsigned long int x = i % p->w;  // column index
        unsigned long int y = i / p->w;  // row index

        int red = 0; 
        int green = 0; 
        int blue = 0;

        // Apply the filter
        for (int fy = 0; fy < FILTER_HEIGHT; fy++) {
            for (int fx = 0; fx < FILTER_WIDTH; fx++) {
                int nx = x + fx - 1;
                int ny = y + fy - 1;

                // Boundary check
                if (nx >= 0 && nx < p->w && ny >= 0 && ny < p->h) {
                    PPMPixel *neighbor = &p->image[ny * p->w + nx];
                    red   += neighbor->r * laplacian[fy][fx];
                    green += neighbor->g * laplacian[fy][fx];
                    blue  += neighbor->b * laplacian[fy][fx];
                }
            }
        }

        // Clamp values to the range [0, 255]
        p->result[y * p->w + x].r = (unsigned char) fmax(0, fmin(RGB_COMPONENT_COLOR, red));
        p->result[y * p->w + x].g = (unsigned char) fmax(0, fmin(RGB_COMPONENT_COLOR, green));
        p->result[y * p->w + x].b = (unsigned char) fmax(0, fmin(RGB_COMPONENT_COLOR, blue));
    }

    return NULL;
}

PPMPixel *apply_filters(PPMPixel *image, unsigned long w, unsigned long h, double *elapsedTime) {
    // instantiate and initialize result array of pixels
    PPMPixel *result;
    result = (PPMPixel *)malloc(w * h *sizeof(PPMPixel));

    if (!result) {  // error check
        fprintf(stderr, "Error: Unable to allocate memory for the result image\n");
        exit(EXIT_FAILURE);
    }

    // instantiate threads array
    pthread_t threads[LAPLACIAN_THREADS];
    struct parameter params[LAPLACIAN_THREADS]; // each thread gets a set of params

    // calculating the work to be performed per thread
    unsigned long int total_pixels = w * h;
    // 
    unsigned long int work_per_thread = total_pixels / LAPLACIAN_THREADS;
    unsigned long int remaining_work = total_pixels % LAPLACIAN_THREADS;    // whatever doesn't divide evenly

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Create threads
    for (int i = 0; i < LAPLACIAN_THREADS; i++) {
        params[i].image = image;
        params[i].result = result;
        params[i].w = w;
        params[i].h = h;
        params[i].start = i * work_per_thread;  // defines index point into image array
        params[i].size = (i == LAPLACIAN_THREADS - 1) ? /* is it the last thread? */
                         (work_per_thread + remaining_work) : work_per_thread;

        // creates threads
        if (pthread_create(& [i], NULL, compute_laplacian_threadfn, &params[i]) != 0) {
            fprintf(stderr, "Error: Unable to create thread %d\n", i);
            free(result);
            exit(EXIT_FAILURE);
        }
    }

    // join threads to wait for all threads to finish work
    for (int i = 0; i < LAPLACIAN_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {  // call in error catch
            fprintf(stderr, "Error: Unable to join thread %d\n", i);
            free(result);
            exit(EXIT_FAILURE);
        }
    }

    gettimeofday(&end_time, NULL);

    // Calculate elapsed time
    *elapsedTime = (end_time.tv_sec - start_time.tv_sec) +
                   (end_time.tv_usec - start_time.tv_usec) / 1e6;

    return result;
}

void write_image(PPMPixel *image, char *filename, unsigned long int width, unsigned long int height) {
    FILE *newfile = fopen(filename, "wb");
    char format[3];
    fprintf(newfile, "P6\n");
    fprintf(newfile, "%lu %lu\n", width, height);
    fprintf(newfile, "%d\n", RGB_COMPONENT_COLOR);

    size_t pixel_count = width * height;
    
    if (fwrite(image, sizeof(PPMPixel), pixel_count, newfile) != pixel_count) {
        fprintf(stderr, "Error: Unable to write pixel data to file %s\n", filename);
    }
    fclose(newfile);
    printf("Image successfully written.\n");
}

PPMPixel *read_image(const char *filename, unsigned long int *width, unsigned long int *height) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // Read the PPM format
    char format[3];
    if (!fgets(format, sizeof(format), file)) {
        fprintf(stderr, "Error: Unable to read the image format\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (strncmp(format, "P6", 2) != 0) {
        fprintf(stderr, "Error: Invalid image format (must be P6)\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // skip comments in the header 
    fgetc(file);
    char c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '#') {
            // Skip the rest of the comment line
            while ((c = fgetc(file)) != '\n' && c != EOF){
            }
            
        } else {
            // Put the character back and stop processing comments
            ungetc(c, file);
            break;
        }
    }

    if (fscanf(file, "%lu %lu", width, height) != 2) {
        fprintf(stderr, "Error: Invalid image size (width, height)\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    int max_val;
    if (fscanf(file, "%d", &max_val) != 1 || max_val != RGB_COMPONENT_COLOR) {
        fprintf(stderr, "Error: Invalid max color value (must be 255)\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Skip single whitespace character after max color value
    fgetc(file);
    PPMPixel *img = (PPMPixel *)malloc((*width) * (*height) * sizeof(PPMPixel));
    if (!img) {
        fprintf(stderr, "Error: Unable to allocate memory for image\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // actually reading the pixel data finally!
    if (fread(img, sizeof(PPMPixel), (*width) * (*height), file) != (*width) * (*height)) {
        fprintf(stderr, "Error: unable to read pixel data\n");
        free(img);
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);
    return img;
}

void *manage_image_file(void *args) {
    struct file_name_args *file_args = (struct file_name_args *)args;

    // define the input and output file names separately
    char *input_file_name = file_args->input_file_name;
    char *output_file_name = file_args->output_file_name;

    unsigned long int width, height;
    double elapsedTime;

    // Read the input image
    PPMPixel *image = read_image(input_file_name, &width, &height);

    if (!image) {
        fprintf(stderr, "Error: Failed to read image %s\n", input_file_name);
        return NULL;
    }

    // Apply the Laplacian filter
    PPMPixel *result = apply_filters(image, width, height, &elapsedTime);

    // Update the global elapsed time (critical section)
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
    total_elapsed_time += elapsedTime;
    pthread_mutex_unlock(&mutex);

    // Write the filtered image to the output file
    write_image(result, output_file_name, width, height);

    // Free allocated memory
    free(image);
    free(result);
    pthread_mutex_destroy(&mutex);

    return NULL;
}

int main(int argc, char *argv[]) {
    int num_threads = argc - 1;
    pthread_t arr_threads[num_threads];
    struct file_name_args file_names[num_threads];

    for(int i = 0; i < num_threads; i++) {
        file_names[i].input_file_name = argv[i + 1]; // saves input filename

        snprintf(file_names[i].output_file_name, sizeof(file_names[i].output_file_name), "laplacian%d.ppm", i + 1);
        pthread_create(&arr_threads[i], NULL, manage_image_file, &file_names[i]);  // figure this out
    }   

    for(int i = 0; i < num_threads; i++) {
        pthread_join(arr_threads[i], NULL);
    }

    return EXIT_SUCCESS;
}

