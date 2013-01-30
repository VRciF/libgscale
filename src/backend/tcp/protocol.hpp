/*
 * protocol.hpp
 *
 *  Created on: Jan 25, 2013
 *      Author: tux
 */

#ifndef PROTOCOL_HPP_
#define PROTOCOL_HPP_

#include <boost/asio.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>

#include "exception.hpp"

namespace GScale{

namespace Backend{

template<class Subscriber>
class TCP_Protocol{
    public:
        TCP_Protocol(boost::asio::ip::tcp::socket &socket_, Subscriber &al, uint32_t buffsize=8192) : sub(sub),socket_(socket_) {
            if(buffsize<64){
                throw GScale::Exception("invalid buffer size given, must be greater or equal to 128", EINVAL, __FILE__, __LINE__);
            }
            this->BUFFSIZE = buffsize;

            char *inbuffer = new char[buffsize];
            if(inbuffer==NULL){
                throw GScale::Exception("not enough memory for output buffer allocation", ENOMEM, __FILE__, __LINE__);
            }
            char *outbuffer = new char[buffsize];
            if(outbuffer==NULL){
                delete[] inbuffer;
                throw GScale::Exception("not enough memory for output buffer allocation", ENOMEM, __FILE__, __LINE__);
            }

            this->outstate = this->instate = FREE;

            boost::system::error_code ec;
            this->onRead(ec, 0);
        }
        ~TCP_Protocol(){
            delete[] this->inbuffer;
            delete[] this->outbuffer;
        }

        void onRead(const boost::system::error_code& error, std::size_t bytes_transferred){
            if(error){
                this->sub.onReadError(error);
            }else{
                // process read buffer
                this->processReadBuffer(bytes_transferred);
            }

            if(this->socket_.is_open()){
                this->socket_.async_read_some(boost::asio::buffer(this->inbuffer+this->readoffset, this->BUFFSIZE-this->readoffset),
                    boost::bind(&TCP_Protocol<Subscriber>::onRead, this, boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
            }
        }

        template<typename Packet>
        void onSend(Packet &p, const boost::system::error_code& error, std::size_t bytes_transferred){
            if(error){
                this->sub.onSendError(p, error);
            }
            else{
                this->alreadysent += bytes_transferred; // excludes header size
                switch(this->outstate){
                    case FREE:
                        this->writeoffset = 0;
                        this->alreadysent = 0;
                        this->outstate = HEADER;
                    case HEADER:
                        boost::system::error_code ec;
                        this->writeHeader(p, ec);
                        if(ec){
                            this->outstate = FREE;
                            this->writeoffset = 0;
                            this->sub.onSendError(p, ec);

                            break;
                        }
                        this->alreadysent = 0;
                        this->outstate = PAYLOAD;
                    case PAYLOAD:
                        uint32_t todo = p.size() - this->alreadysent;
                        uint32_t fillsize = this->BUFFSIZE-this->writeoffset;

                        if(fillsize > todo){
                            fillsize = todo;
                        }

                        if(todo>0 && fillsize>0){
                            memcpy(this->outbuffer+this->writeoffset, p.payload().data()+this->alreadysent, fillsize);
                            this->writeoffset += fillsize;
                            this->alreadysent += fillsize;
                        }
                        else if(this->writeoffset<0){
                            // everything sent
                            this->outstate = FREE;
                            this->sub.sendFinished(p);
                            return;
                        }
                        break;
                }
            }

            if(this->socket_.is_open() && this->writeoffset>0){
                // async_write calls onSend only if an error occured OR all data has been written
                boost::asio::async_write(this->socket_, boost::asio::buffer(this->outbuffer, this->writeoffset),
                                         boost::bind(&TCP_Protocol<Subscriber>::onSend, this, p, boost::asio::placeholders::error,
                                                     boost::asio::placeholders::bytes_transferred));
                this->writeoffset = 0;
            }
        }

        template<typename Packet>
        void send(Packet &p){
            if(outstate != FREE){
                throw GScale::Exception("socket is in transmission state", EFAULT, __FILE__, __LINE__);
            }
            boost::system::error_code ec;
            this->onSend(p, ec, 0);
        }

    protected:
        template<typename Packet>
        void writeHeader(Packet &p, boost::system::error_code &error){
            uint32_t freespace = this->BUFFSIZE-this->writeoffset;
            uint32_t oldoffset = this->writeoffset;

            error = boost::system::error_code(boost::system::errc::success, boost::system::system_category());
            uint32_t size = 0;
            std::stringstream ss;
            boost::archive::binary_oarchive oa(ss);
            // write class instance to archive
            oa << p;
            std::string packetheader = ss.str();

            if(freespace<5+packetheader.length() || (p.size()>0 && freespace<9+packetheader.length())){ // be sure that the header can be written
                error = boost::system::error_code(boost::system::errc::value_too_large, boost::system::system_category());
                return;
            }

            // has the packet a payload?
            this->outbuffer[this->writeoffset] = p.size>0 ? 1 : 0; // 1 byte for payload check
            this->writeoffset++;

            // save header size
            size = htonl(packetheader.length());
            memcpy(this->outbuffer+this->writeoffset, &size, 4);  // 4 bytes is the header size
            this->writeoffset += 4;

            // save payload size
            if(p.size()>0){
                size = htonl(p.size());
                memcpy(this->outbuffer+this->writeoffset, &size, 4);  // 4 bytes is the payload size if paylod is given
                this->writeoffset += 4;
            }

            if(packetheader.length()>=this->BUFFSIZE-this->writeoffset){
                this->writeoffset = oldoffset;
                error = boost::system::error_code(boost::system::errc::value_too_large, boost::system::system_category());
                return;
            }
            // n bytes is the packet header containing, type, sourceuuid, destuuid, ...
            memcpy(this->outbuffer+this->writeoffset, packetheader.data(), packetheader.length());
            this->writeoffset += packetheader.length();
        }

        void processReadBuffer(std::size_t bytes_transferred){
            if(bytes_transferred>0){
                this->readoffset += bytes_transferred;
            }

            while(bytes_transferred>0){
                switch(this->instate){
                    case MIN: case MAX:
                        break;
                    case FREE: // this means, we're expecting a header at first
                        this->instate = HEADER;
                        this->alreadyread = 0;
                        this->readoffset = 0;
                    case HEADER:
                    {
                        if(this->readoffset < 5){
                            bytes_transferred = 0;
                            break;
                        }
                        uint32_t headersize = 0;
                        memcpy(&headersize, &this->inbuffer[1], 4);
                        headersize = ntohl(headersize);

                        this->payloadsize = 0;
                        memcpy(&headersize, &this->inbuffer[5], 4);
                        this->payloadsize = ntohl(this->payloadsize);

                        // here we know, that inbuffer contains the whole header of a newly arrived packet
                        // so do unserialization using binary_iarchive
                        if(this->inbuffer[0] && this->readoffset >= (1+4+4+headersize)){
                            std::string header(this->inbuffer+9, headersize);
                            std::stringstream ss;
                            ss << header;
                            boost::archive::binary_iarchive ia(ss);
                            // write class instance to archive
                            ia >> this->input;

                            memmove(this->inbuffer, this->inbuffer+9+headersize, this->BUFFSIZE-9-headersize);
                            this->readoffset = 0;
                            this->instate = PAYLOAD;
                            bytes_transferred -= 9+headersize;
                        }else if(!this->inbuffer[0] && this->readoffset >= 1+4+headersize){
                            std::string header(this->inbuffer+5, headersize);
                            std::stringstream ss;
                            ss << header;
                            boost::archive::binary_iarchive ia(ss);
                            // write class instance to archive
                            ia >> this->input;

                            memmove(this->inbuffer, this->inbuffer+5+headersize, this->BUFFSIZE-5-headersize);
                            this->readoffset = 0;
                            this->instate = FREE;
                            bytes_transferred -= 5+headersize;

                            this->sub.onRead(this->input);
                            break;
                        }else{
                            bytes_transferred = 0;
                            break;
                        }
                    }
                    case PAYLOAD:
                    {
                        uint32_t todo = this->payloadsize;
                        uint32_t plength = this->readoffset;
                        if(todo <= plength){
                            plength = todo;
                        }
                        std::string payload(this->inbuffer, plength);
                        this->input.payload(payload);
                        this->sub.onRead(this->input);

                        memmove(this->inbuffer, this->inbuffer+plength, this->BUFFSIZE-plength);
                        this->readoffset = 0;
                        bytes_transferred -= plength;
                        this->payloadsize -= plength;

                        if(payloadsize <= 0){
                            this->instate = FREE;
                        }
                    }
                    break;
                }
            }
        }

        Subscriber &sub;
        boost::asio::ip::tcp::socket &socket_;

        int BUFFSIZE;

        enum TRANSFER { MIN=0, FREE, HEADER, PAYLOAD, MAX} instate, outstate;

        uint32_t alreadysent;
        uint32_t alreadyread;

        Packet input;
        char *inbuffer;
        uint32_t payloadsize;
        uint32_t readoffset;

        char *outbuffer;
        uint32_t writeoffset;
};

}

}

#endif /* PROTOCOL_HPP_ */
