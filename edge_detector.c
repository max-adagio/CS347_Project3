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
void *compute_laplacian_threadfn(void *params)
{
    
    int laplacian[FILTER_WIDTH][FILTER_HEIGHT] =
    {
        {-1, -1, -1},
        {-1,  8, -1},
        {-1, -1, -1}
    };

    int red, green, blue;
     
      
      
        
    return NULL;
}

/* Apply the Laplacian filter to an image using threads.
 Each thread shall do an equal share of the work, i.e. work=height/number of threads. If the size is not even, the last thread shall take the rest of the work.
 Compute the elapsed time and store it in *elapsedTime (Read about gettimeofday).
 Return: result (filtered image)
 */
PPMPixel *apply_filters(PPMPixel *image, unsigned long w, unsigned long h, double *elapsedTime) {

    PPMPixel *result;
    result = (PPMPixel *)malloc(w * h *sizeof(PPMPixel));
    if (!result) {  // error check
        fprintf(stderr, "Error: Unable to allocate memory for the result image\n");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[LAPLACIAN_THREADS];
    struct parameter params[LAPLACIAN_THREADS];

    // calculating the work to be performed per thread
    unsigned long int total_pixels = w * h;
    unsigned long int work_per_thread = total_pixels / LAPLACIAN_THREADS;
    unsigned long int remaining_work = total_pixels % LAPLACIAN_THREADS;    // what does not divide evenly

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Create threads
    for (int i = 0; i < LAPLACIAN_THREADS; i++) {
        params[i].image = image;
        params[i].result = result;
        params[i].w = w;
        params[i].h = h;
        params[i].start = i * work_per_thread;
        params[i].size = (i == LAPLACIAN_THREADS - 1) ? 
                         (work_per_thread + remaining_work) : work_per_thread;

        if (pthread_create(&threads[i], NULL, compute_laplacian_threadfn, &params[i]) != 0) {
            fprintf(stderr, "Error: Unable to create thread %d\n", i);
            free(result);
            exit(EXIT_FAILURE);
        }
    }

    // Join threads
    for (int i = 0; i < LAPLACIAN_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
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

/*Create a new P6 file to save the filtered image in. Write the header block
 e.g. P6
      Width Height
      Max color value
 then write the image data.
 The name of the new file shall be "filename" (the second argument).
 */
void write_image(PPMPixel *image, char *filename, unsigned long int width, unsigned long int height)
{
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
    printf("image successfully written\n");
}



/* Open the filename image for reading, and parse it.
    Example of a ppm header:    //http://netpbm.sourceforge.net/doc/ppm.html
    P6                  -- image format
    # comment           -- comment lines begin with
    ## another comment  -- any number of comment lines
    200 300             -- image width & height 
    255                 -- max color value
 
 Check if the image format is P6. If not, print invalid format error message.
 If there are comments in the file, skip them. You may assume that comments exist only in the header block.
 Read the image size information and store them in width and height.
 Check the rgb component, if not 255, display error message.
 Return: pointer to PPMPixel that has the pixel data of the input image (filename).The pixel data is stored in scanline order from left to right (up to bottom) in 3-byte chunks (r g b values for each pixel) encoded as binary numbers.
 */
PPMPixel *read_image(const char *filename, unsigned long int *width, unsigned long int *height)
{
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
    // DELETE THIS IF UNNECESSARY
    fgetc(file);
    char c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '#') {
            printf("Found a comment starting with '#':\n");

            // Skip the rest of the comment line
            while ((c = fgetc(file)) != '\n' && c != EOF){
                printf("%c", c);
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

/* The thread function that manages an image file. 
 Read an image file that is passed as an argument at runtime. 
 Apply the Laplacian filter. 
 Update the value of total_elapsed_time.
 Save the result image in a file called laplaciani.ppm, where i is the image file order in the passed arguments.
 Example: the result image of the file passed third during the input shall be called "laplacian3.ppm".

*/
void *manage_image_file(void *args){
    

}
/*The driver of the program. Check for the correct number of arguments. If wrong print the message: "Usage ./a.out filename[s]"
  It shall accept n filenames as arguments, separated by whitespace, e.g., ./a.out file1.ppm file2.ppm    file3.ppm
  It will create a thread for each input file to manage.  
  It will print the total elapsed time in .4 precision seconds(e.g., 0.1234 s). 
 */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ./edge_detector %s[s]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    unsigned long int width, height;

    printf("reading image: %s\n", filename);
    PPMPixel *image = read_image(filename, &width, &height);

    if (image) {
        printf("Success\n");
        printf("Width: %lu, Height: %lu\n", width, height);

        printf("First few pixels (RGB values):\n");
        for (int i = 0; i < 10 && i < width * height; i++) {
            printf("Pixel %d: R=%u G=%u B=%u\n", i, image[i].r, image[i].g, image[i].b);
        }
        write_image(image, "laplacian.ppm", width, height);


        free (image);
        
        
    } else {
        printf("Failure\n");
    }

    
    return 0;
}

