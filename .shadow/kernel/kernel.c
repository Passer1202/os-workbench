#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>
#include "mypic.h"

#define SIDE 1


static int w, h;  // Screen size
#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names[] = { AM_KEYS(KEYNAME) };

static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

void print_key() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
    puts("Key pressed: ");
    puts(key_names[event.keycode]);
    puts("\n");
    if(strcmp(key_names[event.keycode],"ESCAPE")==0)halt(0);
  }
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}

void splash() {
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;
  
  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
      if(w<=800&&h<=600){
        //int cx=800/w;
        //int cy=600/h;
        int index=(x*SIDE/w)*800+((y*SIDE)/h)*600*800;
        uint32_t color=__1111_rgb[index*3+0]<<16|__1111_rgb[index*3+1]<<8|__1111_rgb[index*3+2];
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE,color); 
      }
      assert(w<=800&&h<=600);
        
 
      /*
      int xcolor=x%10;
      switch(xcolor){
      	case 0:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff);break;
      	case 1:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xfffacd);break;
      	case 2:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xfff5ee);break;
      	case 3:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x87ceeb);break;
      	case 4:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffc0cb);break;
      	case 5:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xa020f0);break;
      	case 6:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x00ff00);break;
      	case 7:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xff6a6a);break;
      	case 8:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x90ee90);break;
      	case 9:draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xfdf5e6);break;
      }
      */
    }
  }
}

// Operating system is a C program!
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args);  // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
  }
  return 0;
}
