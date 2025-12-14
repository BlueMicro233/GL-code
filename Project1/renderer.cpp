#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLFWwindow* window = nullptr;
int width = 1280, height = 720;
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f); // 相机初始位置
glm::vec2 lastMousePos = glm::vec2(width / 2, height / 2);
float yaw = -90.0f, pitch = 0.0f;
bool firstMouse = true;

std::string readShader(const char* path) 
{
    std::ifstream file(path);
    if (!file) { std::cerr << "Failed to open shader: " << path << std::endl; return ""; }
    std::stringstream ss; ss << file.rdbuf();
    return ss.str();
}

GLuint createProgram(const char* vertPath, const char* fragPath) 
{
    // 1. 编译顶点着色器
    std::string vertCode = readShader(vertPath);
    const char* vCode = vertCode.c_str();
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vCode, nullptr);
    glCompileShader(vertShader);
    // 检查编译错误
    GLint success; char info[512];
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(vertShader, 512, nullptr, info); std::cerr << "VERT ERROR: " << info << std::endl; }

    // 2. 编译片段着色器
    std::string fragCode = readShader(fragPath);
    const char* fCode = fragCode.c_str();
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fCode, nullptr);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(fragShader, 512, nullptr, info); std::cerr << "FRAG ERROR: " << info << std::endl; }

    // 3. 链接程序
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) { glGetProgramInfoLog(program, 512, nullptr, info); std::cerr << "LINK ERROR: " << info << std::endl; }

    // 清理临时对象
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return program;
}

// 初始化全屏四边形（VAO/VBO）
void initQuad(GLuint& VAO, GLuint& VBO) 
{
    float quad[] = { -1,1, -1,-1, 1,-1, 1,1 }; // NDC 坐标
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
}

// 鼠标回调：控制相机视角
void mouseCallback(GLFWwindow* win, double x, double y) 
{
    if (firstMouse) { lastMousePos = glm::vec2(x, y); firstMouse = false; }
    float xOff = x - lastMousePos.x;
    float yOff = lastMousePos.y - y;
    lastMousePos = glm::vec2(x, y);

    xOff *= 0.1f; yOff *= 0.1f;
    yaw += xOff; pitch += yOff;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);
}

// 键盘回调：控制相机移动
void keyCallback(GLFWwindow* win, int key, int sc, int act, int mod) 
{
    float speed = 0.1f;
    if (key == GLFW_KEY_ESCAPE && act == GLFW_PRESS) glfwSetWindowShouldClose(win, true);
    if (key == GLFW_KEY_W) cameraPos += speed * glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw)));
    if (key == GLFW_KEY_S) cameraPos -= speed * glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw)));
    if (key == GLFW_KEY_A) cameraPos -= speed * glm::normalize(glm::cross(glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw))), glm::vec3(0, 1, 0)));
    if (key == GLFW_KEY_D) cameraPos += speed * glm::normalize(glm::cross(glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw))), glm::vec3(0, 1, 0)));
}

int main() 
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(width, height, "renderer", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        std::cerr << "GLAD init failed!" << std::endl;
        return -1;
    }

    GLuint program = createProgram("stress_test.vert", "stress_test.frag");
    GLuint VAO, VBO;
    initQuad(VAO, VBO);


    GLint uViewLoc = glGetUniformLocation(program, "u_view");
    GLint uProjLoc = glGetUniformLocation(program, "u_proj");
    GLint uInvViewProjLoc = glGetUniformLocation(program, "u_invViewProj");
    GLint uCameraPosLoc = glGetUniformLocation(program, "u_cameraPos");


    while (!glfwWindowShouldClose(window)) 
    {

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + glm::vec3(cos(glm::radians(yaw)) * cos(glm::radians(pitch)), sin(glm::radians(pitch)), sin(glm::radians(yaw)) * cos(glm::radians(pitch))),
            glm::vec3(0, 1, 0)
        );
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);
        glm::mat4 invViewProj = glm::inverse(proj * view);

        // 设置 Uniform
        glUseProgram(program);
        glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(uInvViewProjLoc, 1, GL_FALSE, glm::value_ptr(invViewProj));
        glUniform3fv(uCameraPosLoc, 1, glm::value_ptr(cameraPos));

        // 绘制全屏四边形（触发光线步进）
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 交换缓冲区 + 事件处理
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}