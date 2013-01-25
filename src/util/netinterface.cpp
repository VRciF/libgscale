/*
 * netinterface.cpp
 *
 */

#include "netinterface.hpp"
#include "exception.hpp"

#include <boost/config.hpp>
#include <stdexcept>


#if defined(BOOST_WINDOWS)

#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>

// Link with Iphlpapi.lib
#pragma comment(lib, "IPHLPAPI.lib")

#else

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#if __MACH__ || __NetBSD__ || __OpenBSD__ || __FreeBSD__
#include <sys/sysctl.h>
#endif
/* Include sockio.h if needed */
#ifndef SIOCGIFCONF
#include <sys/sockio.h>
#endif
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#if __MACH__
#include <net/if_dl.h>
#endif

/* On platforms that have variable length
   ifreq use the old fixed length interface instead */
#ifdef OSIOCGIFCONF
#undef SIOCGIFCONF
#define SIOCGIFCONF OSIOCGIFCONF
#undef SIOCGIFADDR
#define SIOCGIFADDR OSIOCGIFADDR
#undef SIOCGIFBRDADDR
#define SIOCGIFBRDADDR OSIOCGIFBRDADDR
#endif

#endif

namespace GScale{

std::vector<NetInterface> NetInterface::all(){
    std::vector<NetInterface> result;

#if defined(BOOST_WINDOWS)
    /* Declare and initialize variables */

    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    unsigned int i = 0;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_ALL_INTERFACES |
                  GAA_FLAG_INCLUDE_PREFIX |
                  GAA_FLAG_SKIP_DNS_SERVER |
                  GAA_FLAG_SKIP_MULTICAST;

    // default to unspecified address family (both)

    LPVOID lpMsgBuf = NULL;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
    PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
    IP_ADAPTER_PREFIX *pPrefix = NULL;

    // Allocate a 15 KB buffer to start with.
    outBufLen = 15000;

    do {
        outBufLen *= 2;
        pAddresses = (IP_ADAPTER_ADDRESSES *) malloc(outBufLen);
        if (pAddresses == NULL) {
            throw GScale::Exception(NULL, 0, __FILE__, __LINE__);
        }

        dwRetVal =
            GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            free(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (outBufLen < 1048576));

    if (dwRetVal != NO_ERROR) {
        if (pAddresses)
            free(pAddresses);

        GScale::Exception except(NULL, dwRetVal, __FILE__, __LINE__);

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                // Default language
                (LPTSTR) & lpMsgBuf, 0, NULL)) {

            except << lpMsgBuf;

            LocalFree(lpMsgBuf);

        }

        throw except;
    }

    // iterate over the list and add the entries to our listing
    for (PIP_ADAPTER_ADDRESSES ptr = pAddresses; ptr; ptr = ptr->Next) {
        std::string ifname = ptr->AdapterName;

        QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
        interfaces << iface;

        iface->index = 0;
        if (ptr->Length >= offsetof(IP_ADAPTER_ADDRESSES, Ipv6IfIndex) && ptr->Ipv6IfIndex != 0)
            iface->index = ptr->Ipv6IfIndex;
        else if (ptr->IfIndex != 0)
            iface->index = ptr->IfIndex;

        iface->flags = QNetworkInterface::CanBroadcast;
        if (ptr->OperStatus == IfOperStatusUp)
            iface->flags |= QNetworkInterface::IsUp | QNetworkInterface::IsRunning;
        if ((ptr->Flags & IP_ADAPTER_NO_MULTICAST) == 0)
            iface->flags |= QNetworkInterface::CanMulticast;

        iface->name = QString::fromLocal8Bit(ptr->AdapterName);
        iface->friendlyName = QString::fromWCharArray(ptr->FriendlyName);
        if (ptr->PhysicalAddressLength)
            iface->hardwareAddress = iface->makeHwAddress(ptr->PhysicalAddressLength,
                                                          ptr->PhysicalAddress);
        else
            // loopback if it has no address
            iface->flags |= QNetworkInterface::IsLoopBack;

        // The GetAdaptersAddresses call has an interesting semantic:
        // It can return a number N of addresses and a number M of prefixes.
        // But if you have IPv6 addresses, generally N > M.
        // I cannot find a way to relate the Address to the Prefix, aside from stopping
        // the iteration at the last Prefix entry and assume that it applies to all addresses
        // from that point on.
        PIP_ADAPTER_PREFIX pprefix = 0;
        if (ptr->Length >= offsetof(IP_ADAPTER_ADDRESSES, FirstPrefix))
            pprefix = ptr->FirstPrefix;
        for (PIP_ADAPTER_UNICAST_ADDRESS addr = ptr->FirstUnicastAddress; addr; addr = addr->Next) {
            QNetworkAddressEntry entry;
            entry.setIp(addressFromSockaddr(addr->Address.lpSockaddr));
            if (pprefix) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    entry.setNetmask(ipv4netmasks[entry.ip()]);

                    // broadcast address is set on postProcess()
                } else { //IPV6
                    entry.setPrefixLength(pprefix->PrefixLength);
                }
                pprefix = pprefix->Next ? pprefix->Next : pprefix;
            }
            iface->addressEntries << entry;
        }
    }

    // If successful, output some information from the data we received
    pCurrAddresses = pAddresses;
    while (pCurrAddresses) {
        std::string ifname(pCurrAddresses->AdapterName);
        printf("\tLength of the IP_ADAPTER_ADDRESS struct: %ld\n",
               pCurrAddresses->Length);
        printf("\tIfIndex (IPv4 interface): %u\n", pCurrAddresses->IfIndex);
        printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);

        pUnicast = pCurrAddresses->FirstUnicastAddress;
        if (pUnicast != NULL) {
            for (i = 0; pUnicast != NULL; i++)
                pUnicast = pUnicast->Next;
            printf("\tNumber of Unicast Addresses: %d\n", i);
        } else
            printf("\tNo Unicast Addresses\n");

        pAnycast = pCurrAddresses->FirstAnycastAddress;
        if (pAnycast) {
            for (i = 0; pAnycast != NULL; i++)
                pAnycast = pAnycast->Next;
            printf("\tNumber of Anycast Addresses: %d\n", i);
        } else
            printf("\tNo Anycast Addresses\n");

        pMulticast = pCurrAddresses->FirstMulticastAddress;
        if (pMulticast) {
            for (i = 0; pMulticast != NULL; i++)
                pMulticast = pMulticast->Next;
            printf("\tNumber of Multicast Addresses: %d\n", i);
        } else
            printf("\tNo Multicast Addresses\n");

        pDnServer = pCurrAddresses->FirstDnsServerAddress;
        if (pDnServer) {
            for (i = 0; pDnServer != NULL; i++)
                pDnServer = pDnServer->Next;
            printf("\tNumber of DNS Server Addresses: %d\n", i);
        } else
            printf("\tNo DNS Server Addresses\n");

        printf("\tDNS Suffix: %wS\n", pCurrAddresses->DnsSuffix);
        printf("\tDescription: %wS\n", pCurrAddresses->Description);
        printf("\tFriendly name: %wS\n", pCurrAddresses->FriendlyName);

        if (pCurrAddresses->PhysicalAddressLength != 0) {
            printf("\tPhysical address: ");
            for (i = 0; i < (int) pCurrAddresses->PhysicalAddressLength;
                 i++) {
                if (i == (pCurrAddresses->PhysicalAddressLength - 1))
                    printf("%.2X\n",
                           (int) pCurrAddresses->PhysicalAddress[i]);
                else
                    printf("%.2X-",
                           (int) pCurrAddresses->PhysicalAddress[i]);
            }
        }
        printf("\tFlags: %ld\n", pCurrAddresses->Flags);
        printf("\tMtu: %lu\n", pCurrAddresses->Mtu);
        printf("\tIfType: %ld\n", pCurrAddresses->IfType);
        printf("\tOperStatus: %ld\n", pCurrAddresses->OperStatus);
        printf("\tIpv6IfIndex (IPv6 interface): %u\n",
               pCurrAddresses->Ipv6IfIndex);
        printf("\tZoneIndices (hex): ");
        for (i = 0; i < 16; i++)
            printf("%lx ", pCurrAddresses->ZoneIndices[i]);
        printf("\n");

        printf("\tTransmit link speed: %I64u\n", pCurrAddresses->TransmitLinkSpeed);
        printf("\tReceive link speed: %I64u\n", pCurrAddresses->ReceiveLinkSpeed);

        pPrefix = pCurrAddresses->FirstPrefix;
        if (pPrefix) {
            for (i = 0; pPrefix != NULL; i++)
                pPrefix = pPrefix->Next;
            printf("\tNumber of IP Adapter Prefix entries: %d\n", i);
        } else
            printf("\tNumber of IP Adapter Prefix entries: 0\n");

        printf("\n");

        pCurrAddresses = pCurrAddresses->Next;
    }

    if (pAddresses) {
        free(pAddresses);
    }

#else
    std::map<std::string, std::vector<struct ifreq *> > interfaces;
    std::map<std::string, std::vector<struct ifreq *> >::iterator ifit;

    static int sck=-1;
    struct ifconf   ifc = {0};
    char            *buf = NULL;
    int             nInterfaces = 0;
    struct ifreq   *ifr = NULL;
    int             i = 0;
    std::string ifname;
    struct ifreq *item = NULL;

    /* Get a socket handle. */
    if(sck<0)
      sck = socket(PF_INET, SOCK_DGRAM, 0);
    if(sck < 0) {
        throw GScale::Exception(__FILE__,__LINE__);
    }

    /* Query available interfaces. */
    int32_t bufsize = 4069;
    int rval = 0;
    do{
        bufsize *= 2;
        if(buf!=NULL){
            free(buf);
        }
        buf = (char*)calloc(sizeof(char), bufsize);
        if(buf==NULL){
            throw GScale::Exception(__FILE__,__LINE__);
        }

        ifc.ifc_len = bufsize;
        ifc.ifc_buf = buf;
        rval=ioctl(sck, SIOCGIFCONF, &ifc);
        if(rval < 0 && errno!=EOVERFLOW){
            throw GScale::Exception(__FILE__,__LINE__);
        }
    }while(ifc.ifc_len==bufsize && bufsize<1048576);  // bufsize limit 1 megabyte

    /* Iterate through the list of interfaces. */
    ifr = ifc.ifc_req;
    nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
    for(i = 0; i < nInterfaces; i++) {
        item = &ifr[i];
        ifname = std::string(item->ifr_name, strlen(item->ifr_name));

        /* Get the address
         * This may seem silly but it seems to be needed on some systems
         */
        if(ioctl(sck, SIOCGIFADDR, item) < 0) {
            continue;
        }
#ifdef SIOCGIFHWADDR
        /* Linux */
        /* Get the MAC address */
        if(ioctl(sck, SIOCGIFHWADDR, item) < 0) {
            continue;
        }
#elif SIOCGENADDR
        /* Solaris and possibly all SysVR4 */
        /* Get the MAC address */
        if(ioctl(sck, SIOCGENADDR, item) < 0) {
            continue;
        }
#endif

        ifit = interfaces.find(ifname);
        if(ifit==interfaces.end()){
            std::pair<std::map<std::string, std::vector<struct ifreq *> >::iterator,bool> ret;

            ret = interfaces.insert(std::pair<std::string, std::vector<struct ifreq*> >(ifname, std::vector<struct ifreq*>()));
            if(ret.second == false){
                continue;
            }
            ifit = ret.first;
        }
        ifit->second.push_back(item);
    }

    for(ifit=interfaces.begin();ifit!=interfaces.end();ifit++){
        try{

            result.push_back(NetInterface(ifit->second));
        }catch(...){}
    }
    if(buf!=NULL){
        free(buf);
    }

#endif

    return result;
}

#if defined(BOOST_WINDOWS)
#else
NetInterface::NetInterface(std::vector<struct ifreq *> items){
    struct ifreq *first = NULL;

    if(items.size()<=0){
        GScale::Exception except(__FILE__,__LINE__);
        except.setCode(ENOENT);
        throw except;
    }
    first = items[0];

    this->ifmtu = first->ifr_mtu;
    this->ifname = std::string(first->ifr_name, strlen(first->ifr_name));

    boost::tuple<boost::asio::ip::address, boost::asio::ip::address, boost::asio::ip::address> addr;
    for(uint16_t i=0;i<items.size();i++){
        /* Show the device name and IP address */
        switch(items[i]->ifr_addr.sa_family) {
          case AF_INET:
              addr = boost::make_tuple(boost::asio::ip::address_v4(((struct sockaddr_in*)&(items[i]->ifr_addr))->sin_addr.s_addr),
                                       boost::asio::ip::address_v4(((struct sockaddr_in*)&(items[i]->ifr_broadaddr))->sin_addr.s_addr),
                                       boost::asio::ip::address_v4(((struct sockaddr_in*)&(items[i]->ifr_netmask))->sin_addr.s_addr));

             this->addr.push_back(addr);
             break;
          case AF_INET6:
             boost::asio::ip::address_v6::bytes_type bt_addr,bt_broad,bt_netm;

             for (int j = 0; j < 16; ++j){
                 bt_addr[j] = ((struct sockaddr_in6*)&(items[i]->ifr_addr))->sin6_addr.s6_addr[j];
                 bt_broad[j] = ((struct sockaddr_in6*)&(items[i]->ifr_broadaddr))->sin6_addr.s6_addr[j];
                 bt_netm[j] = ((struct sockaddr_in6*)&(items[i]->ifr_netmask))->sin6_addr.s6_addr[j];
             }
             addr = boost::make_tuple(boost::asio::ip::address_v6(bt_addr, ((struct sockaddr_in6*)&(items[i]->ifr_addr))->sin6_scope_id),
                                      boost::asio::ip::address_v6(),
                                      boost::asio::ip::address_v6()
                    );

             this->addr.push_back(addr);
             break;
        }
    }


    /* Lots of different ways to get the ethernet address */
#ifdef SIOCGIFHWADDR
    this->ifmac = std::string(first->ifr_hwaddr.sa_data, 6);
    /* display result */
    /*printf(" %02x:%02x:%02x:%02x:%02x:%02x",
       (unsigned char)first->ifr_hwaddr.sa_data[0],
       (unsigned char)first->ifr_hwaddr.sa_data[1],
       (unsigned char)first->ifr_hwaddr.sa_data[2],
       (unsigned char)first->ifr_hwaddr.sa_data[3],
       (unsigned char)first->ifr_hwaddr.sa_data[4],
       (unsigned char)first->ifr_hwaddr.sa_data[5]);
       */

#elif SIOCGENADDR

    this->ifmac = std::string(first->ifr_enaddr, 6);
    /* display result */
    /*printf(" %02x:%02x:%02x:%02x:%02x:%02x",
       (unsigned char)first->ifr_enaddr[0],
       (unsigned char)first->ifr_enaddr[1],
       (unsigned char)first->ifr_enaddr[2],
       (unsigned char)first->ifr_enaddr[3],
       (unsigned char)first->ifr_enaddr[4],
       (unsigned char)first->ifr_enaddr[5]);
       */

#elif __MACH__ || __NetBSD__ || __OpenBSD__ || __FreeBSD__
    /* MacOS X and all modern BSD implementations (I hope) */
    int                mib[6] = {0};
    int                len = 0;
    char               *macbuf;
    struct if_msghdr   *ifm;
    struct sockaddr_dl *sdl;
    unsigned char      ptr[];

    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_LINK;
    mib[4] = NET_RT_IFLIST;
    mib[5] = if_nametoindex(first->ifr_name);
    if(mib[5] == 0)
      continue;

    if(sysctl(mib, 6, NULL, (size_t*)&len, NULL, 0) != 0) {
      continue;
    }

    macbuf = new char[len];
    if(macbuf == NULL) {
      continue;
    }

    if(sysctl(mib, 6, macbuf, (size_t*)&len, NULL, 0) != 0) {
        delete[] macbuf;
        continue;
    }

    ifm = (struct if_msghdr *)macbuf;
    sdl = (struct sockaddr_dl *)(ifm + 1);
    ptr = (unsigned char *)LLADDR(sdl);

    this->ifmac = std::string(ptr, 6);
    /*printf(" %02x:%02x:%02x:%02x:%02x:%02x",
       ptr[0], ptr[1], ptr[2],
       ptr[3], ptr[4], ptr[5]);

    free(macbuf);
    */
    delete[] macbuf;

#else
#error OS Distribution Not Recognized
#endif
}
#endif


std::set<boost::asio::ip::address> NetInterface::address(){
    return this->getAddrByIndex<0>();
}
std::set<boost::asio::ip::address> NetInterface::broadcast(){
    return this->getAddrByIndex<1>();
}
std::set<boost::asio::ip::address> NetInterface::netmask(){
    return this->getAddrByIndex<2>();
}

std::vector<boost::tuple<boost::asio::ip::address, boost::asio::ip::address, boost::asio::ip::address> > NetInterface::addresses(){
    return this->addr;
}

int NetInterface::mtu(){
    return this->ifmtu;
}

}
