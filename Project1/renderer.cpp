#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include "shader_read.h"

float iTime = 0.0f;          // 时间
float iMouseX = 0.0f, iMouseY = 0.0f; // 鼠标位置（归一化）
bool iMousePressed = false;

// 读取着色器文件
std::string vertexShaderCode = readShaderFile("mlpshader.vert");
std::string fragmentShaderCode = readShaderFile("mlpshader.frag");

// 转换为 const char*（OpenGL 接口要求）
const char* vertexShaderSource = vertexShaderCode.c_str();
const char* fragmentShaderSource = fragmentShaderCode.c_str();

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// GLFW 鼠标位置回调：更新归一化鼠标坐标
void mousePosCallback(GLFWwindow* window, double xpos, double ypos) {
    // 获取窗口尺寸
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    // 转换为归一化坐标（0 ~ 1），Y 轴翻转（匹配着色器逻辑）
    iMouseX = static_cast<float>(xpos) / width;
    iMouseY = 1.0f - static_cast<float>(ypos) / height; // 翻转Y轴
}

// GLFW鼠标按键回调：更新按下状态
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        iMousePressed = (action == GLFW_PRESS);
    }
}


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "renderer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "失败！" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    // 加载 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "失败！" << std::endl;
        return -1;
    }

    // 着色器构建与编译
	// 顶点着色器
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // 纠错
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "顶点着色器编译失败！" << std::endl;
		std::cout << infoLog << std::endl;
	}

	// 像素着色器
	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// 纠错
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "像素着色器编译失败！" << std::endl;
		std::cout << infoLog << std::endl;
	}

	// 着色器程序
	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// 纠错
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "着色器程序链接失败！" << std::endl;
		std::cout << infoLog << std::endl;
	}

	// 使用着色器程序
	glUseProgram(shaderProgram);

	// 删除着色器对象
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

    // 顶点
    float quadVertices[] = {
        // 位置          // 纹理坐标
        -1.0f,  1.0f,    0.0f, 1.0f, // 左上
        -1.0f, -1.0f,    0.0f, 0.0f, // 左下
        1.0f, -1.0f,    1.0f, 0.0f, // 右下
        1.0f,  1.0f,    1.0f, 1.0f  // 右上
    };
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
	unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // 绑定
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // 索引缓冲（绘制两个三角形组成四边形）
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 位置属性（layout 0）
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 纹理坐标属性（layout 1）
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLint iTimeLoc = glGetUniformLocation(shaderProgram, "iTime");
    GLint iResolutionLoc = glGetUniformLocation(shaderProgram, "iResolution");
    GLint iMouseLoc = glGetUniformLocation(shaderProgram, "iMouse");
    GLint iChannel0Loc = glGetUniformLocation(shaderProgram, "iChannel0");

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        float deltaTime = static_cast<float>(glfwGetTime());
        glfwSetTime(0.0);
        iTime += deltaTime;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glUniform1f(iTimeLoc, iTime);
        glUniform2f(iResolutionLoc, 800.0f, 600.0f);
        glUniform4f(
            iMouseLoc,
            iMouseX,
            iMouseY,
            iMousePressed ? 1.0f : 0.0f,
            iMousePressed ? 1.0f : 0.0f
        );
        glUniform1i(iChannel0Loc, 0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();

    return 0;
}