/*
 *    crop.h
 *
 *    Include file for handling image cropping.
 *
 *    Copyright 2019, Joseph McKenna
 *
 *    This software is distributed under the GNU Public license
 *    Version 2.  See also the file 'COPYING'.
 */
#ifndef _INCLUDE_CROP_H
#define _INCLUDE_CROP_H

#include "motion.h" /* for struct context */

/**
 * crop_init
 *
 *  Sets up rotation data by allocating a temporary buffer for 90/270 degrees
 *  rotation, and by determining the right rotate-180-degrees function.
 *
 * Parameters:
 *
 *  cnt - current thread's context structure
 *
 * Returns: nothing
 */
void crop_init(struct context *cnt);

/**
 * rotate_deinit
 *
 *  Frees memory allocated by rotate_init.
 *
 * Parameters:
 *
 *   cnt - current thread's context structure
 */
void crop_deinit(struct context *cnt);

/**
 * rotate_map
 *
 *  Rotates the image stored in img according to the rotation data
 *  available in cnt. Rotation is performed clockwise. Supports 90,
 *  180 and 270 degrees rotation. 180 degrees rotation is performed
 *  in-place by simply reversing the image data, which is a very
 *  fast operation. 90 and 270 degrees rotation are performed using
 *  a temporary buffer and a somewhat more complicated algorithm,
 *  which makes them slower.
 *
 *  Note that to the caller, all rotations will seem as they are
 *  performed in-place.
 *
 * Parameters:
 *
 *   img_data - the image data to rotate
 *   cnt - current thread's context structure
 *
 * Returns:
 *
 *   0  - success
 *   -1 - failure (rare, shouldn't happen)
 */
int crop_map(struct context *cnt, struct image_data *img_data);

#endif
