/*
 * session.hpp
 *
 */

#ifndef GSCALE_TCP_SESSION_HPP_
#define GSCALE_TCP_SESSION_HPP_

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/function.hpp>

#include <algorithm>

#include "packet.hpp"
#include "protocol.hpp"

namespace GScale{

namespace Backend{

class TCP;

class TCP_Session
  : public boost::enable_shared_from_this<TCP_Session>
{
    public:
        TCP_Session(TCP *backend, boost::asio::io_service& io_service);
        template <class T> void on_close(T f) {
            this->onclose = f;
        }

        boost::asio::ip::tcp::socket& socket();
        boost::asio::ip::tcp::endpoint remote_endpoint() const;
        boost::asio::ip::tcp::endpoint local_endpoint() const;

        void syncNodeList();

        void start();

        void onReadError(const boost::system::error_code& error);
        void onSendError(const boost::system::error_code& error);
        void onRead(Packet &p);
        void sendFinished(Packet &p);

        void close();

    public:
        boost::uuids::uuid remotehostuuid;

    private:
        boost::asio::io_service &io_service;
        boost::asio::ip::tcp::socket socket_;
        char readbuffer[64*1024];  // 64KB buffer
        char writebuffer[64*1024];  // 64KB buffer

        boost::function<void()> onclose;
        TCP *backend;

        enum STATES { MIN=0, SYNC, AVAIL, MAX} state;
        boost::posix_time::ptime nodesyncctime;

        boost::shared_ptr<TCP_Protocol<TCP_Session>  > proto;
};

typedef boost::shared_ptr<TCP_Session> TCP_Session_ptr;

}

}

#endif /* TCP_SESSION_HPP_ */
