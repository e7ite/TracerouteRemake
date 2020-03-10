#include "handler.h"

int main(int argc, const char** argv)
{
    int hops = 1;
    char orighostname[NI_MAXHOST];
    struct sockaddr_in connection;
    memset(&connection, 0, sizeof(connection));
    memset(orighostname, 0, sizeof(orighostname));
    
    // Setup the file descriptor table and set interrupt handler
    // to close file descriptors
    struct sigaction handler;
    memset(&handler, 0, sizeof(struct sigaction));
    handler.sa_handler = CloseFileDescriptors;
    sigaction(SIGINT, &handler, NULL);
    sigaction(SIGPIPE, &handler, NULL);

    // Flush FILE buffers
    fflush(stdout);
    fflush(stderr);
    
    // Check if host name was specified via cmd args
    if (argc != 2)
    {
        char errbuf[NI_MAXHOST];
        snprintf(errbuf, 0x40, "USAGE: %s <HOST>", argv[0]);
        HandleError(errbuf, ERRNO_NOT_SET);
    }

    // Get linked list of available connections to connect to host using
    // any port available
    int errcode;
    struct addrinfo* result;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_flags = 0;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    if ((errcode = getaddrinfo(argv[1], "80", &hints, &result)))
        HandleError(gai_strerror(errcode), ERRNO_NOT_SET);

    // Iterate through linked list of addresses and attempt to setup a 
    // a UDP connection socket
    int sfd;
    struct addrinfo* addrnode = result;
    for (; addrnode; addrnode = addrnode->ai_next)
        // Attempt to initialize socket with info specified by address node
        if ((sfd = socket(addrnode->ai_family, 
                            addrnode->ai_socktype, addrnode->ai_protocol)) != -1)
            break;

    if (!addrnode)
        HandleError("Tried all addresses and failed to connect", ERRNO_NOT_SET);
    if (addrnode->ai_addrlen != sizeof(connection))
        HandleError("Socket address != sizeof(struct sockaddr)", ERRNO_NOT_SET);

    memcpy(&connection, addrnode->ai_addr, sizeof(connection));
    AddFileDescriptor(sfd);
    freeaddrinfo(result);

    // Set ability to receive ICMP message
    if (setsockopt(sfd, IPPROTO_IP, IP_RECVERR, &hops, sizeof(int)) == -1)
        HandleError("Failed to set extended error receiving", ERRNO_SET);

    for (; hops <= 30; hops++)
    {
        // Set max hops
        if (setsockopt(sfd, IPPROTO_IP, IP_TTL, &hops, sizeof(int)) == -1)
            HandleError("Failed to set max hops", ERRNO_SET);

        // Probe the seerver three times
        const char* msg = "hello";
        for (int i = 0; i < 3; i++)
        {
            int result = sendto(sfd, msg, strlen(msg) + 1, 0, 
                            &connection, sizeof(connection));
            if (result != -1)
                break;
            if (i == 2)
                HandleError("Failed to probe server", ERRNO_SET);
        }

        // Buffer to store original ICMP packet 
        struct iovec iov;
        struct icmphdr icmph;
        memset(&icmph, 0, sizeof(icmph));
        iov.iov_base = &icmph;
        iov.iov_len = sizeof(struct icmphdr);

        // Store place for cmsg header
        char cmsgbuf[1024];
        struct sockaddr senderaddr;
        memset(&cmsgbuf, 0, sizeof(cmsgbuf));
        memset(&senderaddr, 0, sizeof(senderaddr));

        struct msghdr recvhints;
        memset(&recvhints, 0, sizeof(recvhints));
        // Where to store the sender name
        recvhints.msg_name = &senderaddr;
        recvhints.msg_namelen = sizeof(senderaddr);
        // Where to store ancillary data
        recvhints.msg_control = &cmsgbuf;
        recvhints.msg_controllen = sizeof(cmsgbuf);
        // Where to store the original packet and how many
        recvhints.msg_iov = &iov;
        recvhints.msg_iovlen = 1;

        // Loop until ancillary packet is received or timer ends
        int timer = 100000;
        while (recvmsg(sfd, &recvhints, MSG_ERRQUEUE) == -1 && timer--);

        // Extract error message from ancillary message
        char msgReceived = 0;
        for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&recvhints);
                cmsg; cmsg = CMSG_NXTHDR(&recvhints, cmsg))
        {
            // Is error message on IP level
            if (cmsg->cmsg_level == IPPROTO_IP)
            {
                // Is error message from extended reliable error message passing 
                if (cmsg->cmsg_type == IP_RECVERR)
                {
                    // Go to data section of ancillary message to extract message 
                    struct sock_extended_err* err = (struct sock_extended_err*)CMSG_DATA(cmsg);       
                    if (!err)
                        continue;             
                    
                    // Was error message from ICMP protocol
                    if (err->ee_origin == SO_EE_ORIGIN_ICMP)
                    {
                        // Is TTL expired (continue) or port unreachable (exit)
                        // (ICMP specific)
                        if (err->ee_type == ICMP_PORT_UNREACH 
                            || err->ee_type == ICMP_TIME_EXCEEDED)
                        {
                            msgReceived = 1;

                            // Extract source of error from error msg
                            struct sockaddr* errsrc = SO_EE_OFFENDER(err);
                            if (!errsrc || errsrc->sa_family == AF_UNSPEC)
                                continue;
                            
                            // Perform reverse DNS lookup to get host name
                            char hostbuf[NI_MAXHOST];
                            if (!getnameinfo(errsrc, sizeof(*errsrc), hostbuf, 
                                    NI_MAXHOST, NULL, 0, 0))
                                printf("%i: Host: %s\n", hops, hostbuf);

                            // If ICMP error code is port unreachable, then we have
                            // reached destination. close program 
                            if (err->ee_type == ICMP_DEST_UNREACH)
                                CloseFileDescriptors(EXIT_SUCCESS);
                        }
                    }
                }
            }
        }

        // If recv timer runs out of we did not get a dest reached or timeout
        // print out a message
        if (!msgReceived || !timer)
            printf("%i: Did not get response\n", hops);
    }

    // Close file descriptors and exit program
    CloseFileDescriptors(EXIT_SUCCESS);
}