#version 330 core
layout (location = 0) in vec2 aPos; // 全屏四边形的 NDC 坐标

out vec2 v_uv;
out vec3 v_rayOrigin;
out vec3 v_rayDirection;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_invViewProj;
uniform vec3 u_cameraPos;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    v_uv = aPos; // 传递 NDC 坐标（用于计算射线）
    
    // 1. 射线起点 = 相机位置
    v_rayOrigin = u_cameraPos;
    
    // 2. 计算世界空间射线方向（核心：从 NDC 反推世界空间射线）
    vec4 clipPos = vec4(aPos, -1.0, 1.0); // NDC 远平面点
    vec4 worldPos = u_invViewProj * clipPos; // 反推世界空间点
    v_rayDirection = normalize(worldPos.xyz / worldPos.w - v_rayOrigin);
}