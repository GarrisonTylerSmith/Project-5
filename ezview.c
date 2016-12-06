#define GLFW_DLL 1
#define GL_GLEXT_PROTOTYPES
#define CREATOR "Garrison Smith"
#define RGB_COMPONENT_COLOR 255

#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


GLFWwindow* window;

typedef struct {
     unsigned char red,green,blue;
} PPMPixel;

typedef struct {
     int x, y;
     PPMPixel *data;
} PPMImage;


typedef struct {
  float position[3];
  float color[4];
} Vertex;

const Vertex Vertices[] = {
  {{1, -1, 0}, {1, 0, 0, 1}},
  {{1, 1, 0}, {0, 1, 0, 1}},
  {{-1, 1, 0}, {0, 0, 1, 1}},
  {{-1, -1, 0}, {0, 0, 0, 1}}
};


const GLubyte Indices[] = {
  0, 1, 2,
  2, 3, 0
};


char* vertex_shader_src =
"attribute vec4 Position;\n"
        "attribute vec4 SourceColor;\n"
        "attribute vec2 SourceTexcoord;\n"
        "uniform vec2 Scale;\n"
        "uniform vec2 Translation;"
        "uniform vec2 Shear;\n"
        "uniform float Rotation;\n"
        "varying vec4 DestinationColor;\n"
        "varying vec2 DestinationTexcoord;\n"
        "mat4 RotationMatrix = mat4( cos(Rotation), -sin(Rotation), 0.0, 0.0,\n"
        "                            sin(Rotation),  cos(Rotation), 0.0, 0.0,\n"
        "                            0.0,            0.0,           1.0, 0.0,\n"
        "                            0.0,            0.0,           0.0, 1.0 );\n"
        "\n"
        "mat4 TranslationMatrix = mat4(1.0, 0.0, 0.0, Translation.x,\n"
        "                              0.0, 1.0, 0.0, Translation.y,\n"
        "                              0.0, 0.0, 1.0, 0.0,\n"
        "                              0.0, 0.0, 0.0, 1.0 );\n"
        "\n"
        "mat4 ScaleMatrix = mat4(Scale.x, 0.0,     0.0, 0.0,\n"
        "                        0.0,     Scale.y, 0.0, 0.0,\n"
        "                        0.0,     0.0,     1.0, 0.0,\n"
        "                        0.0,     0.0,     0.0, 1.0 );\n"
        "\n"
        "mat4 ShearMatrix = mat4(1.0,     Shear.x, 0.0, 0.0,\n"
        "                        Shear.y, 1.0,     0.0, 0.0,\n"
        "                        0.0,     0.0,     1.0, 0.0,\n"
        "                        0.0,     0.0,     0.0, 1.0 );\n"
        "\n"
        "void main(void) {\n"
        "    DestinationColor = SourceColor;\n"
        "    DestinationTexcoord = SourceTexcoord;\n"
        "    gl_Position = Position*ScaleMatrix*ShearMatrix*RotationMatrix*TranslationMatrix;\n"
        "}";


/**
 * The fragment shader for the application. Handles actually mapping the
 * input image onto the geometry.
 */
char* fragment_shader_src =
        "varying vec4 DestinationColor;\n"
        "varying vec2 DestinationTexcoord;\n"
        "uniform sampler2D Texture;\n"
        "\n"
        "void main(void) {\n"
        "    gl_FragColor = texture2D(Texture, DestinationTexcoord) * DestinationColor;\n"
        "}";

static PPMImage *readPPM(const char *filename){
         char buff[16];
         PPMImage *img;
         FILE *fp;
         int c, rgb_comp_color;
         //open PPM file for reading
         fp = fopen(filename, "rb");
         if (!fp) {
              fprintf(stderr, "Unable to open file '%s'\n", filename);
              exit(1);
         }

         //read image format
         if (!fgets(buff, sizeof(buff), fp)) {
              perror(filename);
              exit(1);
         }

    //check the image format
    if (buff[0] != 'P' || buff[1] != '6') {
         fprintf(stderr, "Invalid image format (must be 'P6')\n");
         exit(1);
    }

    //alloc memory form image
    img = (PPMImage *)malloc(sizeof(PPMImage));
    if (!img) {
         fprintf(stderr, "Unable to allocate memory\n");
         exit(1);
    }

    //check for comments
    c = getc(fp);
    while (c == '#') {
    while (getc(fp) != '\n') ;
         c = getc(fp);
    }

    ungetc(c, fp);
    //read image size information
    if (fscanf(fp, "%d %d", &img->x, &img->y) != 2) {
         fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
         exit(1);
    }

    //read rgb component
    if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
         fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
         exit(1);
    }

    //check rgb component depth
    if (rgb_comp_color!= RGB_COMPONENT_COLOR) {
         fprintf(stderr, "'%s' does not have 8-bits components\n", filename);
         exit(1);
    }

    while (fgetc(fp) != '\n') ;
    //memory allocation for pixel data
    img->data = (PPMPixel*)malloc(img->x * img->y * sizeof(PPMPixel));

    if (!img) {
         fprintf(stderr, "Unable to allocate memory\n");
         exit(1);
    }

    //read pixel data from file
    if (fread(img->data, 3 * img->x, img->y, fp) != img->y) {
         fprintf(stderr, "Error loading image '%s'\n", filename);
         exit(1);
    }

    fclose(fp);
    return img;
}

GLint simple_shader(GLint shader_type, char* shader_src) {

  GLint compile_success = 0;

  // generate a new shader
  int shader_id = glCreateShader(shader_type);

  // Tell the shader where the source is
  glShaderSource(shader_id, 1, &shader_src, 0);

  //printf("Compiling shader\n");
  //printf("%s\n", shader_type);

  // Actual compile shader
  glCompileShader(shader_id);

  // check status compile
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_success);

  // If is failed print error
  if (compile_success == GL_FALSE) {
    GLchar message[256];
    glGetShaderInfoLog(shader_id, sizeof(message), 0, &message[0]);
    printf("glCompileShader Error: %s\n", message);
    exit(1);
  }

  return shader_id;
}


int simple_program() {

  GLint link_success = 0;

  GLint program_id = glCreateProgram();
  GLint vertex_shader = simple_shader(GL_VERTEX_SHADER, vertex_shader_src);
  GLint fragment_shader = simple_shader(GL_FRAGMENT_SHADER, fragment_shader_src);

// attach shaders to program
  glAttachShader(program_id, vertex_shader);
  glAttachShader(program_id, fragment_shader);

// link the program
  glLinkProgram(program_id);

  glGetProgramiv(program_id, GL_LINK_STATUS, &link_success);

  if (link_success == GL_FALSE) {
    GLchar message[256];
    glGetProgramInfoLog(program_id, sizeof(message), 0, &message[0]);
    printf("glLinkProgram Error: %s\n", message);
    exit(1);
  }

  return program_id;
}


static void error_callback(int error, const char* description) {
  fputs(description, stderr);
}

// define the linear transformations
float ScaleTo[] = {1.0, 1.0};
float Scale[] = {1.0, 1.0};
float ShearTo[] = {0.0, 0.0};
float Shear[] = {0.0, 0.0};
float TranslationTo[] = {0.0, 0.0};
float Translation[] = {0, 0};
float RotationTo = 0;
float Rotation = 0;


void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
        switch (key) {
            // Scale up the whole image
            case GLFW_KEY_UP:
                ScaleTo[0] += 0.5;
                ScaleTo[1] += 0.5;
                break;
            // Scale down the whole image
            case GLFW_KEY_DOWN:
                ScaleTo[0] -= 0.5;
                ScaleTo[1] -= 0.5;
                if (ScaleTo[0] < 0)
                    ScaleTo[0] = 0;
                if (ScaleTo[1] < 0)
                    ScaleTo[1] = 0;
                break;
            // Translate down in the X direction
            case GLFW_KEY_A:
                TranslationTo[0] -= 0.5;
                break;
            // Translate up in the X direction
            case GLFW_KEY_D:
                TranslationTo[0] += 0.5;
                break;
            // Translate down in the Y direction
            case GLFW_KEY_S:
                TranslationTo[1] -= 0.5;
                break;
            // Translate up in the Y direction
            case GLFW_KEY_W:
                TranslationTo[1] += 0.5;
                break;
            // Add rotation
            case GLFW_KEY_E:
                RotationTo += 0.1;
                break;
            // Subtract rotation
            case GLFW_KEY_Q:
                RotationTo -= 0.1;
                break;
            // Shear up in the X direction
            case GLFW_KEY_J:
                ShearTo[0] += 0.1;
                break;
            // Shear down in the X direction
            case GLFW_KEY_L:
                ShearTo[0] -= 0.1;
                break;
            // Shear up in the Y direction
            case GLFW_KEY_I:
                ShearTo[1] += 0.1;
                break;
            // Shear down in the Y direction
            case GLFW_KEY_K:
                ShearTo[1] -= 0.1;
                break;
            // Reset all values to their original
            case GLFW_KEY_R:
                ScaleTo[0] = 1.0;
                ScaleTo[1] = 1.0;
                ShearTo[0] = 0.0;
                ShearTo[1] = 0.0;
                TranslationTo[0] = 0.0;
                TranslationTo[1] = 0.0;
                RotationTo = 0;
                break;
        }
}

void tween(float *currentValues, float *newValues, int totalEntries){

    for (totalEntries--; totalEntries >= 0; totalEntries--)
        currentValues[totalEntries] += (newValues[totalEntries] - currentValues[totalEntries]) * 0.1;
}


int main(int argc, char** argv) {

  // Checks to make sure that the input file is the correct file
    if(argc != 2) {

        fprintf(stderr, "Error: Incorrect number of arguments; format should be -> [input.ppm]\n");
        return -1;
      }
      // stores input file name 
      char *input_name = argv[1]; 
      
      // Checks to make sure it is a PPM image file
      char* temp_ptr;
      int input_length = strlen(input_name);
      temp_ptr = input_name + (input_length - 4); 
      if(strcmp(temp_ptr, ".ppm") != 0){

        fprintf(stderr, "Error: Input file must be a .ppm file\n");
        return -1;
      }
      
      FILE *fp;
      
      fp = fopen(input_name, "r");
      
      if(fp == NULL){

        fprintf(stderr, "Error: File didn't open properly; filename may be incorrect or file may not exist.\n");
        return -1;
      }   
      fclose(fp);
      
      // block of code allocating memory to global header_buffer before its use
      // int header_buffer = (struct header_data*)malloc(sizeof(struct header_data)); 
      // header_buffer->file_format = (char *)malloc(100);
      // header_buffer->file_height = (char *)malloc(100);
      // header_buffer->file_width = (char *)malloc(100);
      // header_buffer->file_maxcolor = (char *)malloc(100);
      
      // function calls which start the bulk of the program, reading PPM image data into a file
      // Not sure if I need this but I will just comment it out and see if I need it later
      //******

      // readPPM(input_name); // reads and parses header information

      // intermediate image_buffer memory allocation here as image_width and image_height were previously unavailable
      //image_buffer = (image_data *)malloc(sizeof(image_data) * image_width * image_height  + 1); // + 1
      
      // reads and stores image information
      readPPM(input_name); 
      printf("Done reading .ppm file.\n");
      
      
      // Define GLFW variable

      GLint program_id;
      GLuint color_slot;
      GLuint position_slot;
      GLuint texcoord_slot;
      GLuint scale_slot;
      GLuint translation_slot;
      GLuint rotation_slot;
      GLuint shear_slot;
      GLuint vertex_buffer;
      GLuint index_buffer;
      GLuint tex;
      

      glfwSetErrorCallback(error_callback);

      // Initialize GLFW library
      if (!glfwInit())
        return -1;

      glfwDefaultWindowHints();
      glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
      glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

      // Create and open a window
      window = glfwCreateWindow(640,
                                480,
                                "Image viewer",
                                NULL,
                                NULL);

      if (!window) {
        glfwTerminate();
        printf("glfwCreateWindow Error\n");
        exit(1);
      }

      glfwMakeContextCurrent(window);

      program_id = simple_program();

      glUseProgram(program_id);

      position_slot = glGetAttribLocation(program_id, "Position");
      color_slot = glGetAttribLocation(program_id, "SourceColor");
      texcoord_slot = glGetAttribLocation(program_id, "SourceTexcoord");
      scale_slot = glGetUniformLocation(program_id, "Scale");
      translation_slot = glGetUniformLocation(program_id, "Translation");
      rotation_slot = glGetUniformLocation(program_id, "Rotation");
      shear_slot = glGetUniformLocation(program_id, "Shear");
      
      glEnableVertexAttribArray(position_slot);
      glEnableVertexAttribArray(color_slot);
      glEnableVertexAttribArray(texcoord_slot);


      // Create Buffer
      glGenBuffers(1, &vertex_buffer);

      // Map GL_ARRAY_BUFFER to this buffer
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

      // Send the data
      glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

      glGenBuffers(1, &index_buffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);


      // Repeat
      while (!glfwWindowShouldClose(window)) {

        // Tween values
        tween(Scale, ScaleTo, 2);
        tween(Translation, TranslationTo, 2);
        tween(Shear, ShearTo, 2);
        tween(&Rotation, &RotationTo, 1);

        // Send updated values to the shader
        glUniform2f(scale_slot, Scale[0], Scale[1]);
        glUniform2f(translation_slot, Translation[0], Translation[1]);
        glUniform2f(shear_slot, Shear[0], Shear[1]);
        glUniform1f(rotation_slot, Rotation);

        // clear the screen
        glClearColor(0, 104.0/255.0, 55.0/255.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, 640, 480);

        glVertexAttribPointer(position_slot,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              0);

        glVertexAttribPointer(color_slot,
                              4,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              (GLvoid*) (sizeof(float) * 3));
        // Draw everything
        glDrawElements(GL_TRIANGLES,
                       sizeof(Indices) / sizeof(GLubyte),
                       GL_UNSIGNED_BYTE, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
      }
      
      // Finished, close everything up
      glfwDestroyWindow(window);
      glfwTerminate();
      exit(EXIT_SUCCESS);
    }
