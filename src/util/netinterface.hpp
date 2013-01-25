/*
 * netinterface.hpp
 *
 * nix code from https://github.com/ajrisi/lsif/blob/master/lsif.c
 *
 * Updated code to obtain IP and MAC address for all "up" network
 * interfaces on a linux system. Now IPv6 friendly and updated to use
 * inet_ntop instead of the deprecated inet_ntoa function. This version
 * should not seg fault on newer linux systems
 *
 * Version 2.0
 *
 * Authors:
 *   Adam Pierce
 *   Adam Risi
 *   William Schaub
 *
 * Date: 11/11/2009
 * http://www.adamrisi.com
 * http://www.doctort.org/adam/
 * http://teotwawki.steubentech.com/
 *
 */

#ifndef NETINTERFACE_HPP_
#define NETINTERFACE_HPP_

#include <set>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>

namespace GScale{

class NetInterface{
    public:
        static std::vector<NetInterface> all();

        std::string name();
        std::string mac();
        std::set<boost::asio::ip::address> address();
        std::set<boost::asio::ip::address> broadcast();
        std::set<boost::asio::ip::address> netmask();

        std::vector<boost::tuple<boost::asio::ip::address, boost::asio::ip::address, boost::asio::ip::address> > addresses();
        int mtu();

    protected:
#if defined(BOOST_WINDOWS)
#else
        NetInterface(std::vector<struct ifreq *> items);
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X) || __cplusplus >= 201103L
        template<std::size_t I = 0>
#else
        template<std::size_t I>
#endif
        std::set<boost::asio::ip::address> getAddrByIndex(){
            std::set<boost::asio::ip::address> v;
            for(unsigned int j=0;j<this->addr.size();j++){
                v.insert(this->addr[j].get<I>());
            }
            return v;
        }

        std::string ifname;
        /*
         * [0] = addr
         * [1] = broadaddr
         * [2] = netmask
         */
        std::vector<boost::tuple<boost::asio::ip::address, boost::asio::ip::address, boost::asio::ip::address> > addr;

        std::string ifmac;
        int ifmtu;
};

}

#endif /* NETINTERFACE_HPP_ */
