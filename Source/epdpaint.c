//#include <avr/pgmspace.h>
#include "epdpaint.h"

// Предвычисленная таблица масок
static const unsigned char bit_masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

unsigned char image[768];
unsigned char* pimage;
int pwidth;
int pheight;
int protate;
int pinvert;
int pwidth_bytes;  // Для оптимизации: байт на строку

void PaintPaint(unsigned char* image, int width, int height) {
    pinvert = IF_INVERT_COLOR;
    protate = ROTATE_0;
    pimage = image;
    /* 1 byte = 8 pixels, so the width should be the multiple of 8 */
    pwidth = width % 8 ? width + 8 - (width % 8) : width;
    pheight = height;
    pwidth_bytes = (pwidth + 7) / 8;  // Предвычисляем байты на строку
}

void PaintSetWidth(int width) {
    pwidth = width % 8 ? width + 8 - (width % 8) : width;
    pwidth_bytes = (pwidth + 7) / 8;  // Обновляем байты на строку
}

void PaintSetHeight(int height) {
    pheight = height;
}

void PaintSetRotate(int rotate){
    protate = rotate;
}

// Быстрая очистка буфера
void PaintClear(int colored) {
    unsigned char fill_byte;
    
    if (pinvert) {
        fill_byte = colored ? 0xFF : 0x00;
    } else {
        fill_byte = colored ? 0x00 : 0xFF;
    }
    
    // Заполняем побайтово вместо посепиксельно
    int total_bytes = pwidth_bytes * pheight;
    for (int i = 0; i < total_bytes; i++) {
        pimage[i] = fill_byte;
    }
}

// Быстрая отрисовка пикселя (инлайн для производительности)
static inline void PaintDrawAbsolutePixelFast(int x, int y, int colored) {
    if (x < 0 || x >= pwidth || y < 0 || y >= pheight) {
        return;
    }
    
    int byte_index = (x >> 3) + y * pwidth_bytes;  // Оптимизация: >>3 вместо /8
    unsigned char bit_mask = bit_masks[x & 7];      // Таблица вместо 0x80 >> (x % 8)
    
    if (pinvert) {
        if (colored) {
            pimage[byte_index] |= bit_mask;
        } else {
            pimage[byte_index] &= ~bit_mask;
        }
    } else {
        if (colored) {
            pimage[byte_index] &= ~bit_mask;
        } else {
            pimage[byte_index] |= bit_mask;
        }
    }
}

// Старая версия для совместимости
void PaintDrawAbsolutePixel(int x, int y, int colored) {
    if (x < 0 || x >= pwidth || y < 0 || y >= pheight) {
        return;
    }
    // Используем таблицу масок вместо вычисления
    if (pinvert) {
        if (colored) {
            pimage[(x + y * pwidth) / 8] |= bit_masks[x % 8];  // Используем таблицу
        } else {
            pimage[(x + y * pwidth) / 8] &= ~bit_masks[x % 8]; // Используем таблицу
        }
    } else {
        if (colored) {
            pimage[(x + y * pwidth) / 8] &= ~bit_masks[x % 8]; // Используем таблицу
        } else {
            pimage[(x + y * pwidth) / 8] |= bit_masks[x % 8];  // Используем таблицу
        }
    }
}

void PaintDrawPixel(int x, int y, int colored) {
    int point_temp;
    if (protate == ROTATE_0) {
        if(x < 0 || x >= pwidth || y < 0 || y >= pheight) {
            return;
        }
        PaintDrawAbsolutePixelFast(x, y, colored);  // Используем быструю версию
    } else if (protate == ROTATE_90) {
        if(x < 0 || x >= pheight || y < 0 || y >= pwidth) {
          return;
        }
        point_temp = x;
        x = pwidth - y;
        y = point_temp;
        PaintDrawAbsolutePixelFast(x-1, y, colored);  // Используем быструю версию
    } else if (protate == ROTATE_180) {
        if(x < 0 || x >= pwidth || y < 0 || y >= pheight) {
          return;
        }
        x = pwidth - x;
        y = pheight - y;
        PaintDrawAbsolutePixelFast(x-1, y-1, colored);  // Используем быструю версию
    } else if (protate == ROTATE_270) {
        if(x < 0 || x >= pheight || y < 0 || y >= pwidth) {
          return;
        }
        point_temp = x;
        x = y;
        y = pheight - point_temp;
        PaintDrawAbsolutePixelFast(x, y-1, colored);  // Используем быструю версию
    }
}

void PaintSetInvert(int invert) {
  pinvert = invert;
}

// Оптимизированная версия отрисовки изображения
void PaintDrawImage(const unsigned char* imgData, int x, int y, int Width, int Height, int colored) {
  int i, j;
  const unsigned char* prt = imgData;
  int byteWidth = (Width + 7) / 8;  // Предвычисляем байты на строку
  
    for (j = 0; j < Height; j++) {
        for (i = 0; i < Width; i++) {
          // Используем таблицу масок и быстрый доступ
          if (prt[i >> 3] & bit_masks[i & 7]) {
            PaintDrawPixel(x + i, y + j, colored);            
          }
        }
        prt += byteWidth;  // Переходим к следующей строке
    }
}
        
void PaintDrawCharAt(int x, int y, char ascii_char, sFONT* font, int colored) {
    int i, j;
    unsigned int char_offset = (ascii_char - ' ') * font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
    const unsigned char* ptr = &font->table[char_offset];
    int byteWidth = (font->Width + 7) / 8;  // Предвычисляем байты на строку

    for (j = 0; j < font->Height; j++) {
        for (i = 0; i < font->Width; i++) {
          // Используем таблицу масок и быстрый доступ
          if (ptr[i >> 3] & bit_masks[i & 7]) {     
                PaintDrawPixel(x + i, y + j, colored);
            }
        }
        ptr += byteWidth;  // Переходим к следующей строке
    }
}

void PaintDrawStringAt(int x, int y, const char* text, sFONT* font, int colored) {
    const char* p_text = text;
    unsigned int counter = 0;
    int refcolumn = x;
    
    /* Send the string character by character on EPD */
    while (*p_text != 0) {
        /* Display one character on EPD */
        PaintDrawCharAt(refcolumn, y, *p_text, font, colored);
        /* Decrement the column position by 16 */
        refcolumn += font->Width;
        /* Point on the next character */
        p_text++;
        counter++;
    }
}

unsigned char* PaintGetImage(void) {
    return pimage;
}

int PaintGetWidth(void) {
    return pwidth;
}

int PaintGetHeight(void) {
    return pheight;
}

void PaintDrawRectangle(int x0, int y0, int x1, int y1, int colored) {
    int min_x, min_y, max_x, max_y;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y1 : y0;

    if (protate == ROTATE_0) {
      PaintDrawHorizontalLine(min_x, min_y, max_x - min_x + 1, colored);
    } else if (protate == ROTATE_90) {      
      PaintDrawHorizontalLine(min_x, min_y+1, max_x - min_x + 1, colored);
    }
    PaintDrawHorizontalLine(min_x, max_y, max_x - min_x + 1, colored);
    PaintDrawVerticalLine(min_x, min_y, max_y - min_y + 1, colored);
    PaintDrawVerticalLine(max_x, min_y, max_y - min_y + 1, colored);
}

void PaintDrawHorizontalLine(int x, int y, int line_width, int colored) {
    int i;
    for (i = x; i < x + line_width; i++) {
        PaintDrawPixel(i, y, colored);
    }
}

void PaintDrawVerticalLine(int x, int y, int line_height, int colored) {
    int i;
    for (i = y; i < y + line_height; i++) {
        PaintDrawPixel(x, i, colored);
    }
}

void PaintDrawLine(int x0, int y0, int x1, int y1, int colored) {
    /* Bresenham algorithm */
    int dx = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 - y0 <= 0 ? y1 - y0 : y0 - y1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while((x0 != x1) && (y0 != y1)) {
        PaintDrawPixel(x0, y0 , colored);
        if (2 * err >= dy) {     
            err += dy;
            x0 += sx;
        }
        if (2 * err <= dx) {
            err += dx; 
            y0 += sy;
        }
    }
}

void PaintDrawFilledRectangle(int x0, int y0, int x1, int y1, int colored) {
    int min_x, min_y, max_x, max_y;
    int i;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y1 : y0;
    
    for (i = min_x; i <= max_x; i++) {
      PaintDrawVerticalLine(i, min_y, max_y - min_y + 1, colored);
    }
}
      
void PaintDrawCircle(int x, int y, int radius, int colored) {
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        PaintDrawPixel(x - x_pos, y + y_pos, colored);
        PaintDrawPixel(x + x_pos, y + y_pos, colored);
        PaintDrawPixel(x + x_pos, y - y_pos, colored);
        PaintDrawPixel(x - x_pos, y - y_pos, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
              e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}
              
void PaintDrawFilledCircle(int x, int y, int radius, int colored) {
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        PaintDrawPixel(x - x_pos, y + y_pos, colored);
        PaintDrawPixel(x + x_pos, y + y_pos, colored);
        PaintDrawPixel(x + x_pos, y - y_pos, colored);
        PaintDrawPixel(x - x_pos, y - y_pos, colored);
        PaintDrawHorizontalLine(x + x_pos, y + y_pos, 2 * (-x_pos) + 1, colored);
        PaintDrawHorizontalLine(x + x_pos, y - y_pos, 2 * (-x_pos) + 1, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if(e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while(x_pos <= 0);
}
