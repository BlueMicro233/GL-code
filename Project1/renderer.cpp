#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

float iTime = 0.0f;          // 时间
float iMouseX = 0.0f, iMouseY = 0.0f; // 鼠标位置（归一化）

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord; // 传递纹理坐标给片段着色器

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    texCoord = aTexCoord;
}
)";

// 黑洞渲染着色器（适配 OpenGL 330 core）
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 texCoord; // 从顶点着色器传入的纹理坐标

// Shadertoy 核心宏定义
#define AA 1  // 改为 2 提升抗锯齿质量
#define _Speed 3.0  // 吸积盘旋转速度
#define _Steps  12. // 吸积盘纹理层数
#define _Size 0.3   // 黑洞大小

// Uniform 变量（对应 Shadertoy 的内置变量）
uniform float iTime;       // 时间
uniform vec2 iResolution;  // 窗口分辨率
uniform vec2 iMouse;       // 鼠标位置（归一化）
uniform sampler2D iChannel0; // 纹理通道（星云图，这里简化为噪声）

// 哈希函数
float hash(float x){ return fract(sin(x)*152754.742);}
float hash(vec2 x){	return hash(x.x + hash(x.y));}

// 价值噪声
float value(vec2 p, float f)
{
    float bl = hash(floor(p*f + vec2(0.,0.)));
    float br = hash(floor(p*f + vec2(1.,0.)));
    float tl = hash(floor(p*f + vec2(0.,1.)));
    float tr = hash(floor(p*f + vec2(1.,1.)));
    
    vec2 fr = fract(p*f);    
    fr = (3.0 - 2.0*fr)*fr*fr;	
    float b = mix(bl, br, fr.x);	
    float t = mix(tl, tr, fr.x);
    return mix(b, t, fr.y);
}

// 背景（星空+星云，简化 iChannel0 为噪声）
vec4 background(vec3 ray)
{
    vec2 uv = ray.xy;
    
    if( abs(ray.x) > 0.5)
        uv.x = ray.z;
    else if( abs(ray.y) > 0.5)
        uv.y = ray.z;

    // 星空噪声
    float brightness = value(uv*3.0, 100.0);
    float color = value(uv*2.0, 20.0); 
    brightness = pow(brightness, 256.0);
    brightness = brightness*100.0;
    brightness = clamp(brightness, 0.0, 1.0);
    
    vec3 stars = brightness * mix(vec3(1.0, 0.6, 0.2), vec3(0.2, 0.6, 1.0), color);

    // 简化星云（替代 iChannel0 纹理采样）
    vec4 nebulae = vec4(value(uv*1.5, 50.0) * 0.2);
    nebulae.xyz = pow(nebulae.xyz, vec3(4.0));
    nebulae.xyz += stars;
    
    return nebulae;
}

// 光线步进吸积盘
vec4 raymarchDisk(vec3 ray, vec3 zeroPos)
{
    vec3 position = zeroPos;      
    float lengthPos = length(position.xz);
    float dist = min(1.0, lengthPos*(1.0/_Size) *0.5) * _Size * 0.4 *(1.0/_Steps) /( abs(ray.y) );

    position += dist*_Steps*ray*0.5;     

    vec2 deltaPos;
    deltaPos.x = -zeroPos.z*0.01 + zeroPos.x;
    deltaPos.y = zeroPos.x*0.01 + zeroPos.z;
    deltaPos = normalize(deltaPos - zeroPos.xz);
    
    float parallel = dot(ray.xz, deltaPos);
    parallel /= sqrt(lengthPos);
    parallel *= 0.5;
    float redShift = parallel +0.3;
    redShift *= redShift;
    redShift = clamp(redShift, 0.0, 1.0);
    
    float disMix = clamp((lengthPos - _Size * 2.0)*(1.0/_Size)*0.24, 0.0, 1.0);
    vec3 insideCol = mix(vec3(1.0,0.8,0.0), vec3(0.5,0.13,0.02)*0.2, disMix);
    insideCol *= mix(vec3(0.4, 0.2, 0.1), vec3(1.6, 2.4, 4.0), redShift);
    insideCol *= 1.25;

    vec4 o = vec4(0.0);

    for(float i = 0.0 ; i < _Steps; i++)
    {                      
        position -= dist * ray ;  

        float intensity = clamp(1.0 - abs((i - 0.8) * (1.0/_Steps) * 2.0), 0.0, 1.0); 
        float lengthPos = length(position.xz);
        float distMult = 1.0;

        distMult *= clamp((lengthPos - _Size * 0.75) * (1.0/_Size) * 1.5, 0.0, 1.0);        
        distMult *= clamp((_Size * 10.0 - lengthPos) * (1.0/_Size) * 0.20, 0.0, 1.0);
        distMult *= distMult;

        float u = lengthPos + iTime*_Size*0.3 + intensity * _Size * 0.2;

        vec2 xy ;
        float rot = mod(iTime*_Speed, 8192.0);
        xy.x = -position.z*sin(rot) + position.x*cos(rot);
        xy.y = position.x*sin(rot) + position.z*cos(rot);

        float x = abs(xy.x/(xy.y));         
        float angle = 0.02*atan(x);
  
        const float f = 70.0;
        float noise = value(vec2(angle, u * (1.0/_Size) * 0.05), f);
        noise = noise*0.66 + 0.33*value(vec2(angle, u * (1.0/_Size) * 0.05), f*2.0);     

        float extraWidth = noise * 1.0 * (1.0 - clamp(i * (1.0/_Steps)*2.0 - 1.0, 0.0, 1.0));
        float alpha = clamp(noise*(intensity + extraWidth)*( (1.0/_Size) * 10.0  + 0.01 ) * dist * distMult , 0.0, 1.0);

        vec3 col = 2.0*mix(vec3(0.3,0.2,0.15)*insideCol, insideCol, min(1.0, intensity*2.0));
        o = clamp(vec4(col*alpha + o.rgb*(1.0-alpha), o.a*(1.0-alpha) + alpha), vec4(0.0), vec4(1.0));

        lengthPos *= (1.0/_Size);
        o.rgb += redShift*(intensity*1.0 + 0.5)* (1.0/_Steps) * 100.0*distMult/(lengthPos*lengthPos);
    }  
 
    o.rgb = clamp(o.rgb - 0.005, 0.0, 1.0);
    return o ;
}

// 向量旋转函数
void Rotate(inout vec3 vector, vec2 angle)
{
    vector.yz = cos(angle.y)*vector.yz + sin(angle.y)*vec2(-1,1)*vector.zy;
    vector.xz = cos(angle.x)*vector.xz + sin(angle.x)*vec2(-1,1)*vector.zx;
}

void main()
{
    vec2 fragCoord = texCoord * iResolution; // 转换为Shadertoy的fragCoord
    vec4 colOut = vec4(0.0);
    
    vec2 fragCoordRot;
    fragCoordRot.x = fragCoord.x*0.985 + fragCoord.y * 0.174;
    fragCoordRot.y = fragCoord.y*0.985 - fragCoord.x * 0.174;
    fragCoordRot += vec2(-0.06, 0.12) * iResolution.xy;
    
    // 抗锯齿循环
    for( int j=0; j<AA; j++ )
    for( int i=0; i<AA; i++ )
    {
        // 相机初始化
        vec3 ray = normalize(vec3((fragCoordRot - iResolution.xy*0.5 + vec2(i,j)/float(AA))/iResolution.x, 1.0)); 
        vec3 pos = vec3(0.0,0.05,-(20.0*iMouse.xy/iResolution.y-10.0)*(20.0*iMouse.xy/iResolution.y-10.0)*0.05); 
        vec2 angle = vec2(iTime*0.1, 0.2);      
        angle.y = (2.0*iMouse.y/iResolution.y)*3.14159 + 0.1 + 3.14159;
        float dist = length(pos);
        
        Rotate(pos, angle);
        angle.xy -= min(0.3/dist , 3.14159) * vec2(1, 0.5);
        Rotate(ray, angle);

        vec4 col = vec4(0.0); 
        vec4 glow = vec4(0.0); 
        vec4 outCol = vec4(100.0);

        // 光线步进循环
        for(int disks = 0; disks< 20; disks++)
        {
            for (int h = 0; h < 6; h++)
            {
                float dotpos = dot(pos, pos);
                float invDist = inversesqrt(dotpos);
                float centDist = dotpos * invDist; 	
                float stepDist = 0.92 * abs(pos.y / (ray.y + 1e-6)); // 避免除0
                float farLimit = centDist * 0.5;
                float closeLimit = centDist*0.1 + 0.05*centDist*centDist*(1.0/_Size);
                stepDist = min(stepDist, min(farLimit, closeLimit));
				
                float invDistSqr = invDist * invDist;
                float bendForce = stepDist * invDistSqr * _Size * 0.625;
                ray = normalize(ray - (bendForce * invDist )*pos);
                pos += stepDist * ray; 
                
                glow += vec4(1.2,1.1,1.0, 1.0) * (0.01*stepDist * invDistSqr * invDistSqr * clamp(centDist*2.0 - 1.2, 0.0, 1.0));
            }

            float dist2 = length(pos);

            // 光线被黑洞吞噬
            if(dist2 < _Size * 0.1)
            {
                outCol = vec4(col.rgb * col.a + glow.rgb * (1.0-col.a), 1.0);
                break;
            }
            // 光线逃逸到背景
            else if(dist2 > _Size * 1000.0)
            {                   
                vec4 bg = background(ray);
                outCol = vec4(col.rgb*col.a + bg.rgb*(1.0-col.a) + glow.rgb*(1.0-col.a), 1.0);       
                break;
            }
            // 光线击中吸积盘
            else if (abs(pos.y) <= _Size * 0.002 )
            {                             
                vec4 diskCol = raymarchDisk(ray, pos);
                pos.y = 0.0;
                pos += abs(_Size * 0.001 / (ray.y + 1e-6)) * ray;  
                col = vec4(diskCol.rgb*(1.0-col.a) + col.rgb, col.a + diskCol.a*(1.0-col.a));
            }	
        }
   
        if(outCol.r == 100.0)
            outCol = vec4(col.rgb + glow.rgb*(col.a + glow.a), 1.0);

        colOut += outCol / float(AA*AA);
    }
    
    // 伽马校正
    colOut.rgb = pow(colOut.rgb, vec3(0.6));
    FragColor = colOut;
}
)";

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