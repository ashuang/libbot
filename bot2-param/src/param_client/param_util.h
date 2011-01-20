#ifndef __ARCONF_H__
#define __ARCONF_H__

#include <bot_core/bot_core.h>
#include <bot_param/param_client.h>

#ifdef __cplusplus
extern "C" {
#endif

  /* ================ general ============== */

  int bot_param_get_translation(BotParam *bot_param, const char *name, double translation[3]);
  int bot_param_get_quat(BotParam * bot_param, const char *name, double quat[4]);
  int bot_param_get_trans(BotParam * bot_param, const char *name, BotTrans *trans);
  int bot_param_get_matrix_4_4(BotParam * bot_param, const char *name, double m[16]);

  /* ================ cameras ============== */
  char** bot_param_get_all_camera_names(BotParam *bot_param);
  int bot_param_get_camera_calibration_config_prefix(BotParam *bot_param, const char *cam_name, char *result,
      int result_size);
  BotCamTrans* bot_param_get_new_camtrans(BotParam *bot_param, const char *cam_name);

  //TODO: this should be formalized & generalized
  char *bot_param_get_camera_thumbnail_channel(BotParam *bot_param, const char *camera_name);
  char * bot_param_cam_get_name_from_lcm_channel(BotParam *bot_param, const char *channel);

  /* ================ lidar ============== */

  char** bot_param_get_all_planar_lidar_names(BotParam *bot_param);
  int bot_param_get_planar_lidar_config_path(BotParam *bot_param, const char *plidar_name, char *result,
      int result_size);

#ifdef __cplusplus
}
#endif

#endif
