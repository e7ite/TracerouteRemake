# TracerouteRemake

This program was made to recreate `traceroute(1)` with an implementaion 
without using SOCK_RAW socket types (requires superuser rights), but by setting 
the extended reliable error message passing setting `IP_RECVERR` on the IP layer using  
`setsockopt(2)`. This uses the POSIX.1-2001 conforming socket API functions. 

## Image Preview
![](/preview.png)

## Issues
This currently does not exactly replicate `traceroute(1)` because it gets different 
results usually after hops > 10.
