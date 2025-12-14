#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include "shader_read.h"

float iTime = 0.0f;          // 时间
float iMouseX = 0.0f, iMouseY = 0.0f; // 鼠标位置（归一化）

// 读取着色器文件
std::string vertexShaderCode = readShaderFile("blackhole.vert");
std::string fragmentShaderCode = readShaderFile("blackhole.frag");

// 转换为const char*（OpenGL 接口要求）
const char* vertexShaderSource = vertexShaderCode.c_str();
const char* fragmentShaderSource = fragmentShaderCode.c_str();

// 创建空纹理（绑定iChannel0，避免着色器采样未绑定纹理报错）
unsigned int createDummyTexture()
{
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // 填充1x1的白色像素
    unsigned char data[] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    iMouseX = static_cast<float>(xpos) / 800;
    iMouseY = static_cast<float>(ypos) / 600;
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
    glfwSetCursorPosCallback(window, mouse_callback);

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
    // 0. 绑定
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

    // 3. 创建并绑定 dummy 纹理（修复 iChannel0 未绑定问题）
    unsigned int dummyTex = createDummyTexture();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, dummyTex);

    // 3. 获取Uniform位置（用于传递数据）
    GLint iTimeLoc = glGetUniformLocation(shaderProgram, "iTime");
    GLint iResolutionLoc = glGetUniformLocation(shaderProgram, "iResolution");
    GLint iMouseLoc = glGetUniformLocation(shaderProgram, "iMouse");
    GLint iChannel0Loc = glGetUniformLocation(shaderProgram, "iChannel0");

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 输入
        processInput(window);

        iTime += glfwGetTime();
        glfwSetTime(0.0); // 重置 GLFW 时间，避免溢出

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 使用着色器程序
        glUseProgram(shaderProgram);

        // 传 Uniform 变量
        glUniform1f(iTimeLoc, iTime);
        glUniform2f(iResolutionLoc, 800, 600);
        glUniform2f(iMouseLoc, iMouseX * 800, iMouseY * 600);
        glUniform1i(iChannel0Loc, 0); // 绑定纹理单元 0 到 iChannel0

        // 绘制全屏四边形
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 检查并调用事件，交换缓冲
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &dummyTex);

    glfwTerminate();

    return 0;
}