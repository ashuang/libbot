#ifndef __ar_bot_coord_frames_h__
#define __ar_bot_coord_frames_h__

#include <bot_core/bot_core.h>
#include <bot_param/param_client.h>
#include <lcmtypes/quad_lcmtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * BotCoordFrames:
 *
 * BotCoordFrames is intended to be a one-stop shop for coordinate frame transformations.
 *
 *
 * It assumes that there is a block in the param file specifying the layout of the coordinate frames.
 * For example:
 *
 coordinate_frames {
   local {
    #name the root frame, block should be empty
   }
   body {
     relative_to = "local";
     history = 1000;                    #number of past transforms to keep around,
     update_channel = "BODY_TO_LOCAL";  #transform updates will be listened for on this channel
     initial_transform{
       trans_vec = [ 0, 0, 0 ];         #(x,y,z) translation vector
       quat = [ 1, 0, 0, 0 ];           #may be specified as a quaternion, rpy, rodrigues, or axis-angle
     }
   }
   laser {
     relative_to = "body";
     history = 0;                       #if set to 0, transform will not be updated
     update_channel = "";               #ignored since history=0
     initial_transform{
       trans_vec = [ 0, 0, 0 ];
       rpy = [ 0, 0, 0 ];
     }
   }
   camera {
     relative_to = "body";
     history = 0;
     initial_transform{
       trans_vec = [ 0, 0, 0 ];
       rodrigues = [ 0, 0, 0 ];
     }
   }
  #etc...
}
 *
 *
 */
typedef struct _BotCoordFrames BotCoordFrames;

BotCoordFrames * bot_coord_frames_new(lcm_t *lcm, BotParam *config);

void bot_coord_frames_destroy(BotCoordFrames * bot_coord_frames);



BotCoordFrames * bot_coord_frames_get_global(lcm_t *lcm, BotParam *config);


/**
 * compute the latest rigid body transformation from one coordinate frame to
 * another.
 *
 * Returns: 1 on success, 0 on failure
 */
int bot_coord_frames_get_trans(BotCoordFrames *bot_coord_frames, const char *from_frame,
        const char *to_frame, BotTrans *result);

/**
 * compute the rigid body transformation from one coordinate frame
 * to another.
 *
 * Returns: 1 on success, 0 on failure
 */
int bot_coord_frames_get_trans_with_utime(BotCoordFrames *bot_coord_frames, const char *from_frame,
        const char *to_frame, int64_t utime, BotTrans *result);


/**
 *
 * Get the timestamp of the most recent transform
 *
 * Returns: 1 if the requested transformation is availabe, 0 if not
 */
int
bot_coord_frames_get_trans_latest_timestamp(BotCoordFrames *bot_coord_frames, const char *from_frame, const char *to_frame,
    int64_t *timestamp);


/**
 * Returns: 1 if the requested transformation is availabe, 0 if not
 */
int bot_coord_frames_have_trans(BotCoordFrames *bot_coord_frames, const char *from_frame,
        const char *to_frame);

// convenience function
int bot_coord_frames_get_trans_mat_3x4(BotCoordFrames *bot_coord_frames, const char *from_frame,
        const char *to_frame, double mat[12]);

// convenience function
int bot_coord_frames_get_trans_mat_3x4_with_utime(BotCoordFrames *bot_coord_frames,
        const char *from_frame, const char *to_frame, int64_t utime,
        double mat[12]);

// convenience function
int bot_coord_frames_get_trans_mat_4x4(BotCoordFrames *bot_coord_frames, const char *from_frame,
        const char *to_frame, double mat[16]);

// convenience function
int bot_coord_frames_get_trans_mat_4x4_with_utime(BotCoordFrames *bot_coord_frames,
        const char *from_frame, const char *to_frame, int64_t utime,
        double mat[16]);


/**
 * Transforms a vector from one coordinate frame to another
 *
 * Returns: 1 on success, 0 on failure
 */
int bot_coord_frames_transform_vec(BotCoordFrames *bot_coord_frames, const char *from_frame,
        const char *to_frame, const double src[3], double dst[3]);

/**
 * Rotates a vector from one coordinate frame to another.  Does not apply
 * any translations.
 *
 * Returns: 1 on success, 0 on failure
 */
int bot_coord_frames_rotate_vec(BotCoordFrames *bot_coord_frames, const char *from_frame,
        const char *to_frame, const double src[3], double dst[3]);

/**
 * Retrieves the number of transformations available for the specified link.
 * Only valid for <from_frame, to_frame> pairs that are directly linked.  e.g.
 * <body, local> is valid, but <camera, local> is not.
 */
int bot_coord_frames_get_n_trans(BotCoordFrames *bot_coord_frames, const char *from_frame,
        const char *to_frame, int nth_from_latest);

/**
 * @nth_from_latest:  0 corresponds to most recent transformation update
 * @btrans: may be NULL
 * @timestamp: may be NULL
 *
 * Returns: 1 on success, 0 on failure
 */
int bot_coord_frames_get_nth_trans(BotCoordFrames *bot_coord_frames, const char *from_frame,
        const char *to_frame, int nth_from_latest,
        BotTrans *btrans, int64_t *timestamp);

#ifdef __cplusplus
}
#endif

#endif
