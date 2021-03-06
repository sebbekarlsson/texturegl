#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cglm/cglm.h>
#include <cglm/call.h>
#include <math.h>
#include <png.h>


/**
 * Vertices to be used for drawing triangle.
 */
static const struct
{
    float x, y;
    float r, g, b;
} vertices[3] =
{
    { -0.6f, -0.4f, 1.f, 0.f, 0.f },
    {  0.6f, -0.4f, 0.f, 1.f, 0.f },
    {   0.f,  0.6f, 0.f, 0.f, 1.f }
};

/**
 * Texture coordinates for texture on triangle.
 */
static float texCoords[] = {
    0.0f, 0.0f,  // lower-left corner  
    1.0f, 0.0f,  // lower-right corner
    0.5f, 1.0f   // top-center corner
};

/**
 * Capture errors from glfw.
 */
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

/**
 * Capture key callbacks from glfw
 */
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

/**
 * Get a texture as an unsigned integer
 *
 * @param const char* path
 * @return unsigned int
 */
static unsigned int get_texture(const char* path)
{
    /**
     * Texture will be stored in this unsigned integer.
     */
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    /**
     * This is texture parameters,
     * you can see them as "effects" that are applied to the
     * loaded texture.
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /**
     * Using libpng to read & load a .png file
     */
    png_image image = {};
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_file(&image, path))
        fprintf(stderr, "Could not read file `%s`: %s\n", path, image.message);

    image.format = PNG_FORMAT_RGBA;

    /**
     * image_pixels in this case is the data we are interested in and which
     * is used for drawing later.
     */
    uint32_t *image_pixels = malloc(sizeof(uint32_t) * image.width * image.height);
    if (image_pixels == NULL)
        fprintf(stderr, "Could not allocate memory for an image\n");

    /**
     * Check for errors
     */
    if (!png_image_finish_read(&image, NULL, image_pixels, 0, NULL))
        fprintf(stderr, "libpng error: %s\n", image.message);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 image.width,
                 image.height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 image_pixels);

    glGenerateMipmap(GL_TEXTURE_2D);

    return texture;
}

int main(int argc, char* argv[])
{
    glfwSetErrorCallback(error_callback);

    /**
     * Initialize glfw to be able to use it.
     */
    if (!glfwInit())
        perror("Failed to initialize glfw.\n");

    /**
     * Setting some parameters to the window,
     * using OpenGL 3.3
     */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_FLOATING, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    /**
     * Creating our window
     */
    GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
    if (!window)
        perror("Failed to create window.\n");

    glfwSetKeyCallback(window, key_callback);

    /**
     * Enable OpenGL as current context
     */
    glfwMakeContextCurrent(window);

    /** 
     * Initialize glew and check for errors
     */
    GLenum err = glewInit();
    if (GLEW_OK != err)
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));

    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);

    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location, texcoord_location;

    /**
     * Vertex Shader
     */
    static const char* vertex_shader_text =
        "#version 330 core\n"
        "uniform mat4 MVP;\n"
        "attribute vec3 vCol;\n"
        "attribute vec2 vPos;\n"
        "attribute vec2 aTexCoord;\n"
        "varying vec3 color;\n"
        "out vec2 TexCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
        "    TexCoord = aTexCoord;"
        "    color = vCol;\n"
        "}\n";
    
    /**
     * Fragment Shader
     */    
    static const char* fragment_shader_text =
        "#version 330 core\n"
        "varying vec3 color;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D ourTexture;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture(ourTexture, TexCoord);\n"
        "}\n"; 

    int success;
    char infoLog[512];
 
    /**
     * Compile vertex shader and check for errors
     */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        printf("Vertex Shader Error\n");
        glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
        perror(infoLog);
    }

    /**
     * Compile fragment shader and check for errors
     */ 
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        printf("Fragment Shader Error\n");
        glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
        perror(infoLog);
    }

    /**
     * Create shader program and check for errors
     */ 
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        perror(infoLog);
    }

    /**
     * Grab locations from shader
     */ 
    mvp_location = glGetUniformLocation(program, "MVP");
    vpos_location = glGetAttribLocation(program, "vPos");
    vcol_location = glGetAttribLocation(program, "vCol");
    texcoord_location = glGetAttribLocation(program, "aTexCoord");

    glBindVertexArray(VAO);
    
    /**
     * Buffer / send our vertices
     */
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
 
    /**
     * Tell OpenGL where the data is stored in the buffer
     */
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) 0);
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) (sizeof(float) * 2));

    /**
     * Create and bind texture
     */
    unsigned int texture = get_texture("rainbow.png");
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    /**
     * Buffer / send our texture coordinates
     */
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(texcoord_location);
    glVertexAttribPointer(texcoord_location, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);

    /**
     * Main loop
     */
    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        mat4 p, mvp;
        double t = glfwGetTime();

        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        
        mat4 m = GLM_MAT4_IDENTITY_INIT; 

        glm_translate(m, (vec3){ 0, cos(t), 0 });
        
        glm_ortho_default(width / (float) height, p);
        glm_mat4_mul(p, m, mvp);

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
   
    glfwDestroyWindow(window); 
    glfwTerminate();
    return 0;
}
