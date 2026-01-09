#!/bin/bash

echo "========================================"
echo "真实地球仪"
echo "========================================"

# 检查 Xcode Command Line Tools
if ! xcode-select -p &>/dev/null; then
    echo "未检测到 Xcode Command Line Tools，正在安装..."
    xcode-select --install
    echo "请安装完成后重新运行本脚本"
    exit 1
fi

# 简要提示
echo "准备纹理并编译程序..."

# 检查 Python（仅用于图片处理，不输出详细信息）
if command -v python3 &>/dev/null; then
    if ! python3 -c "from PIL import Image" &>/dev/null; then
        pip3 install Pillow >/dev/null 2>&1
    fi
fi

# 处理图片（静默执行）
if [ -f "earth.jpg" ]; then
    python3 create_world_bmp.py "earth.jpg" >/dev/null 2>&1
elif [ -f "world.jpg" ]; then
    python3 create_world_bmp.py "world.jpg" >/dev/null 2>&1
else
    python3 create_world_bmp.py >/dev/null 2>&1
fi

# 编译程序
g++ -std=c++11 main.cpp -o earth -framework GLUT -framework OpenGL

if [ $? -ne 0 ]; then
    echo "✗ 编译失败"
    exit 1
fi

echo "✓ 编译成功！"
echo
echo "运行地球仪..."


# 运行程序
./earth
