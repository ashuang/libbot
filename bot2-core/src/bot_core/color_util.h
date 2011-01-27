#ifndef BOT_COLOR_UTIL
#define BOT_COLOR_UTIL
/**
 * @defgroup BotColorUtil Color Utilities
 * @brief utilities for making color schemes
 * @include: bot_core/bot_core.h
 *
 * Linking: `pkg-config --libs bot2-core`
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

  /*
   * Get a random color
   */
  void bot_color_util_rand_color(float f[4], double alpha, double min_intensity);

  /*
   * Get the color in the JET color space associated with value v [0,1]
   */
  float *bot_color_util_jet(double v);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif
