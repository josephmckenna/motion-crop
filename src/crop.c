/*
 *    crop.c
 *
 *    Module for handling image rotation.
 *
 *    Copyright 2019, Joseph McKenna (per@pjd.nu)
 *
 *    This software is distributed under the GNU Public license
 *    Version 2.  See also the file 'COPYING'.
 *
 *    Image rotation is a feature of Motion that can be used when the
 *    camera is mounted where a large section of the camera does not see
 *    useful space. An example usage would be the view through a peep hole, 
 *    processing performance can be improved to crop the image to only the 
 *    space with information in it

 *    Version history:
 *      v1 (2019)        - initial version
 */
#include "translate.h"
#include "crop.h"
#include <stdint.h>
#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#define bswap_32(x) bswap32(x)
#elif defined(__OpenBSD__)
#include <sys/types.h>
#define bswap_32(x) swap32(x)
#elif defined(__NetBSD__)
#include <sys/bswap.h>
#define bswap_32(x) bswap32(x)
#else
#include <byteswap.h>
#endif

static void crop(unsigned char* src, register unsigned char* dst,
		 int size_src, int size_dst,
                 int width_src,
		 //int width_src, int width_dst,
		 //int height_src, int height_dst,
		 int left, int right, int top, int bottom)
{
  unsigned char* src_endp, dst_endp;
  register unsigned char* base;

  src_endp = src + size_src;

  for (base = src + width_src*top; base < src_endp; base+=right) {
    for (base = base + left; base < base + left - right; base++) {
      *dst++ = *base;
    }
  }

}

/* * crop_init
 *
 *  Initializes crop data - allocates memory.
 *
 * Parameters:
 *
 *   cnt - the current thread's context structure
 *
 * Returns: nothing
 */
void crop_init(struct context *cnt){
    int size_norm, size_high;

    /* Make sure buffer_norm isn't freed if it hasn't been allocated. */
    cnt->crop_data.buffer_norm = NULL;
    cnt->crop_data.buffer_high = NULL;

    /*
     * Assign the values in conf.crop_left/right/top/bottom to crop_data.px_left/px_right/px_top/px_bottom. This way,
     * we have a value that is safe from changes caused by motion-control.
     */
    if (cnt->conf.crop_left < 0) {
        MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO
            ,_("Config option \"crop_left\" not positive number: %d")
            ,cnt->conf.crop_left);
        cnt->conf.crop_left = 0;     /* Disable crop. */
        cnt->crop_data.px_left = 0; /* Force return below. */
    } else {
        cnt->crop_data.px_left = cnt->conf.crop_left; 
    }

    if (cnt->conf.crop_right < 0) {
      MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO
		 ,_("Config option \"crop_right\" not positive number: %d")
		 ,cnt->conf.crop_right);
      cnt->conf.crop_right = 0;     /* Disable crop. */
      cnt->crop_data.px_left = 0; /* Force return below. */
    } else {
      cnt->crop_data.px_right = cnt->conf.crop_right;
    }


    if (cnt->conf.crop_top < 0) {
      MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO
		 ,_("Config option \"crop_top\" not positive number: %d")
		 ,cnt->conf.crop_top);
      cnt->conf.crop_top = 0;     /* Disable crop. */
      cnt->crop_data.px_top = 0; /* Force return below. */
    } else {
      cnt->crop_data.px_top = cnt->conf.crop_top;
    }


    if (cnt->conf.crop_bottom < 0) {
      MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO
		 ,_("Config option \"crop_bottom\" not positive number: %d")
		 ,cnt->conf.crop_bottom);
      cnt->conf.crop_bottom = 0;     /* Disable rotation. */
      cnt->crop_data.px_bottom = 0; /* Force return below. */
    } else {
      cnt->crop_data.px_bottom = cnt->conf.crop_bottom;
    }

    if (cnt->conf.crop_left + cnt->conf.crop_right > cnt->imgs.width) {
      MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO
		 ,_("Config option \"crop_left\" (%d) and \"crop_right\" (%d) take away more than the image width: %d")
		 ,cnt->conf.crop_left,cnt->conf.crop_right,cnt->imgs.width);
    }

    if (cnt->conf.crop_top + cnt->conf.crop_bottom > cnt->imgs.height) {
      MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO
		 ,_("Config option \"crop_top\" (%d) and \"crop_bottom\" (%d) take away more than the image height: %d")
		 ,cnt->conf.crop_top,cnt->conf.crop_bottom,cnt->imgs.height);
    }
    
    /*
     * Upon entrance to this function, imgs.width and imgs.height contain the
     * capture dimensions (as set in the configuration file, or read from a
     * netcam source).
     *
     * If rotating 90 or 270 degrees, the capture dimensions and output dimensions
     * are not the same. Capture dimensions will be contained in capture_width_norm and
     * capture_height_norm in cnt->rotate_data, while output dimensions will be contained
     * in imgs.width and imgs.height.
     */

    /* 1. Transfer capture dimensions into capture_width_norm and capture_height_norm. */
    cnt->crop_data.capture_width_norm  = cnt->imgs.width;
    cnt->crop_data.capture_height_norm = cnt->imgs.height;

    cnt->crop_data.capture_width_high  = cnt->imgs.width_high;
    cnt->crop_data.capture_height_high = cnt->imgs.height_high;

    size_norm = cnt->imgs.width * cnt->imgs.height * 3 / 2;
    size_high = cnt->imgs.width_high * cnt->imgs.height_high * 3 / 2;

    if ((cnt->rotate_data.degrees == 90) || (cnt->rotate_data.degrees == 270)) {
        /* 2. "Swap" imgs.width and imgs.height. */
        cnt->imgs.width = cnt->rotate_data.capture_height_norm;
        cnt->imgs.height = cnt->rotate_data.capture_width_norm;
        if (size_high > 0 ) {
            cnt->imgs.width_high = cnt->rotate_data.capture_height_high;
            cnt->imgs.height_high = cnt->rotate_data.capture_width_high;
        }
    }

    /*
     * If we're not cropping, let's exit once we have setup the capture dimensions
     * and output dimensions properly.
     */
    if (cnt->crop_data.px_left == 0)
      if (cnt->crop_data.px_right == 0)
	if (cnt->crop_data.px_top == 0)
	  if (cnt->crop_data.px_bottom == 0)
            return;
	    

    /*
     * Allocate memory if rotating 90 or 270 degrees, because those rotations
     * cannot be performed in-place (they can, but it would be too slow).
     */
    cnt->imgs.width = cnt->imgs.width
      - cnt->crop_data.px_left
      - cnt->crop_data.px_right;

    cnt->imgs.width_high = cnt->imgs.width_high
      - cnt->crop_data.px_left
      - cnt->crop_data.px_right;
    
    cnt->imgs.height = cnt->imgs.height
      - cnt->crop_data.px_top
      - cnt->crop_data.px_bottom;

    cnt->imgs.height_high = cnt->imgs.height_high
       - cnt->crop_data.px_top
       - cnt->crop_data.px_bottom;
     
    size_norm = cnt->imgs.width * cnt->imgs.height * 3 / 2;
    size_high = cnt->imgs.width_high * cnt->imgs.height_high * 3 / 2;
    
    cnt->crop_data.buffer_norm = mymalloc(size_norm);
    if (size_high > 0 ) cnt->rotate_data.buffer_high = mymalloc(size_high);

}

/**
 * crop_deinit
 *
 *  Frees resources previously allocated by crop_init.
 *
 * Parameters:
 *
 *   cnt - the current thread's context structure
 *
 * Returns: nothing
 */
void crop_deinit(struct context *cnt){

    if (cnt->crop_data.buffer_norm)
        free(cnt->crop_data.buffer_norm);

    if (cnt->crop_data.buffer_high)
        free(cnt->crop_data.buffer_high);
}

/**
 * crop_map
 *
 *  Main entry point for rotation.
 *
 * Parameters:
 *
 *   img_data- pointer to the image data to rotate
 *   cnt - the current thread's context structure
 *
 * Returns:
 *
 *   0  - success
 *   -1 - failure (shouldn't happen)
 */
int crop_map(struct context *cnt, struct image_data *img_data){
    /*
     * The image format is YUV 4:2:0 planar, which has the pixel
     * data is divided in three parts:
     *    Y - width x height bytes
     *    U - width x height / 4 bytes
     *    V - as U
     */

    int indx, indx_max;
    int wh, wh4 = 0, w2 = 0, h2 = 0;  /* width * height, width * height / 4 etc. */
    int whc, wh4c =0, w2c = 0, h2c =0; /* width * height after crop, width * height / 4 after crop etc. */
    int size, sizec, left, right, top, bottom;
    int width, height;
    int widthc, heightc;
    unsigned char *img;
    unsigned char *temp_buff;

    if ( cnt->crop_data.px_left == 0 )
      if ( cnt->crop_data.px_right == 0 )
	if ( cnt->crop_data.px_top == 0 )
	  if ( cnt->crop_data.px_bottom == 0 )
	    return 0;

    indx = 0;
    indx_max = 0;
    if ((cnt->crop_data.capture_width_high != 0) && (cnt->crop_data.capture_height_high != 0)) indx_max = 1;

    while (indx <= indx_max) {
        left   = cnt->crop_data.px_left;
	right  = cnt->crop_data.px_right;
	top    = cnt->crop_data.px_top;
	bottom = cnt->crop_data.px_bottom;
        wh4 = 0;
        w2 = 0;
        h2 = 0;
        if (indx == 0 ){
            img = img_data->image_norm;
            width = cnt->crop_data.capture_width_norm;
            height = cnt->crop_data.capture_height_norm;
            temp_buff = cnt->crop_data.buffer_norm;
        } else {
            img = img_data->image_high;
            width = cnt->crop_data.capture_width_high;
            height = cnt->crop_data.capture_height_high;
            temp_buff = cnt->crop_data.buffer_high;
        }
	widthc  = width - left - right;
        heightc = height - left - right;
        /*
         * Pre-calculate some stuff:
         *  wh   - size of the Y plane
         *  size - size of the entire memory block
         *  wh4  - size of the U plane, and the V plane
         *  w2   - width of the U plane, and the V plane
         *  h2   - as w2, but height instead
         */
        wh = width * height;
	whc = widthc * heightc;
        size = wh * 3 / 2;
	sizec = whc * 3 / 2;
        wh4 = wh / 4;
	wh4c = whc / 4;
        w2 = width / 2;
	w2c = widthc / 2;
        h2 = height / 2;
	h2c = heightc / 2;

	crop(img           , temp_buff             , wh  , whc  , width,/*width, widthc, height, heightc,*/ left, right, top, bottom);
	crop(img + wh      , temp_buff + whc       , wh4 , wh4c , width,/*w2   , w2c   , h2    , h2c    ,*/ left, right, top, bottom);
	crop(img + wh + wh4, temp_buff + whc + wh4c, wh4 , wh4c , width,/*w2   , w2c   , h2    , h2c    ,*/ left, right, top, bottom);
        memcpy(img, temp_buff, sizec);

        indx++;       
    }

    return 0;
}

