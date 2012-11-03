#ifndef BERKLEYSOCKETS_NETSPEED_H_
#define BERKLEYSOCKETS_NETSPEED_H_

/*
 * code is from git://git.kernel.org/pub/scm/linux/kernel/git/romieu/ethtool.git
 * many thanks to the authors of ethtool
 */

#include <stdint.h>

#ifndef SIOCETHTOOL
#define SIOCETHTOOL     0x8946
#endif

#ifndef ETHTOOL_GSET
#define ETHTOOL_GSET            0x00000001 /* Get settings. */
#endif

struct GScale_Backend_BerkleySockets_Netspeed_Settings{
    uint16_t rtt; /* round time trip in milliseconds - e.g. the time for a ping or socket connect() */
};

struct gscale_ethtool_cmd {
		uint32_t   cmd;
		uint32_t   supported;      /* Features this interface supports */
		uint32_t   advertising;    /* Features this interface advertises */
		uint16_t   speed;          /* The forced speed, 10Mb, 100Mb, gigabit */

		uint8_t    duplex;         /* Duplex, half or full */
		uint8_t    port;           /* Which connector port */
		uint8_t    phy_address;
		uint8_t    transceiver;    /* Which transceiver to use */
		uint8_t    autoneg;        /* Enable or disable autonegotiation */

		uint32_t   maxtxpkt;       /* Tx pkts before generating tx int */
		uint32_t   maxrxpkt;       /* Rx pkts before generating rx int */
		uint16_t   speed_hi;
		uint16_t   reserved2;
		uint32_t   reserved[3];
};

uint32_t GScale_Backend_BerkleySockets_Netspeed_calculateSpeed(struct gscale_ethtool_cmd *ep);

/* returns device speed in Mb/s (megabit per second) */
uint32_t GScale_Backend_BerkleySockets_Netspeed_getDeviceSpeed(char *devname, int fd);
uint32_t GScale_Backend_BerkleySockets_Netspeed_getMaxNetSpeed();
/*
 * uses installed network devices (eth0, ...) to get
 * the maximum useful socket buffer size
 * and returns it in bytes
 */
uint32_t GScale_Backend_BerkleySockets_Netspeed_getMaxBufferSize(uint32_t defaultsize);

#endif
