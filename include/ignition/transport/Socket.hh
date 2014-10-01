/*
 * Copyright (C) 2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

/* Reference:
http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/practical/PracticalSocket.h
*/

#ifndef __IGN_TRANSPORT_SOCKET_HH_INCLUDED__
#define __IGN_TRANSPORT_SOCKET_HH_INCLUDED__

#include <exception>
#include <string>

namespace ignition
{
  namespace transport
  {
    /// \class SocketException Socket.hh
    /// ignition/transport/Socket.hh
    /// \brief Exception class for sockets.
    class SocketException : public std::exception
    {
      /// \brief Class constructor.
      /// \param _message Exception message.
      /// \param _inclSysMsg True if system message (from strerror(errno))
      /// should be postfixed to the user provided message.
      public: SocketException(const std::string &_message,
                               bool _inclSysMsg = false);

      /// \brief Class destructor.
      public: ~SocketException() = default;

      /// \brief Get the exception message.
      /// \return Exception message.
      public: const char* what() const noexcept;

      /// \brief Exception message.
      private: std::string userMessage;
    };

    /// \class Socket Socket.hh
    /// ignition/transport/Socket.hh
    /// \brief Socket base class.
    class Socket
    {
      /// \brief Class constructor.
      /// \param _type Socket type. See 'man socket' for all the types.
      /// \param _protocol Socket protocol (IPPROTO_UDP, IPPROTO_TCP).
      public: Socket(int _type,
                     int _protocol) throw(SocketException);

      /// \brief Class constructor.
      /// \param _sockDesc Socket descriptor.
      public: Socket(int _sockDesc);

      /// \brief Prevent the user from trying to use the copy constructor.
      public: Socket(const Socket &_sock) = delete;

      /// \brief Class destructor.
      public: ~Socket();

      /// \brief Set the local port to the specified port and the local address
      /// to any interface.
      /// \param _localPort local port.
      /// \exception SocketException thrown if setting local port fails.
      public: void SetLocalPort(uint16_t _localPort)
                                throw(SocketException);

      /// \brief Set the local port to the specified port and the local address
      /// to the specified address.  If you omit the port, a random port
      /// will be selected.
      /// \param _localAddress local address.
      /// \param _localPort local port.
      /// \exception SocketException thrown if setting local port/address fails.
      public: void SetLocalAddressAndPort(const std::string &_localAddress,
                                          uint16_t _localPort = 0)
                                          throw(SocketException);

      /// \brief Socket descriptor.
      public: int sockDesc;
    };

    /// \class CommunicatingSocket Socket.hh
    /// ignition/transport/Socket.hh
    /// \brief Socket which is able to connect, send, and receive.
    class CommunicatingSocket : public Socket
    {
      /// \brief Class constructor. Documentation inherited.
      public: CommunicatingSocket(int _type,
                                  int _protocol) throw(SocketException);

      /// \brief Class constructor. Documentation inherited.
      public: CommunicatingSocket(int _newConnSD);

      /// \brief Establish a socket connection with the given foreign
      /// address and port.
      /// \param _foreignAddress Foreign address (IP address or name).
      /// \param _foreignPort Foreign port.
      /// \exception SocketException thrown if unable to establish connection.
      public: void Connect(const std::string &_foreignAddress,
                           uint16_t _foreignPort) throw(SocketException);

      /// \brief Write the given buffer to this socket. Call connect() before
      /// calling send().
      /// \param _buffer buffer to be written.
      /// \param _bufferLen number of bytes from buffer to be written.
      /// \exception SocketException thrown if unable to send data.
      public: void Send(const void *_buffer,
                        int _bufferLen) throw(SocketException);

      /// \brief Read into the given buffer up to bufferLen bytes data from this
      /// socket. Call connect() before calling recv().
      /// \param _buffer buffer to receive the data.
      /// \param _bufferLen maximum number of bytes to read into buffer.
      /// \return number of bytes read, 0 for EOF, and -1 for error.
      /// \exception SocketException thrown if unable to receive data.
      public: int Recv(void *_buffer,
                       int _bufferLen) throw(SocketException);
    };

    /// \class CommunicatingSocket Socket.hh
    /// ignition/transport/Socket.hh
    /// \brief UDP Socket class.
    class UDPSocket : public CommunicatingSocket
    {
      /// \brief class constructor.
      /// \exception SocketException thrown if unable to create UDP socket.
      public: UDPSocket() throw(SocketException);

      /// \brief Construct a UDP socket with the given local port.
      /// \param _localPort Local port.
      /// \exception SocketException thrown if unable to create UDP socket.
      public: UDPSocket(uint16_t _localPort) throw(SocketException);

      /// \brief Construct a UDP socket with the given local port and address
      /// \param _localAddress local address.
      /// \param _localPort local port.
      /// \exception SocketException thrown if unable to create UDP socket.
      public: UDPSocket(const std::string &_localAddress,
                        uint16_t _localPort) throw(SocketException);

      /// \brief Unset foreign address and port.
      /// \return true if disassociation is successful.
      /// \exception SocketException thrown if unable to disconnect UDP socket.
      public: void Disconnect() throw(SocketException);

      /// \brief Send the given buffer as a UDP datagram to the
      /// specified address/port.
      /// \param _buffer buffer to be written
      /// \param _bufferLen number of bytes to write
      /// \param _foreignAddress address (IP address or name) to send to
      /// \param _foreignPort port number to send to
      /// \return true if send is successful
      /// \exception SocketException thrown if unable to send datagram
      public: void SendTo(const void *_buffer,
                          int _bufferLen,
                          const std::string &_foreignAddress,
                          uint16_t _foreignPort) throw(SocketException);

      /// \brief Read read up to bufferLen bytes data from this socket.
      /// The given buffer is where the data will be placed.
      /// \param _buffer buffer to receive data.
      /// \param _bufferLen maximum number of bytes to receive.
      /// \param _sourceAddress address of datagram source.
      /// \param _sourcePort port of data source.
      /// \return number of bytes received and -1 for error.
      /// \exception SocketException thrown if unable to receive datagram.
      public: int RecvFrom(void *_buffer,
                           int _bufferLen,
                           std::string &_sourceAddress,
                           uint16_t &_sourcePort) throw(SocketException);

      /// \brief Enable broadcast for this socket.
      private: void SetBroadcast();
    };
  }
}
#endif
