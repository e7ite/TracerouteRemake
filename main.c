#include "handler.h"

int main(int argc, const char** argv)
{
    // Setup the file descriptor table and set interrupt handler
    // to close file descriptors
    struct sigaction handler;
    memset(&handler, 0, sizeof(struct sigaction));
    memset(&gfd, 0, sizeof(struct FileDescriptorTable));
    handler.sa_handler = &CloseFileDescriptors;
    sigaction(SIGINT, &handler, NULL);
    sigaction(SIGPIPE, &handler, NULL);

    // Flush FILE buffers
    fflush(stdout);
    fflush(stderr);
    
    if (argc != 2)
    {
        char errbuf[0x40];
        snprintf(errbuf, 0x40, "USAGE: %s <HOST>", argv[0]);
        HandleError(errbuf, ERRNO_NOT_SET);
    }

    // Get linked list of available connections to connect to host using
    // any port available
    int errcode;
    struct addrinfo* result;
    struct addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = 0;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    if ((errcode = getaddrinfo(argv[1], NULL, &hints, &result)))
        HandleError(gai_strerror(errcode), ERRNO_NOT_SET);

    // Iterate through linked list of addresses and attempt to setup a 
    // a UDP connectionsocket
    int sfd;
    struct addrinfo* addrnode = result;
    for (; addrnode; addrnode = addrnode->ai_next)
        // Attempt to initialize socket with info specified by address node
        if ((sfd = socket(addrnode->ai_family, 
                        addrnode->ai_socktype, addrnode->ai_protocol)) != -1)
            break;

    if (!addrnode)
        HandleError("Tried all addresses and failed to connect", ERRNO_NOT_SET);

    freeaddrinfo(result);

    AddFileDescriptor(sfd);

    // Set max hops
    int options = 30;
    if (setsockopt(sfd, IPPROTO_IP, IP_TTL, &options, sizeof(int)) == -1)
        HandleError("Failed to set max hops", ERRNO_SET);

    // Set ability to receive ICMP message
    options = 1;
    if (setsockopt(sfd, IPPROTO_IP, IP_RECVERR, &options, sizeof(int)) == -1)
        HandleError("Failed to set extended error receiving", ERRNO_SET);

    const char* msg = "hello";
    if (sendto(sfd, msg, strlen(msg) + 1, 0, addrnode->ai_addr, addrnode->ai_addrlen) == -1)
        HandleError("Failed to send UDP message", ERRNO_SET);

    char sendername[0x40];
    struct cmsghdr perrhdr;
    memset(sendername, 0, 0x40);
    memset(&perrhdr, 0, sizeof(perrhdr));

    struct msghdr recvhints;
    memset(&recvhints, 0, sizeof(recvhints));
    // Where to store the sender name
    recvhints.msg_name = sendername;
    recvhints.msg_namelen = sizeof(sendername);
    // Where to store ancillary data
    recvhints.msg_control = &perrhdr;
    recvhints.msg_controllen = sizeof(perrhdr);
    
    int recvcount;
    if ((recvcount = recvmsg(sfd, &recvhints, MSG_ERRQUEUE)) == -1)
        HandleError("Problem receiving msg", ERRNO_NOT_SET);

    printf("%i\n", recvcount);
    puts(recvhints.msg_name);

    // Close file descriptors and exit program
    CloseFileDescriptors(EXIT_SUCCESS);
}