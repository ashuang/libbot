/*
 * conf_tester.cpp
 *
 *  Created on: Sep 13, 2010
 *      Author: abachrac
 */
// reading a complete binary file

#include <stdio.h>
#include <stdlib.h>

#include <bot_param_client/param_client.h>
#include <lcmtypes/bot2_param.h>

int main()
{
  lcm_t * lcm = lcm_create(NULL);

  BotParam * param = bot_param_new_from_server(lcm,1);
  if (param==NULL){
    fprintf(stderr,"could not get params!\n");
    exit(1);
  }

  while (1){
    lcm_handle(lcm);
    char * key = "planar_lidars.LASER.serial";
    fprintf(stderr,"%s = %i\n",key,bot_param_get_int_or_fail(param,key));
  }
  return 0;
}
