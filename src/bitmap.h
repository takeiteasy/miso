//
//  bitmap.h
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#ifndef bitmap_h
#define bitmap_h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>

typedef struct {
    int *buf;
    unsigned int w, h;
} Bitmap;

int RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
int RGB(unsigned char r, unsigned char g, unsigned char b);
int RGBA1(unsigned char c, unsigned char a);
int RGB1(unsigned char c);
unsigned char Rgba(int c);
unsigned char rGba(int c);
unsigned char rgBa(int c);
unsigned char rgbA(int c);
int rGBA(int c, unsigned char r);
int RgBA(int c, unsigned char g);
int RGbA(int c, unsigned char b);
int RGBa(int c, unsigned char a);

Bitmap NewBitmap(unsigned int w, unsigned int h);
void DestroyBitmap(Bitmap *bitmap);
void PSet(Bitmap *b, int x, int y, int col);
int PGet(Bitmap *b, int x, int y);
Bitmap LoadBitmap(const char *path);

#endif /* bitmap_h */
