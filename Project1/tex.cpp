#include <glad/glad.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static int height, width, nrChannels;
unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);

unsigned int texture;