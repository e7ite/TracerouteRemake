#include "handler.h"

struct FileDescriptorTable gfd;

void HandleError(const char *errmsg, char errno)
{
    // Print an error message
    if (errno)
    {
	    perror(errmsg);
    }
    else
    {
   	    fputs(errmsg, stderr);
	    fputs("\n", stderr);
    }
    
    // Close all the file descrptors and returns
    CloseFileDescriptors(EXIT_FAILURE);
}

void AddFileDescriptor(int fd)
{
    if (gfd.fdcount == 9)
        HandleError("Can't open any more file descriptors!", ERRNO_NOT_SET);

    gfd.fds[gfd.fdcount++] = fd;
}

void CloseFileDescriptors(int signal)
{
    for (int i = 0; i < gfd.fdcount; i++)
        close(gfd.fds[i]);
    exit(signal);
}
