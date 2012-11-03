#ifndef BERKLEYSOCKETS_NETSPEED_C_
#define BERKLEYSOCKETS_NETSPEED_C_

/*getifaddrs*/

/*
 * code is from git://git.kernel.org/pub/scm/linux/kernel/git/romieu/ethtool.git
 * many thanks to the authors of ethtool
 */

#include <stdint.h>
#include <string.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <linux/sockios.h>

#include "backend/berkleysockets/netspeed.h"

struct GScale_Backend_BerkleySockets_Netspeed_Settings gscale_netspeed_config = {100};

uint32_t
GScale_Backend_BerkleySockets_Netspeed_calculateSpeed(struct gscale_ethtool_cmd *ep)
{
        return (ep->speed_hi << 16) | ep->speed;
}

/* returns device speed in Mb/s (megabit per second) */
uint32_t
GScale_Backend_BerkleySockets_Netspeed_getDeviceSpeed(char *devname, int fd)
{
        struct ifreq ifr;
        /* This should work for both 32 and 64 bit userland. */
        struct gscale_ethtool_cmd ecmd = { 0, 0, 0, 0,
                                           0, 0, 0, 0, 0,
                                           0, 0, 0, 0, {0, 0, 0}
                                         };
        uint32_t speed = 0;

        ecmd.supported = 0;

        /* Setup our control structures. */
        memset(&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, devname);

        ecmd.cmd = ETHTOOL_GSET;
        ifr.ifr_data = (caddr_t)&ecmd;
        if (ioctl(fd, SIOCETHTOOL, &ifr) == 0) {
           speed = GScale_Backend_BerkleySockets_Netspeed_calculateSpeed(&ecmd);
           if (speed == 0 || speed == (uint16_t)(-1) || speed == (uint32_t)(-1)) speed = 0;
        }
        return speed;
}

uint32_t
GScale_Backend_BerkleySockets_Netspeed_getMaxNetSpeed(uint32_t defaultsize)
{

    int fd, err;
    static struct ifreq ifreqs[20];
    struct ifconf ifconf;
    int  nifaces, i;
    uint32_t speed;
    uint32_t finspeed = 1;
    uint32_t tmpspeed = 0;
    if(defaultsize==0){
        finspeed = defaultsize/(1024*1024);
    }

    /* Open control socket. */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return finspeed;
    }

    memset(&ifconf,0,sizeof(ifconf));
    ifconf.ifc_buf = (char*) (ifreqs);
    ifconf.ifc_len = sizeof(ifreqs);

    if((err = ioctl(fd, SIOCGIFCONF , &ifconf  )) < 0 ){
       return finspeed;
    }

    /* number of interfaces found */
    nifaces =  ifconf.ifc_len/sizeof(struct ifreq);
    for(i = 0; i < nifaces; i++)
    {
        speed = GScale_Backend_BerkleySockets_Netspeed_getDeviceSpeed(ifreqs[i].ifr_name, fd);
        /* 1 terabit (1024*1024) speed is our max
         * this line seems weird, but the interface 'lo'
         * returned a speed of 3220382600Mb/s (~384 terabyte per second)
         * thats why i set a maximum limit as much lower than lo but higher
         * than current possible hardware technology
         */
        if(speed>1024*1024){ continue; }  /* ignore 'lo' */

        tmpspeed = (speed * (gscale_netspeed_config.rtt/1000.0) /8.0);
        if(tmpspeed>finspeed){ finspeed = tmpspeed; }
    }
    return finspeed;
}

/*
 * uses installed network devices (eth0, ...) to get
 * the maximum useful socket buffer size
 * and returns it in bytes
 */
uint32_t
GScale_Backend_BerkleySockets_Netspeed_getMaxBufferSize(uint32_t defaultsize)
{
    /* 1024*1024 for speed in bytes cause calculation is in megabyte */
    return (GScale_Backend_BerkleySockets_Netspeed_getMaxNetSpeed(defaultsize) * (gscale_netspeed_config.rtt/1000.0) /8.0) * 1024*1024;
}

#endif
