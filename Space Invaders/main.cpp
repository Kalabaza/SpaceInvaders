#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Buffer {
public:
    Buffer(size_t width, size_t height) : width_(width), height_(height) {
        image_.resize(width * height);
        ClearImage(0);
    }
    size_t GetWidth() const { return width_; }
    size_t GetHeight() const { return height_; }
    const uint32_t* GetImage() const { return image_.data(); }
    void ClearImage(uint32_t color) {
        for (auto& pixel : image_) {
            pixel = color;
        }
    }
    
private:
    size_t width_, height_;
    std::vector<uint32_t> image_;
};

void ErrorCallback(int error, const char* description) {
    std::cerr << "Error (" << error << "): " << description << std::endl;
}

inline void GLDebug(const char *file, int line) {
    GLenum errorNo;
    while(GL_NO_ERROR != (errorNo = glGetError())){
        std::string errorString;
        switch(errorNo) {
            case GL_INVALID_ENUM: errorString = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: errorString = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: errorString = "GL_INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY: errorString = "GL_OUT_OF_MEMORY"; break;
            default: errorString = "UNKNOWN_ERROR"; break;
        }
        std::cerr << errorString << " - " << file << " : " << line << std::endl;
    }
}

uint32_t RGBToUint32(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 24) | (g << 16) | (b << 8) | 255;
}

static const GLsizei BUFFER_SIZE = 512;

void ValidateShader(GLuint shader, const char* file = nullptr) {
    GLchar buffer[BUFFER_SIZE];
    GLsizei length = 0;

    glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);

    if(length > 0) {
        std::cerr << "Shader " << shader << "(" << file << ") compile error: " << buffer << std::endl;
    }
}

bool ValidateProgram(GLuint program) {
    GLchar buffer[BUFFER_SIZE];
    GLsizei length = 0;

    glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);

    if(length > 0) {
        std::cerr << "Program " << program << " link error: " << buffer << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    glfwSetErrorCallback(ErrorCallback);

    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    constexpr size_t bufferWidth = 224;
    constexpr size_t bufferHeight = 256;

    // Create a windowed mode window and its OpenGL context
    auto* window = glfwCreateWindow(bufferWidth, bufferHeight, "Space Invaders", NULL, NULL);
    if(!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if(GLEW_OK != glewInit()) {
        std::cerr << "Error initializing GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    int glVersion[2] = {0, 0};
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

    GLDebug(__FILE__, __LINE__);

    std::cout << "Using OpenGL: " << glVersion[0] << "." << glVersion[1] << std::endl;
    std::cout << "Renderer used: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Shading Language: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    Buffer buffer(bufferWidth, bufferHeight);
    
    const char* vertexShader =
R"(
#version 330

noperspective out vec2 TexCoord;

void main(void) {
    TexCoord.x = (2 == gl_VertexID) ? 2.0 : 0.0;
    TexCoord.y = (1 == gl_VertexID) ? 2.0 : 0.0;

    gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);
}
)";
    const char* fragmentShader =
R"(
#version 330

uniform sampler2D buffer;
noperspective in vec2 TexCoord;

out vec3 outColor;

void main(void) {
    outColor = texture(buffer, TexCoord).rgb;
}
)";
    
    GLuint fullscreenTriangleVAO;
    glGenVertexArrays(1, &fullscreenTriangleVAO);
    
    GLuint shaderId = glCreateProgram();

    // Create vertex shader
    {
        GLuint shader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(shader, 1, &vertexShader, 0);
        glCompileShader(shader);
        ValidateShader(shader, vertexShader);
        glAttachShader(shaderId, shader);

        glDeleteShader(shader);
    }

    // Create fragment shader
    {
        GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(shader, 1, &fragmentShader, 0);
        glCompileShader(shader);
        ValidateShader(shader, fragmentShader);
        glAttachShader(shaderId, shader);

        glDeleteShader(shader);
    }

    glLinkProgram(shaderId);

    if(!ValidateProgram(shaderId))
    {
        std::cerr << "Error while validating program" << std::endl;
        glfwTerminate();
        glDeleteVertexArrays(1, &fullscreenTriangleVAO);
        return -1;
    }
    
    glUseProgram(shaderId);
    
    GLuint bufferTexture;
    glGenTextures(1, &bufferTexture);
    
    glBindTexture(GL_TEXTURE_2D, bufferTexture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB8,
        static_cast<GLsizei>(buffer.GetWidth()), static_cast<GLsizei>(buffer.GetHeight()), 0,
        GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.GetImage()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    GLint location = glGetUniformLocation(shaderId, "buffer");
    glUniform1i(location, 0);
    
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    
    glBindVertexArray(fullscreenTriangleVAO);

    uint32_t clearColor = RGBToUint32(0, 128, 0);
    while (!glfwWindowShouldClose(window)) {
        buffer.ClearImage(clearColor);
        
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0,
            static_cast<GLsizei>(buffer.GetWidth()), static_cast<GLsizei>(buffer.GetHeight()),
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
            buffer.GetImage()
        );
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
