#ifndef __PICOPLAYER_H__
#define __PICOPLAYER_H__


#include <stdlib.h>
#include <stdint.h>


#define  _PICOPLAYER_DEBUG_
#undef   _OUTPUT_DEBUG_
#define  _MAIN_DEBUG_

#define PIXEL_PORTS 8

typedef struct __string_ctrl
{
   uint32_t channel_count[8];
   uint32_t start_channel[8];
   int brightness[8];
   int gamma[8];
   int direction[8];
   int color_order[8];
   int null_pixels[8];
   int test_mode[8];
   uint32_t test_color[8]; //RGBW
   int * spi_dev;
   int active[8];
//   spi_device * spi_dev;
} string_ctrl;

typedef struct __thread_ctrl
{
   int pixel_ports;
   char *buffer;
   char *bufferb;
   uint8_t activebuffer;
   uint8_t run_mode;
   uint8_t tick;  // Set when timer expired
   string_ctrl led_string[PIXEL_PORTS];
} thread_ctrl;

enum{
   MODE_RUN = 0,
   MODE_TEST,
   MODE_STOPPED
};


//
// get a pointer to the running config
//
thread_ctrl *get_running_config();

#endif