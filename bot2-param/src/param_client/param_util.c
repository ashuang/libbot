#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "param_util.h"

/* ================ cameras ============== */
char *
bot_param_get_camera_thumbnail_channel(BotParam *bot_param, const char *camera_name)
{
  char key[1024];
  snprintf(key, sizeof(key), "cameras.%s.thumbnail_channel", camera_name);
  return bot_param_get_str_or_fail(bot_param, key);
}

char**
bot_param_get_all_camera_names(BotParam *bot_param)
{
  return bot_param_get_subkeys(bot_param, "cameras");
}

int bot_param_get_camera_calibration_config_prefix(BotParam *bot_param, const char *cam_name, char *result,
    int result_size)
{
  snprintf(result, result_size, "calibration.cameras.%s", cam_name);
  return bot_param_has_key(bot_param, result) ? 0 : -1;
}

BotCamTrans*
bot_param_get_new_camtrans(BotParam *bot_param, const char *cam_name)
{
  char prefix[1024];
  int status = bot_param_get_camera_calibration_config_prefix(bot_param, cam_name, prefix, sizeof(prefix));
  if (0 != status)
    goto fail;

  char key[2048];
  double width;
  sprintf(key, "%s.width", prefix);
  if (0 != bot_param_get_double(bot_param, key, &width))
    goto fail;

  double height;
  sprintf(key, "%s.height", prefix);
  if (0 != bot_param_get_double(bot_param, key, &height))
    goto fail;

  double pinhole_params[5];
  snprintf(key, sizeof(key), "%s.pinhole", prefix);
  if (5 != bot_param_get_double_array(bot_param, key, pinhole_params, 5))
    goto fail;
  double fx = pinhole_params[0];
  double fy = pinhole_params[1];
  double cx = pinhole_params[3];
  double cy = pinhole_params[4];
  double skew = pinhole_params[2];

  double position[3];
  sprintf(key, "%s.position", prefix);
  if (3 != bot_param_get_double_array(bot_param, key, position, 3))
    goto fail;

  sprintf(key, "cameras.%s", cam_name);
  double orientation[4];
  if (0 != bot_param_get_quat(bot_param, key, orientation))
    goto fail;

  char *ref_frame;
  sprintf(key, "%s.relative_to", prefix);
  if (0 != bot_param_get_str(bot_param, key, &ref_frame))
    goto fail;

  char * distortion_model;
  sprintf(key, "%s.distortion_model", prefix);
  if (0 != bot_param_get_str(bot_param, key, &distortion_model))
    goto fail;

  if (strcmp(distortion_model, "spherical") == 0) {
    double distortion_param;
    sprintf(key, "%s.distortion_params", prefix);
    if (1 != bot_param_get_double_array(bot_param, key, &distortion_param, 1))
      goto fail;

    BotDistortionObj* sph_dist = bot_spherical_distortion_create(distortion_param);
    BotCamTrans* sph_camtrans = bot_camtrans_new(cam_name, width, height, fx, fy, cx, cy, skew, sph_dist);
    return sph_camtrans;
  }
  else if (strcmp(distortion_model, "plumb-bob") == 0) {
    double dist_k[3];
    sprintf(key, "%s.distortion_k", prefix);
    if (3 != bot_param_get_double_array(bot_param, key, dist_k, 3))
      goto fail;

    double dist_p[2];
    sprintf(key, "%s.distortion_p", prefix);
    if (2 != bot_param_get_double_array(bot_param, key, dist_p, 2))
      goto fail;

    BotDistortionObj* pb_dist = bot_plumb_bob_distortion_create(dist_k[0], dist_k[1], dist_k[2], dist_p[0], dist_p[1]);
    BotCamTrans* pb_camtrans = bot_camtrans_new(cam_name, width, height, fx, fy, cx, cy, skew, pb_dist);
    return pb_camtrans;
  }

  fail: return NULL;
}

char * bot_param_cam_get_name_from_lcm_channel(BotParam *cfg, const char *channel)
{
  char * cam_name = NULL;

  char **cam_names = bot_param_get_subkeys(cfg, "cameras");

  for (int i = 0; cam_names && cam_names[i]; i++) {
    char *thumb_key = g_strdup_printf("cameras.%s.thumbnail_channel", cam_names[i]);
    char *cam_thumb_str = NULL;
    int key_status = bot_param_get_str(cfg, thumb_key, &cam_thumb_str);
    free(thumb_key);

    if ((0 == key_status) && (0 == strcmp(channel, cam_thumb_str))) {
      strcpy(cam_name, cam_names[i]);
      break;
    }

    char *full_key = g_strdup_printf("cameras.%s.full_frame_channel", cam_names[i]);
    char *cam_full_str = NULL;
    key_status = bot_param_get_str(cfg, full_key, &cam_full_str);
    free(full_key);

    if ((0 == key_status) && (0 == strcmp(channel, cam_full_str))) {
      strcpy(cam_name, cam_names[i]);
      break;
    }
  }
  g_strfreev(cam_names);

  return cam_name;
}

/* ================ planar lidar ============== */
char**
bot_param_get_all_planar_lidar_names(BotParam *bot_param)
{
  return bot_param_get_subkeys(bot_param, "planar_lidars");
}

int bot_param_get_planar_lidar_config_path(BotParam *bot_param, const char *plidar_name, char *result, int result_size)
{
  int n = snprintf(result, result_size, "planar_lidars.%s", plidar_name);
  if (n >= result_size)
    return -1;
  if (bot_param)
    return bot_param_has_key(bot_param, result) ? 0 : -1;
  else
    return 0;
}

/* ================ general ============== */
int bot_param_get_quat(BotParam *bot_param, const char *name, double quat[4])
{
  char key[256];
  sprintf(key, "%s.quat", name);
  if (bot_param_has_key(bot_param, key)) {
    int sz = bot_param_get_double_array(bot_param, key, quat, 4);
    assert(sz == 4);
    return 0;
  }

  sprintf(key, "%s.rpy", name);
  if (bot_param_has_key(bot_param, key)) {
    double rpy[3];
    int sz = bot_param_get_double_array(bot_param, key, rpy, 3);
    assert(sz == 3);
    for (int i = 0; i < 3; i++)
      rpy[i] = bot_to_radians (rpy[i]);
    bot_roll_pitch_yaw_to_quat(rpy, quat);
    return 0;
  }

  sprintf(key, "%s.rodrigues", name);
  if (bot_param_has_key(bot_param, key)) {
    double rod[3];
    int sz = bot_param_get_double_array(bot_param, key, rod, 3);
    assert(sz == 3);
    bot_rodrigues_to_quat(rod, quat);
    return 0;
  }

  sprintf(key, "%s.angleaxis", name);
  if (bot_param_has_key(bot_param, key)) {
    double aa[4];
    int sz = bot_param_get_double_array(bot_param, key, aa, 4);
    assert(sz == 4);

    bot_angle_axis_to_quat(aa[0], aa + 1, quat);
    return 0;
  }
  return -1;
}

int bot_param_get_trans_vec(BotParam *bot_param, const char *name, double pos[3])
{
  char key[256];
  sprintf(key, "%s.trans_vec", name);
  if (bot_param_has_key(bot_param, key)) {
    int sz = bot_param_get_double_array(bot_param, key, pos, 3);
    assert(sz == 3);
    return 0;
  }
  // not found
  return -1;
}

int bot_param_get_trans(BotParam *bot_param, const char *name, BotTrans * trans)
{
  if (bot_param_get_quat(bot_param, name, trans->rot_quat))
    return -1;

  if (bot_param_get_trans_vec(bot_param, name, trans->trans_vec))
    return -1;

  return 0;
}

int bot_param_get_matrix_4_4(BotParam *bot_param, const char *name, double m[16])
{
  BotTrans trans;
  if (bot_param_get_trans(bot_param, name, &trans))
    return -1;

  bot_trans_get_mat_4x4(&trans, m);

  return 0;
}
