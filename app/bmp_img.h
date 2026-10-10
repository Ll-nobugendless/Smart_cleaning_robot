#ifndef __BMP_IMG_H
#define __BMP_IMG_H

//OLED方向箭头图片数组（实体定义唯一存在于 bmp_img.c，此处仅 extern 声明，
//避免多个 .c 同时包含 OLED/bmp.h 造成 L6200E 重复定义）
extern unsigned char BMP1[];
extern unsigned char BMP2[];
extern unsigned char BMP3[];
extern unsigned char BMP4[];

#endif
