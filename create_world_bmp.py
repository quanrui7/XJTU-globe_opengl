#!/usr/bin/env python3
# create_world_bmp.py - 修复纹理方向

import sys
import os
from PIL import Image, ImageDraw

def create_simple_world_map():
    """创建简单的世界地图BMP图片（方向正确）"""
    width = 512
    height = 256
    
    # 创建新图片，蓝色背景（海洋）
    img = Image.new('RGB', (width, height), (30, 60, 150))
    draw = ImageDraw.Draw(img)
    
    # 注意：在图片中，北极应该在顶部，南极在底部
    # 所以我们在图片顶部画北极，底部画南极
    
    # 各大洲的位置（注意Y坐标：0是顶部，height是底部）
    continents = [
        ('Africa', (250, 100, 350, 180), (160, 120, 80)),      # 土黄色
        ('Asia', (300, 50, 450, 150), (60, 160, 60)),         # 绿色
        ('Europe', (270, 50, 320, 100), (100, 180, 100)),     # 浅绿色
        ('North America', (50, 50, 200, 120), (180, 80, 80)),  # 红色
        ('South America', (150, 130, 250, 200), (220, 120, 60)), # 橙色
        ('Australia', (400, 170, 450, 210), (180, 80, 180)),   # 紫色
        ('Antarctica', (0, 220, width, height), (220, 220, 220)) # 白色（南极在底部）
    ]
    
    # 绘制各大洲
    for name, bbox, color in continents:
        x1, y1, x2, y2 = bbox
        r, g, b = color
        draw.rectangle([x1, y1, x2, y2], fill=(r, g, b), outline=(255, 255, 255))
    
    # 绘制经纬线
    for i in range(0, width, width//12):
        draw.line([(i, 0), (i, height)], fill=(255, 255, 255), width=1)
    
    for i in range(0, height, height//6):
        draw.line([(0, i), (width, i)], fill=(255, 255, 255), width=1)
    
    # 标记北极和南极
    draw.text((width//2, 10), "NORTH", fill=(255, 255, 255))
    draw.text((width//2, height-20), "SOUTH", fill=(255, 255, 255))
    
    # 保存为BMP
    img.save('world.bmp')
    print(f"创建了世界地图图片: world.bmp ({width}x{height})")

    return True

def convert_jpg_to_bmp(jpg_file):
    """将JPG图片转换为BMP格式（保持正确的方向）"""
    try:
        # 打开JPG文件
        img = Image.open(jpg_file)
        
        # 转换为RGB模式
        if img.mode != 'RGB':
            img = img.convert('RGB')
        
        # 检查图片方向
        print(f"原始图片尺寸: {img.width}x{img.height}")
        
        # 如果图片有EXIF方向信息，进行旋转
        try:
            exif = img._getexif()
            if exif:
                orientation = exif.get(0x0112)
                if orientation:
                    
                    
                    # 根据EXIF方向进行旋转
                    if orientation == 3:
                        img = img.rotate(180, expand=True)
                        print("旋转180度")
                    elif orientation == 6:
                        img = img.rotate(270, expand=True)
                        print("旋转270度")
                    elif orientation == 8:
                        img = img.rotate(90, expand=True)
                        print("旋转90度")
        except:
            pass  # 没有EXIF信息或获取失败
        
        # 保存为BMP
        bmp_file = 'world.bmp'
        img.save(bmp_file)
        
        print(f"转换成功: {jpg_file} -> {bmp_file}")
        print(f"最终尺寸: {img.width}x{img.height}")
        

        
        return True
    except Exception as e:
        print(f"转换失败: {e}")
        return False

def main():
    """主函数"""
    print("")
    print("=" * 40)
    print("世界地图图片处理工具")
    print("=" * 40)

    
    # 检查命令行参数
    if len(sys.argv) > 1:
        jpg_file = sys.argv[1]
        if os.path.exists(jpg_file):
            print(f"处理文件: {jpg_file}")
            if convert_jpg_to_bmp(jpg_file):
                print("✓ 转换完成")
            else:
                print("✗ 转换失败，创建默认世界地图")
                create_simple_world_map()
        else:
            print(f"错误: 文件 {jpg_file} 不存在")
            print("创建默认世界地图...")
            create_simple_world_map()
    else:
        # 没有参数，检查是否有 earth.jpg
        if os.path.exists('earth.jpg'):
            print("找到 earth.jpg，正在转换...")
            if convert_jpg_to_bmp('earth.jpg'):
                print("✓ 转换完成")
            else:
                print("✗ 转换失败，创建默认世界地图")
                create_simple_world_map()
        elif os.path.exists('world.jpg'):
            print("找到 world.jpg，正在转换...")
            if convert_jpg_to_bmp('world.jpg'):
                print("✓ 转换完成")
            else:
                print("✗ 转换失败，创建默认世界地图")
                create_simple_world_map()
        else:
            print("未找到JPG图片，创建默认世界地图...")
            create_simple_world_map()
    


if __name__ == "__main__":
    main()