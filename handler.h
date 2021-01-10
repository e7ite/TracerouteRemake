// Using POSIX standard API
#define _GNU_SOURCE 1

#ifndef HANDLER_H
#define HANDLER_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/errqueue.h>
#include <linux/icmp.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h> 
#include <limits.h>

#define ERRNO_SET 1
#define ERRNO_NOT_SET 0

// Global File descriptor handler
struct FileDescriptorTable 
{
    int fds[10];
    int fdcount;
};
extern struct FileDescriptorTable gfd;

/**
 * @brief Exits program and sends a message informing about the problem
 * @param errmsg A description of the error that occurred. Adds newline to msg
 * @param errno Set if the perpetrator sets errno
 * @param ... Variadic argument which takes file descriptors to close
 *
 * This function is responsible for exiting the program when a problem
 * occurs and cannot continue. Will output a description of the error 
 * using perror if possible
**/
void __attribute((noreturn)) HandleError(const char *errmsg, char errno);

/**
 * @brief Adds a file descriptor to file descriptor table
 * @param fd New file descriptor
 * 
 * This function will add the file descriptor to the global file descriptor
 * table and will increment the file descriptor count. This function should
 * be called creation of a file descriptor throughout the program. If 10 file
 * descriptors are open already and the user tried to add, the program will 
 * terminate and state the problem.
**/
void AddFileDescriptor(int fd);

/**
 * @brief Closes all the file descriptors open
 * 
 * This function is responsible for closing all the file descriptors
 * left open by the user. If the user fails to call the AddFileDescriptor to 
 * add a file descriptor to table, the program will not close the file
 * descriptor properly
**/
void __attribute__((noreturn)) CloseFileDescriptors(int signal);

#endif