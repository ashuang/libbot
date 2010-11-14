/*
 * conf_tester.cpp
 *
 *  Created on: Sep 13, 2010
 *      Author: abachrac
 */
// reading a complete binary file

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <lcm/lcm.h>
#include <bot_param_client/param_client.h>
#include <sys/time.h>
#include "../param_client/misc_utils.h"

#include <lcmtypes/bot2_param.h>

int main(int argc, char ** argv)
{

  lcm_t * lcm = lcm_create(NULL); //TODO: provider options?

  if (argc != 3) {
    fprintf(stderr, "usage:\n %s <key> <param>\n", argv[0]);
    exit(1);
  }

  BotParam * param = bot_param_new_from_server(lcm,0);
  bot_param_set_t msg;
  msg.utime = _timestamp_now();
  msg.sequence_number = bot_param_client_get_seqno(param);
  msg.server_id = bot_param_client_get_server_id(param);
  msg.key = argv[1];
  msg.value = argv[2];
  bot_param_set_t_publish(lcm,"PRM_SET",&msg);

  return 0;
}
