#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

// 读取着色器文件内容
std::string readShaderFile(const char* filePath)
{
    std::string shaderCode;
    std::ifstream shaderFile;

    // 启用异常处理，捕获文件读取错误
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        // 打开文件
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        // 读取文件内容到字符串流
        shaderStream << shaderFile.rdbuf();
        // 关闭文件
        shaderFile.close();
        // 转换为字符串
        shaderCode = shaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "着色器文件读取失败：" << filePath << "\n错误信息：" << e.what() << std::endl;
    }

    return shaderCode;
}