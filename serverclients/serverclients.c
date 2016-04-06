/* Copyright (c) 2010-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <unistd.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

#define BUFSIZE 1024

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

int server(int argc, char *argv[]);
int client(int argc, char *argv[]);

extern inline double poisson_interval_generator(double mean);

int number_of_deadline_violations = 0;

/** Server function  */
int server(int argc, char *argv[]) {

  _XBT_GNUC_UNUSED int res;
  _XBT_GNUC_UNUSED int read;

  msg_task_t task = NULL;
  char mailbox_rcv[256];
  int id = -1;
  int number_of_clients = atoi(argv[2]);

  read = sscanf(argv[1], "%d", &id);
  xbt_assert(read, "Invalid argument %s\n", argv[1]);
  
  sprintf(mailbox_rcv, "server");

  int count = 0;

  char task_name[128];
  char* p_client_number;
  char* p_task_name;
  int client_number;

  while(count != number_of_clients) {
    /* Receives a task */
    res = MSG_task_receive(&(task), mailbox_rcv); 
    //res = MSG_task_receive_with_timeout(&(task), mailbox_rcv, 0.01); 
    
    xbt_assert(res == MSG_OK, "MSG_task_get failed");

    //XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
    //XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));

    snprintf(task_name, sizeof(task_name), "%s", MSG_task_get_name(task));
    p_task_name = strdup(task_name);
    p_client_number = strsep(&p_task_name, "_");
    p_client_number = strsep(&p_task_name, "_");
    client_number = atoi(p_client_number);

    /* Executes a task */
    MSG_task_execute(task);
    
    char mailbox_snd[80];
    sprintf(mailbox_snd, "client_%d", client_number);
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_send(finalize, mailbox_snd);
    
    //XBT_INFO("\"%s\" done", MSG_task_get_name(task));

    count++;

    MSG_task_destroy(task);
    task = NULL;
  }

  return 0;
} 

/** Client function  */
int client(int argc, char *argv[]) {

  int client_number = atoi(argv[1]);
  double task_comp_size = atof(argv[2]); 
  double task_comm_size = atof(argv[3]);

  char mailbox_snd[256];
  char sprintf_buffer[256];
  msg_task_t task_s = NULL;
  msg_task_t task_r = NULL;

  _XBT_GNUC_UNUSED int res;
  _XBT_GNUC_UNUSED int read;

  double task_time = 0.0;

  MSG_process_sleep(poisson_interval_generator(10.0));
  sprintf(mailbox_snd, "server");
  sprintf(sprintf_buffer, "task_%d", client_number);
  task_s = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);

  //XBT_INFO("Sending \"%s\" (of client %d) to mailbox \"%s\"", task->name, client_number, mailbox);

  task_time = MSG_get_clock();
  MSG_task_send(task_s, mailbox_snd);

  //MSG_task_send_with_timeout(task_s, mailbox_snd, 0.01);
  while(1) {
    char mailbox_recv[80];
    sprintf(mailbox_recv, "client_%d", client_number);
    res = MSG_task_receive(&(task_r), mailbox_recv);
    xbt_assert(res == MSG_OK, "MSG_task_get failed");
    //printf("%g\n",MSG_get_clock()-task_time);
    if(MSG_get_clock()-task_time > 0.8) {
      number_of_deadline_violations++;
    }
    if (!strcmp(MSG_task_get_name(task_r), "finalize")) {
      MSG_task_destroy(task_r);
      break;
    }
  }
  
  //XBT_INFO("Client %d exiting.\n", client_number);
  
  return 0;
} 

/** Main function */
int main(int argc, char *argv[]) {
  char buffer[BUFSIZE];
  msg_error_t res;
  const char *platform_file;
  const char *application_file;

  int fd;
  int child;

  MSG_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
  platform_file = argv[1];
  application_file = argv[2];
  fd = atoi(argv[3]);

  { /*  Simulation setting */
    MSG_create_environment(platform_file);
  }
  { /*   Application deployment */
    MSG_function_register("server", server);
    MSG_function_register("client", client);
    //MSG_process_set_data(master,p_values);
    MSG_launch_application(application_file);
  }
  res = MSG_main();

  //printf("%d\n",number_of_deadline_violations);

  //snprintf(buffer, BUFSIZE, "[%s,%g]", "avg", MSG_get_clock());  
  
  snprintf(buffer, BUFSIZE, "%s,%g,%s,%d", "tt", MSG_get_clock(), "dv", number_of_deadline_violations);

  //snprintf(buffer, BUFSIZE, "%g", MSG_get_clock());
  write(fd, buffer, sizeof(buffer));


  if(res == MSG_OK)
    return 0;
  else
    return 1;

}
