# TracerouteRemake

This Linux program was made to recreate `traceroute(1)` with an implementaion 
without using SOCK_RAW socket types (requires superuser rights). We can achieve this 
setting the extended reliable error message passing by setting `IP_RECVERR` on 
the IP layer using `setsockopt(2)`. This uses the POSIX.1-2001 conforming socket 
API. 

## Image Preview
![](/preview.png)
