#version 330 core
precision highp float;

in vec3 v_rayOrigin;
in vec3 v_rayDirection;

out vec4 FragColor;

const float MAX_MARCH_DIST = 50.0;   // 最大步进距离
const float MARCH_STEP = 0.05;       // 步进精度
const float VOLUME_THRESHOLD = -10.0;// 体积判定阈值
const vec3 BG_COLOR = vec3(0.1);     // 背景色

vec3 hsv2rgb(vec3 c) 
{
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 kernel_with_iter(vec3 ver) 
{
    vec3 a = ver;
    float b, c, d, e;
    int iter = 0; 
    for(int i = 0; i < 5; i++) 
    {
        iter = i;
        b = length(a);
        if(b < 1e-6) break;
        c = atan(a.y, a.x) * 8.0;
        e = 1.0 / b;
        d = acos(clamp(a.z * e, -1.0, 1.0)) * 8.0;
        b = pow(b, 8.0);
        a = vec3(b*sin(d)*cos(c), b*sin(d)*sin(c), b*cos(d)) + ver;
        if(b > 6.0) break;
    }
    float sdf = 4.0 - dot(a, a);
    return vec2(sdf, float(iter)); 
}

vec4 rayMarch(vec3 rayOrigin, vec3 rayDir) 
{
    vec3 col = vec3(0.0);
    float alpha = 0.0;
    float t = 0.0;

    for(int i = 0; i < int(MAX_MARCH_DIST / MARCH_STEP); i++) 
    {
        if(t > MAX_MARCH_DIST || alpha >= 1.0) break;
        
        vec3 p = rayOrigin + rayDir * t;
        vec2 kernelRes = kernel_with_iter(p);
        float sdf = kernelRes.x;
        float iter = kernelRes.y; // 迭代次数（0~4）

        if(sdf > VOLUME_THRESHOLD) 
        {
            float hue = smoothstep(0.2, 0.9, iter / 4.0);
            vec3 color = hsv2rgb(vec3(hue, 0.9, 0.9));
            float density = smoothstep(0.0, 4.0, sdf);
            col += color * density * (1.0 - alpha) * 0.1;
            alpha += density * 0.05;
        }
        t += MARCH_STEP;
    }
    return vec4(col, alpha);
}

void main() 
{
    vec4 finalColor = rayMarch(v_rayOrigin, v_rayDirection);
    vec3 mixedColor = finalColor.rgb + BG_COLOR * (1.0 - finalColor.a);
    FragColor = vec4(mixedColor, 1.0);
}