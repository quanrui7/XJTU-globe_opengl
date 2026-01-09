#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

using namespace std;

// 窗口尺寸
int WIDTH = 800;
int HEIGHT = 600;

// 地球参数
float rotationY = 0.0f;
float rotationX = 0.0f;
float zoom = 1.0f;
int lastMouseX = 0, lastMouseY = 0;
bool mouseLeftDown = false;

// 纹理ID
GLuint textureID = 0;
GLuint shadowTextureID = 0; // 阴影纹理

// 光源参数
int currentLightPosition = 0; // 当前光源位置索引
bool lightEnabled = true;     // 光照是否启用
bool shadowEnabled = true;    // 阴影是否启用
float shadowIntensity = 0.6f; // 阴影强度

// 地面参数
float floorY = -1.5f;         // 地面Y坐标
float floorSize = 4.0f;       // 地面大小

// 简化后的光源位置
struct LightPosition {
    float x, y, z;  // 位置
    const char* name; // 位置描述
};

vector<LightPosition> lightPositions = {
    {5.0f, 5.0f, 5.0f, "右上前方"},
    {-5.0f, 5.0f, 5.0f, "左上前方"},
    {0.0f, 10.0f, 0.0f, "正上方"},
    {0.0f, -10.0f, 0.0f, "正下方"},
    {10.0f, 0.0f, 0.0f, "正右侧"},
    {-10.0f, 0.0f, 0.0f, "正左侧"},
    {0.0f, 0.0f, 10.0f, "正前方"},
    {0.0f, 0.0f, -10.0f, "正后方"}
};

// ========================
// BMP文件加载函数
// ========================

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
    uint32_t dibSize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

bool loadBMPFile(const char* filename, unsigned char*& data, int& width, int& height) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "错误: 无法打开文件 " << filename << endl;
        return false;
    }
    
    BMPHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (header.type != 0x4D42) {
        cerr << "错误: 不是有效的BMP文件" << endl;
        return false;
    }
    
    if (header.bitsPerPixel != 24) {
        cerr << "错误: 只支持24位BMP文件" << endl;
        return false;
    }
    
    width = header.width;
    height = header.height;
    
    file.seekg(header.offset, ios::beg);
    
    int rowPadding = (4 - (width * 3) % 4) % 4;
    data = new unsigned char[width * height * 3];
    
    for (int y = 0; y < height; ++y) {
        int fileY = height - 1 - y;
        file.read(reinterpret_cast<char*>(&data[fileY * width * 3]), width * 3);
        file.seekg(rowPadding, ios::cur);
    }
    
    for (int i = 0; i < width * height * 3; i += 3) {
        swap(data[i], data[i + 2]);
    }
    
    file.close();
    return true;
}

void createDefaultTexture() {
    unsigned char pixels[64 * 64 * 3];
    
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            int idx = (y * 64 + x) * 3;
            
            if ((x / 8 + y / 8) % 2 == 0) {
                pixels[idx] = 100;
                pixels[idx+1] = 150;
                pixels[idx+2] = 50;
            } else {
                pixels[idx] = 30;
                pixels[idx+1] = 60;
                pixels[idx+2] = 150;
            }
        }
    }
    
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

bool loadTextureFromJPG(const char* filename) {
    cout << "尝试加载JPG图片: " << filename << endl;
    
    ifstream testFile(filename);
    if (!testFile.good()) {
        cerr << "错误: 文件 " << filename << " 不存在" << endl;
        return false;
    }
    testFile.close();
    
    string command = "python3 create_world_bmp.py \"" + string(filename) + "\"";
    int result = system(command.c_str());
    
    if (result != 0) {
        cerr << "警告: JPG转换失败" << endl;
        return false;
    }
    
    string bmpFile = "world.bmp";
    ifstream bmpTest(bmpFile);
    if (!bmpTest.good()) {
        cerr << "错误: 转换后文件 " << bmpFile << " 不存在" << endl;
        return false;
    }
    bmpTest.close();
    
    unsigned char* imageData = nullptr;
    int width = 0, height = 0;
    
    if (loadBMPFile(bmpFile.c_str(), imageData, width, height)) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
        
        delete[] imageData;
        cout << "✓ 成功加载并转换图片: " << filename << " -> " << bmpFile << endl;
        cout << "  尺寸: " << width << "x" << height << endl;
        return true;
    }
    
    return false;
}

void initTexture() {
    cout << "初始化纹理..." << endl;
    
    if (loadTextureFromJPG("earth.jpg")) {
        cout << "✓ 使用 earth.jpg 作为地球纹理" << endl;
    } 
    else if (loadTextureFromJPG("world.jpg")) {
        cout << "✓ 使用 world.jpg 作为地球纹理" << endl;
    }
    else {
        cout << "尝试直接加载BMP文件..." << endl;
        const char* possibleFiles[] = {"world.bmp", "earth.bmp", "map.bmp", "texture.bmp", NULL};
        
        bool loaded = false;
        for (int i = 0; possibleFiles[i]; i++) {
            ifstream file(possibleFiles[i]);
            if (file.good()) {
                file.close();
                unsigned char* imageData = nullptr;
                int width = 0, height = 0;
                
                if (loadBMPFile(possibleFiles[i], imageData, width, height)) {
                    glGenTextures(1, &textureID);
                    glBindTexture(GL_TEXTURE_2D, textureID);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
                    
                    delete[] imageData;
                    cout << "✓ 成功加载BMP文件: " << possibleFiles[i] << endl;
                    loaded = true;
                    break;
                }
            }
        }
        
        if (!loaded) {
            cout << "✗ 无法加载任何图片文件，使用默认棋盘格纹理" << endl;
            createDefaultTexture();
        }
    }
}

// ========================
// 创建阴影纹理（软阴影）
// ========================

void createSoftShadowTexture() {
    const int texSize = 256;
    unsigned char* pixels = new unsigned char[texSize * texSize * 4];
    
    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            int idx = (y * texSize + x) * 4;
            
            // 计算到纹理中心的距离
            float dx = (x - texSize/2.0f) / (texSize/2.0f);
            float dy = (y - texSize/2.0f) / (texSize/2.0f);
            float dist = sqrt(dx*dx + dy*dy);
            
            if (dist <= 1.0f) {
                // 使用三次函数创建软阴影边缘
                float t = dist;
                float alpha = 1.0f - t*t*t; // 中心不透明，边缘透明
                
                // 使阴影中心更暗
                float centerFactor = 1.0f - t * 0.5f;
                alpha *= centerFactor;
                
                // 确保alpha在0-1范围内
                if (alpha < 0.0f) alpha = 0.0f;
                if (alpha > 1.0f) alpha = 1.0f;
                
                pixels[idx] = 0;     // R
                pixels[idx+1] = 0;   // G
                pixels[idx+2] = 0;   // B
                pixels[idx+3] = (unsigned char)(alpha * 255 * shadowIntensity); // A
            } else {
                pixels[idx] = 0;
                pixels[idx+1] = 0;
                pixels[idx+2] = 0;
                pixels[idx+3] = 0; // 完全透明
            }
        }
    }
    
    glGenTextures(1, &shadowTextureID);
    glBindTexture(GL_TEXTURE_2D, shadowTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    delete[] pixels;
}

// ========================
// 绘制函数
// ========================

void generateSphere(float radius, int slices, int stacks, 
                    vector<float>& vertices, vector<float>& normals, 
                    vector<float>& texCoords) {
    const float PI = 3.14159265359f;
    
    for (int i = 0; i <= stacks; ++i) {
        float phi = PI * i / stacks;
        
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * PI * j / slices;
            
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            normals.push_back(x / radius);
            normals.push_back(y / radius);
            normals.push_back(z / radius);
            
            float u = (float)j / slices;
            float v = (float)i / stacks;
            
            texCoords.push_back(u);
            texCoords.push_back(v);
        }
    }
}

void drawCustomSphere(float radius, int slices, int stacks) {
    static vector<float> vertices;
    static vector<float> normals;
    static vector<float> texCoords;
    static bool initialized = false;
    
    if (!initialized) {
        generateSphere(radius, slices, stacks, vertices, normals, texCoords);
        initialized = true;
    }
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    for (int i = 0; i < stacks; ++i) {
        glBegin(GL_TRIANGLE_STRIP);
        
        for (int j = 0; j <= slices; ++j) {
            int idx1 = (i * (slices + 1) + j) * 3;
            int texIdx1 = (i * (slices + 1) + j) * 2;
            
            int idx2 = ((i + 1) * (slices + 1) + j) * 3;
            int texIdx2 = ((i + 1) * (slices + 1) + j) * 2;
            
            glNormal3f(normals[idx1], normals[idx1 + 1], normals[idx1 + 2]);
            glTexCoord2f(texCoords[texIdx1], texCoords[texIdx1 + 1]);
            glVertex3f(vertices[idx1], vertices[idx1 + 1], vertices[idx1 + 2]);
            
            glNormal3f(normals[idx2], normals[idx2 + 1], normals[idx2 + 2]);
            glTexCoord2f(texCoords[texIdx2], texCoords[texIdx2 + 1]);
            glVertex3f(vertices[idx2], vertices[idx2 + 1], vertices[idx2 + 2]);
        }
        
        glEnd();
    }
    
    glDisable(GL_TEXTURE_2D);
}

// ========================
// 绘制地面
// ========================

void drawFloor() {
    glDisable(GL_LIGHTING);
    
    // 首先绘制地面基础颜色
    glColor3f(0.5f, 0.5f, 0.5f); // 中灰色地面
    
    glBegin(GL_QUADS);
    glVertex3f(-floorSize, floorY, -floorSize);
    glVertex3f(floorSize, floorY, -floorSize);
    glVertex3f(floorSize, floorY, floorSize);
    glVertex3f(-floorSize, floorY, floorSize);
    glEnd();
    
    // 绘制网格线
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINES);
    
    // X方向网格线
    for (float x = -floorSize; x <= floorSize; x += 0.5f) {
        glVertex3f(x, floorY + 0.001f, -floorSize);
        glVertex3f(x, floorY + 0.001f, floorSize);
    }
    
    // Z方向网格线
    for (float z = -floorSize; z <= floorSize; z += 0.5f) {
        glVertex3f(-floorSize, floorY + 0.001f, z);
        glVertex3f(floorSize, floorY + 0.001f, z);
    }
    glEnd();
    
    glEnable(GL_LIGHTING);
}

// ========================
// 阴影计算函数
// ========================

// 计算点光源投影
void calculatePointShadow(float lightX, float lightY, float lightZ,
                         float pointX, float pointY, float pointZ,
                         float& shadowX, float& shadowZ, float& shadowSize) {
    // 地面平面：y = floorY
    // 从光源到点的向量
    float dx = pointX - lightX;
    float dy = pointY - lightY;
    float dz = pointZ - lightZ;
    
    if (fabs(dy) < 0.0001f) {
        // 光线与地面平行
        shadowX = lightX;
        shadowZ = lightZ;
        shadowSize = 0.0f;
        return;
    }
    
    // 计算光线与地面的交点
    float t = (floorY - lightY) / dy;
    
    if (t < 0) {
        // 光线从下往上照射
        shadowX = lightX;
        shadowZ = lightZ;
        shadowSize = 0.0f;
        return;
    }
    
    shadowX = lightX + t * dx;
    shadowZ = lightZ + t * dz;
    
    // 计算阴影大小：考虑地球半径和透视效果
    float earthRadius = 1.0f * zoom;
    float distanceToLight = sqrt(dx*dx + dy*dy + dz*dz);
    
    // 简单的阴影大小计算
    shadowSize = earthRadius * (1.0f + t * 0.5f);
    
    // 根据光源高度调整阴影大小
    float lightHeight = fabs(lightY - floorY);
    if (lightHeight > 0) {
        shadowSize *= (distanceToLight / lightHeight);
    }
    
    // 限制最大阴影大小
    if (shadowSize > floorSize * 0.6f) {
        shadowSize = floorSize * 0.6f;
    }
}

// 计算阴影椭圆的拉伸参数
void calculateShadowEllipse(float lightX, float lightY, float lightZ,
                           float& stretchX, float& stretchZ, float& rotation) {
    // 计算光源方向向量在地面上的投影
    float lightLen = sqrt(lightX*lightX + lightY*lightY + lightZ*lightZ);
    
    if (lightLen < 0.0001f) {
        stretchX = 1.0f;
        stretchZ = 1.0f;
        rotation = 0.0f;
        return;
    }
    
    // 归一化光源向量
    float lx = lightX / lightLen;
    float ly = lightY / lightLen;
    float lz = lightZ / lightLen;
    
    // 计算光源在地面上的投影方向
    float groundLX = lx;
    float groundLZ = lz;
    
    // 拉伸参数：在光源方向上拉伸
    float dotX = fabs(groundLX);
    float dotZ = fabs(groundLZ);
    
    // 拉伸程度取决于光源方向
    stretchX = 1.0f + dotX * 0.8f;
    stretchZ = 1.0f + dotZ * 0.8f;
    
    // 旋转角度（弧度）
    rotation = atan2(groundLZ, groundLX);
}

// ========================
// 绘制自然阴影
// ========================

void drawNaturalShadow() {
    if (!shadowEnabled || !lightEnabled) return;
    
    // 获取光源位置
    LightPosition light = lightPositions[currentLightPosition];
    
    // 计算阴影位置和大小
    float shadowX, shadowZ, shadowSize;
    calculatePointShadow(light.x, light.y, light.z, 0, 0, 0, shadowX, shadowZ, shadowSize);
    
    // 计算阴影椭圆参数
    float stretchX, stretchZ, rotation;
    calculateShadowEllipse(light.x, light.y, light.z, stretchX, stretchZ, rotation);
    
    // 如果阴影太小，不绘制
    if (shadowSize < 0.1f) return;
    
    // 保存当前矩阵状态
    glPushMatrix();
    
    // 移动到阴影位置
    glLoadIdentity();
    gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
    
    // 应用阴影的变换
    glTranslatef(shadowX, floorY + 0.002f, shadowZ);
    glRotatef(rotation * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
    glScalef(shadowSize * stretchX, 1.0f, shadowSize * stretchZ);
    
    // 禁用光照，启用混合和阴影纹理
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 使用阴影纹理
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, shadowTextureID);
    
    // 设置纹理环境为调制
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    // 使用黑色半透明颜色
    glColor4f(0.0f, 0.0f, 0.0f, shadowIntensity);
    
    // 禁用深度写入，防止阴影遮挡地面
    glDepthMask(GL_FALSE);
    
    // 绘制阴影四边形（使用纹理）
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 0.0f, 1.0f);
    glEnd();
    
    // 恢复状态
    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    
    glPopMatrix();
}

// ========================
// 绘制环境光遮蔽（AO）效果
// ========================

void drawAmbientOcclusion() {
    if (!shadowEnabled) return;
    
    // 保存当前矩阵状态
    glPushMatrix();
    
    // 应用地球的变换
    glScalef(zoom, zoom, zoom);
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
    
    // 计算地球底部位置
    float earthBottomY = -1.0f * zoom; // 地球半径为1
    
    // 禁用光照，启用混合
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 在地球底部绘制环境光遮蔽（接触阴影）
    int segments = 32;
    float contactRadius = 0.2f * zoom;
    
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f * shadowIntensity);
    glVertex3f(0.0f, earthBottomY - 0.001f, 0.0f);
    
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = contactRadius * cos(angle);
        float z = contactRadius * sin(angle);
        glVertex3f(x, earthBottomY - 0.001f, z);
    }
    glEnd();
    
    // 恢复状态
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    
    glPopMatrix();
}

// ========================
// 光照设置函数
// ========================

void setCurrentLight() {
    if (!lightEnabled) return;
    
    LightPosition light = lightPositions[currentLightPosition];
    
    // 设置光源位置
    GLfloat lightPosition[] = {light.x, light.y, light.z, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    
    // 设置光源颜色
    GLfloat lightAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    GLfloat lightDiffuse[] = {1.0f, 1.0f, 0.9f, 1.0f}; // 稍微偏暖色
    GLfloat lightSpecular[] = {0.8f, 0.8f, 0.8f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
}

void nextLightPosition() {
    currentLightPosition = (currentLightPosition + 1) % lightPositions.size();
    LightPosition light = lightPositions[currentLightPosition];
    cout << "光源位置: " << currentLightPosition << " - " << light.name 
         << " (" << light.x << ", " << light.y << ", " << light.z << ")" << endl;
}

void prevLightPosition() {
    currentLightPosition = (currentLightPosition - 1 + lightPositions.size()) % lightPositions.size();
    LightPosition light = lightPositions[currentLightPosition];
    cout << "光源位置: " << currentLightPosition << " - " << light.name 
         << " (" << light.x << ", " << light.y << ", " << light.z << ")" << endl;
}

void setLightPosition(int index) {
    if (index >= 0 && index < (int)lightPositions.size()) {
        currentLightPosition = index;
        LightPosition light = lightPositions[currentLightPosition];
        cout << "光源位置: " << currentLightPosition << " - " << light.name 
             << " (" << light.x << ", " << light.y << ", " << light.z << ")" << endl;
    } else {
        cout << "无效的光源位置索引: " << index << "，有效范围: 0-" << lightPositions.size()-1 << endl;
    }
}

void toggleLighting() {
    lightEnabled = !lightEnabled;
    if (lightEnabled) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        cout << "光照: 开启" << endl;
    } else {
        glDisable(GL_LIGHTING);
        cout << "光照: 关闭" << endl;
    }
}

void toggleShadow() {
    shadowEnabled = !shadowEnabled;
    cout << "阴影: " << (shadowEnabled ? "开启" : "关闭") << endl;
}

void adjustShadowIntensity(float delta) {
    shadowIntensity += delta;
    if (shadowIntensity < 0.1f) shadowIntensity = 0.1f;
    if (shadowIntensity > 1.0f) shadowIntensity = 1.0f;
    cout << "阴影强度: " << shadowIntensity << endl;
    
    // 更新阴影纹理
    if (shadowTextureID != 0) {
        glDeleteTextures(1, &shadowTextureID);
        shadowTextureID = 0;
    }
    createSoftShadowTexture();
}

// 打印光源信息
void printLightInfo() {
    cout << endl;
    cout << "========================================" << endl;
    cout << "当前光源位置: " << currentLightPosition << endl;
    LightPosition light = lightPositions[currentLightPosition];
    cout << "位置描述: " << light.name << endl;
    cout << "坐标: (" << light.x << ", " << light.y << ", " << light.z << ")" << endl;
    cout << "光照状态: " << (lightEnabled ? "开启" : "关闭") << endl;
    cout << "阴影状态: " << (shadowEnabled ? "开启" : "关闭") << endl;
    cout << "阴影强度: " << shadowIntensity << endl;
    cout << "========================================" << endl;
}

// ========================
// 显示回调函数
// ========================

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 设置投影矩阵
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)WIDTH/HEIGHT, 0.1, 100.0);
    
    // 设置模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // 设置相机位置
    gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
    
    // 设置光源
    setCurrentLight();
    
    // 绘制地面
    drawFloor();
    
    // 绘制阴影（在地面之上）
    if (shadowEnabled && lightEnabled) {
        drawNaturalShadow();
    }
    
    // 应用地球变换
    glScalef(zoom, zoom, zoom);
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
    
    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    
    // 设置地球材质属性
    if (lightEnabled) {
        GLfloat matAmbient[] = {0.7f, 0.7f, 0.7f, 1.0f};
        GLfloat matDiffuse[] = {0.9f, 0.9f, 0.9f, 1.0f};
        GLfloat matSpecular[] = {0.3f, 0.3f, 0.3f, 1.0f};
        GLfloat matShininess = 30.0f;
        
        glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialf(GL_FRONT, GL_SHININESS, matShininess);
    } else {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    
    // 绘制地球
    drawCustomSphere(1.0f, 36, 18);
    
    // 绘制环境光遮蔽（接触阴影）
    if (shadowEnabled) {
        drawAmbientOcclusion();
    }
    
    // 如果禁用了光照，重新启用它
    if (lightEnabled) {
        glEnable(GL_LIGHTING);
    }
    
    glutSwapBuffers();
}

// ========================
// 交互函数
// ========================

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        mouseLeftDown = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
    } else if (button == 3) { // 滚轮向上
        zoom *= 1.1f;
        glutPostRedisplay();
    } else if (button == 4) { // 滚轮向下
        zoom /= 1.1f;
        glutPostRedisplay();
    }
}

void motion(int x, int y) {
    if (mouseLeftDown) {
        rotationY += (x - lastMouseX) * 0.5f;
        rotationX += (y - lastMouseY) * 0.5f;
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC键退出
            exit(0);
            break;
            
        case '+': // 放大
        case '=':
            zoom *= 1.1f;
            glutPostRedisplay();
            break;
            
        case '-': // 缩小
        case '_':
            zoom /= 1.1f;
            glutPostRedisplay();
            break;
            
        case 'r': // 重置
        case 'R':
            rotationX = rotationY = 0.0f;
            zoom = 1.0f;
            glutPostRedisplay();
            break;
            
        case 't': // 重新加载纹理
        case 'T':
            if (textureID != 0) {
                glDeleteTextures(1, &textureID);
                textureID = 0;
            }
            initTexture();
            glutPostRedisplay();
            break;
            
        case 'l': // 切换光照开关
        case 'L':
            toggleLighting();
            glutPostRedisplay();
            break;
            
        case 's': // 切换阴影开关
        case 'S':
            toggleShadow();
            glutPostRedisplay();
            break;
            
        case '[': // 减少阴影强度
            adjustShadowIntensity(-0.1f);
            glutPostRedisplay();
            break;
            
        case ']': // 增加阴影强度
            adjustShadowIntensity(0.1f);
            glutPostRedisplay();
            break;
            
        case 'n': // 下一个光源位置
        case 'N':
            nextLightPosition();
            printLightInfo();
            glutPostRedisplay();
            break;
            
        case 'p': // 上一个光源位置
        case 'P':
            prevLightPosition();
            printLightInfo();
            glutPostRedisplay();
            break;
            
        case 'i': // 显示当前光源信息
        case 'I':
            printLightInfo();
            break;
            
        // 数字键选择光源位置（0-7对应8个光源位置）
        case '0': setLightPosition(0); glutPostRedisplay(); break;
        case '1': setLightPosition(1); glutPostRedisplay(); break;
        case '2': setLightPosition(2); glutPostRedisplay(); break;
        case '3': setLightPosition(3); glutPostRedisplay(); break;
        case '4': setLightPosition(4); glutPostRedisplay(); break;
        case '5': setLightPosition(5); glutPostRedisplay(); break;
        case '6': setLightPosition(6); glutPostRedisplay(); break;
        case '7': setLightPosition(7); glutPostRedisplay(); break;
        
        // 其他数字键提示
        case '8':
        case '9':
            cout << "提示：光源位置只有0-7，共8个位置" << endl;
            break;
    }
}

// ========================
// 初始化函数
// ========================

void init() {
    glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    // 启用颜色混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    initTexture();
    createSoftShadowTexture();
}

// ========================
// 主函数
// ========================

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("真实地球仪 - 自然阴影效果");
    
    // 启用多重采样抗锯齿
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    
    init();
    
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    
    // 打印操作说明
    cout << endl;
    cout << "========================================" << endl;
    cout << "真实地球仪 - 自然阴影效果" << endl;
    cout << "========================================" << endl;
    cout << "操作说明:" << endl;
    cout << "  鼠标左键拖拽 - 旋转地球" << endl;
    cout << "  鼠标滚轮 - 缩放地球" << endl;
    cout << "  +/- 键 - 缩放地球" << endl;
    cout << "  R 键 - 重置视图" << endl;
    cout << "  T 键 - 重新加载纹理" << endl;
    cout << "  L 键 - 切换光照开关" << endl;
    cout << "  S 键 - 切换阴影开关" << endl;
    cout << "  [ 键 - 减少阴影强度" << endl;
    cout << "  ] 键 - 增加阴影强度" << endl;
    cout << "  N 键 - 下一个光源位置" << endl;
    cout << "  P 键 - 上一个光源位置" << endl;
    cout << "  I 键 - 显示当前光源信息" << endl;
    cout << "  0-7 键 - 选择特定光源位置" << endl;
    cout << "  ESC 键 - 退出程序" << endl;
    cout << endl;
    cout << "阴影特性:" << endl;
    cout << "  • 软阴影边缘（中心暗，边缘渐变）" << endl;
    cout << "  • 根据光源方向拉伸阴影形状" << endl;
    cout << "  • 接触阴影（地球与地面接触处更暗）" << endl;
    cout << "  • 阴影位置随地球旋转而变化" << endl;
    cout << "========================================" << endl;
    
    glutMainLoop();
    
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
    if (shadowTextureID != 0) {
        glDeleteTextures(1, &shadowTextureID);
    }
    
    return 0;
}