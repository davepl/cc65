// --------------------------------------------------------------------------
// Simple Graphics Test for KIM-1 with MTU Visible Memory Board
//
// Assumes the MTU Visible Memory Board mapped at 0xA000 for 8K of video RAM
//
// davepl@davepl.com
// --------------------------------------------------------------------------

#include <stdio.h>                  // For printf
#include <stdlib.h>                 // For rand, srand
#include <string.h>                 // For memcpy
#include <ctype.h>

typedef unsigned char byte;
typedef          char sbyte;

extern void ClearScreen(void);      // In subs.asm
extern void ScrollScreen(void);
extern void DrawCircle(void);
extern void SetPixel(void);
extern void ClearPixel(void);
extern void DrawChar(void);
extern void Demo(void);
extern void __fastcall__ Delay(byte loops);
extern void __fastcall__ DrawLine(byte bSet);
extern byte __fastcall__ AscToPet(byte in);
extern byte __fastcall__ PetToAsc(byte in);
extern byte __fastcall__ ReverseBits(byte in);
extern void __fastcall__ CharOut(byte asci_char);
extern byte __fastcall__ getch();
extern unsigned char font8x8_basic[256][8];

extern int  x1cord;
extern int  y1cord;
extern int  x2cord;
extern int  y2cord;
extern int  cursorX;
extern int  cursorY;

// If in zeropage:
//
// #pragma zpsym("x1cord")
// #pragma zpsym("x2cord")
// #pragma zpsym("y1cord")
// #pragma zpsym("y2cord")

// Screen memory is placed at A000-BFFF, 320x200 pixels, mapped right to left within each horizontal byte

byte * screen    = (byte *) 0xA000;

// Cursor position

#define SCREEN_WIDTH      320
#define SCREEN_HEIGHT     200
#define SCREEN_CENTER_X   (SCREEN_WIDTH/2)
#define SCREEN_CENTER_Y   (SCREEN_HEIGHT/2)
#define CHARWIDTH         8
#define CHARHEIGHT        8
#define BYTESPERROW       (SCREEN_WIDTH / 8)
#define BYTESPERCHARROW   (BYTESPERROW * 8)
#define CHARSPERROW       (SCREEN_WIDTH / CHARWIDTH)
#define ROWSPERCOLUMN     (SCREEN_HEIGHT / CHARHEIGHT)

// SETPIXEL
//
// 0 <= x < 320
// 0 <= y < 200
//
// Draws a pixel on the screen in white or black at pixel pos x, y

void SETPIXEL(int x, int y, byte b)
{
   x1cord = x;
   y1cord = y;

   if (b)
      SetPixel();
   else
      ClearPixel();
}

// DRAWPIXEL
//
// 0 <= x < 320
// 0 <= y < 200
//
// Turns on a screen pixel at pixel pos x,y

void DRAWPIXEL(int x, int y)
{
   x1cord = x;
   y1cord = y;
   SetPixel();
}

int c;

void DrawText(char * psz)
{
   while (*psz)
   {
      while (cursorX >= CHARSPERROW)
      {
         cursorX -= CHARSPERROW;
         cursorY += 1;
      }

      // If we've gone off the bottom of the screen, we scroll the screen and back up to the last line again

      if (cursorY >= ROWSPERCOLUMN)
      {
         cursorY = ROWSPERCOLUMN - 1;
         ScrollScreen();
      }

      // If we output a newline we advanced the cursor down one line and reset it to the left

      if (*psz == 0x0A)
      {
         cursorX = 0;
         cursorY++;
         psz++;
      }
      else
      {
         c = *psz;

         __asm__ ("ldx %v", cursorX);
         __asm__ ("ldy %v", cursorY);
         __asm__ ("lda %v", c);
         DrawChar();
         cursorX++;
         psz++;
      }
   }
}

void DrawTextAt(int x, int y, char * psz)
{
   cursorX = x;
   cursorY = y;
   DrawText(psz);
}

// Something like Bresenham's algorithm for drawing a line
/*
void DrawLine(int x0, int y0, int x1, int y1, byte val)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    while (1)
    {
        SETPIXEL(x0, y0, val);

        if (x0 == x1 && y0 == y1)
            break;

        e2 = err;

        if (e2 > -dx)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy)
        {
            err += dx;
            y0 += sy;
        }
    }
}
*/

// DrawCircle
//
// Draw a circle without sin, cos, or floating point!

void DrawCircleC(int x0, int y0, int radius, byte)
{
   x1cord = x0;
   y1cord = y0;
   y2cord = radius;
   DrawCircle();
}

void DrawLineC(int x1, int y1, int x2, int y2, byte bSet)
{
   x1cord = x1;
   y1cord = y1;
   x2cord = x2;
   y2cord = y2;
   DrawLine(bSet);
}

// MirrorFont
//
// RAM font is backwards left-right relative to the way memory is laid out on the KIM-1, so we swap all the
// bytes in place by reversing the order of the bits in every byte

void MirrorFont()
{
   int c;
   byte * pb = (byte *) font8x8_basic;

   for (c = 0; c < 128 * 8; c++)
      pb[c] = ReverseBits(pb[c]);
}

// DrawScreenMoire
//
// Draws a moire pattern on the screen without clearing it first

void DrawMoire(int left, int top, int right, int bottom, byte pixel)
{
   int x, y;

   for (x = left; x < right; x += 6)
      DrawLineC(x, top, right - x + left, bottom, pixel);

   for (y = top; y < bottom; y += 6)
      DrawLineC(left, y, right, bottom - y + top, pixel);
}

void DrawScreenMoire(int left, int top, int right, int bottom)
{
   int x, y;

   DrawLineC(left, top, right, top, 1);
   DrawLineC(left, bottom, right, bottom, 1);
   DrawLineC(left, top, left, bottom, 1);
   DrawLineC(right, top, right, bottom, 1);

   left++; top++; right--; bottom--;

   for (x = left; x < right; x += 6)
      DrawLineC(x, top, right - x + left, bottom, 1);
   for (y = top; y < bottom; y += 6)
      DrawLineC(left, y, right, bottom - y + top, 1);
   for (x = left; x < right; x += 6)
      DrawLineC(x, top, right - x + left, bottom, 0);
   for (y = top; y < bottom; y += 6)
      DrawLineC(left, y, right, bottom - y + top, 0);

}


// sin8_C and cos8_C
//
// An approximation of sin8 and cos8 that uses a 16-byte lookup table and linear interpolation
// to provide a fast and close approximation of sin and cos for angles in the 0-255 range
// The table is a 16-byte table that provides the slope and y-intercept for 8 sections of the
// 0-255 range, and the code uses the high 4 bits of the angle to select the section and the
// low 4 bits to interpolate between the two points in that section.
//
// Inspired by the lib8tion library in FastLED, but rewritten in cc65 C for the KIM-1

static const byte b_m16_interleave[] = { 0, 49, 49, 41, 90, 27, 117, 10 };

byte sin8_C(byte theta)
{
    sbyte y;
    byte offset, secoffset, section, s2, b, m16, mx;
    const byte* p;

    offset = theta;
    if( theta & 0x40 ) {
        offset = (byte)255 - offset;
    }
    offset &= 0x3F; // 0..63

    secoffset  = offset & 0x0F; // 0..15
    if( theta & 0x40) ++secoffset;

    section = offset >> 4; // 0..3
    s2 = section * 2;
    p = b_m16_interleave;
    p += s2;
    b =*p;
    ++p;
    m16 =  *p;

    mx = (m16 * secoffset) >> 4;

    y = mx + b;
    if( theta & 0x80 ) y = -y;

    y += 128;

    return y;
}

byte cos8_C(byte theta)
{
    return sin8_C(theta + 64);  
}


void DrawClockOuter()
{
    byte z;
    const int radius = SCREEN_HEIGHT / 2 - 5;
    
    ClearScreen();
    DrawCircleC(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, radius, 1);
    DrawCircleC(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, radius - 2, 1);

    for (z = 0; z < 12*21; z += 21) 
    {   // 21 approximates 30 degrees in 0-255 system
        // Convert the angle from 0-255 range to 0-360 degrees approximation
        byte angle = z;

        // Calculate the start and end points of the tick marks
        int x2 = (SCREEN_CENTER_X + ((radius - 5) * (sin8_C(angle) - 128) / 128));
        int y2 = (SCREEN_CENTER_Y - ((radius - 5) * (cos8_C(angle) - 128) / 128));
        int x3 = (SCREEN_CENTER_X + ((radius - 2) * (sin8_C(angle) - 128) / 128));
        int y3 = (SCREEN_CENTER_Y - ((radius - 2) * (cos8_C(angle) - 128) / 128));

        // Draw the tick mark
        DrawLineC(x2, y2, x3, y3, 1);
    }
}

void DrawClockHands(byte hours, byte minutes, byte seconds, byte bDraw)
{
    // minutesPerArc = 255 / 60 = 4.25
    // hoursPerArc = 255 / 12 = 21.25

    // Calculate the angles for the hour, minute, and second hands centered around 128
    byte hourAngle = ((hours % 12) * 21) + (minutes / 2); // Hour hand angle
    byte minuteAngle = (minutes * 17) / 4; // 17/4 == 4.25
    byte secondAngle = (seconds * 17) / 4; // Second hand angle

    int hourX, hourY, minuteX, minuteY, secondX, secondY;
    const int radius = SCREEN_HEIGHT / 2 - 20;

    // Calculate the position for the hour hand
    hourX = SCREEN_CENTER_X + ((radius - 40) * (sin8_C(hourAngle) - 128) / 128);
    hourY = SCREEN_CENTER_Y - ((radius - 40) * (cos8_C(hourAngle) - 128) / 128);

    // Calculate the position for the minute hand
    minuteX = SCREEN_CENTER_X + ((radius - 10) * (sin8_C(minuteAngle) - 128) / 128);
    minuteY = SCREEN_CENTER_Y - ((radius - 10) * (cos8_C(minuteAngle) - 128) / 128);

    // Calculate the position for the second hand
    secondX = SCREEN_CENTER_X + (radius * (sin8_C(secondAngle) - 128) / 128);
    secondY = SCREEN_CENTER_Y - (radius * (cos8_C(secondAngle) - 128) / 128);

    // Draw the hands
    DrawLineC(SCREEN_CENTER_X, SCREEN_CENTER_Y, hourX, hourY, bDraw);   // Hour hand
    DrawLineC(SCREEN_CENTER_X, SCREEN_CENTER_Y, minuteX, minuteY, bDraw); // Minute hand
    DrawLineC(SCREEN_CENTER_X, SCREEN_CENTER_Y, secondX, secondY, bDraw); // Second hand

    DrawCircleC(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 3, 1); // Little dot in the middle
}

void DrawClock(byte hours, byte minutes, byte seconds)
{
    static byte lastHour = 0;
    static byte lastMinute = 0;
    static byte lastSecond = 0;

    DrawClockHands(lastHour, lastMinute, lastSecond, 0);
    DrawClockHands(hours, minutes, seconds, 1);
    
    lastHour = hours;
    lastMinute = minutes;
    lastSecond = seconds;

}

int main (void)
{
    byte hour = 5, minute = 6, second = 0;

    DrawClockOuter();

    while(1)
    {
        second++;
        if (second == 60)
        {
            second = 0;
            minute++;
            if (minute == 60)
            {
                minute = 0;
                hour++;
                if (hour == 24)
                    hour = 0;
            }
        }
        DrawClock(hour, minute, second);
        Delay(60);
    }

    return 0;
}
