/*
 * conf_tester.cpp
 *
 *  Created on: Sep 13, 2010
 *      Author: abachrac
 */
// reading a complete binary file

#include <stdio.h>
#include <stdlib.h>

#include <bot_param/param_client.h>
#include <lcmtypes/bot2_param.h>

int main()
{
  lcm_t * lcm = lcm_create(NULL);

  BotParam * param = bot_param_new_from_server(lcm, 1);
  bot_param_write(param, stderr);
  if (param == NULL) {
    fprintf(stderr, "could not get params!\n");
    exit(1);
  }
  double foo = bot_param_get_double_or_fail(param, "foo");
  double bar = bot_param_get_double_or_fail(param, "bar");
  printf("foo=%f, bar = %f\n", foo, bar);

  while (1) {
    lcm_handle(lcm);
    char * key = "coordinate_frames.body.history";
    fprintf(stderr, "%s = %d\n", key, bot_param_get_int_or_fail(param, key));
  }
  return 0;
}
