/*
 *   C++ sockets on Unix and Windows
 *   Copyright (C) 2002
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
  // For socket(), connect(), send(), and recv().
  #include <winsock.h>
  typedef int socklen_t;
  // Type used for raw data on this platform.
  typedef char raw_type;
#else
  // For data types
  #include <sys/types.h>
  // For socket(), connect(), send(), and recv()
  #include <sys/socket.h>
  // For gethostbyname()
  #include <netdb.h>
  // For inet_addr()
  #include <arpa/inet.h>
  // For close()
  #include <unistd.h>
  // For sockaddr_in
  #include <netinet/in.h>
  // Type used for raw data on this platform
  typedef void raw_type;
#endif

// For errno
#include <errno.h>
#include "ignition/transport/Socket.hh"

#ifdef WIN32
static bool initialized = false;
#endif

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
SocketException::SocketException(const std::string &_message, bool _inclSysMsg)
  : userMessage(_message)
{
  if (_inclSysMsg)
  {
    this->userMessage.append(": ");
    this->userMessage.append(strerror(errno));
  }
}

//////////////////////////////////////////////////
const char* SocketException::what() const noexcept
{
  return this->userMessage.c_str();
}

//////////////////////////////////////////////////
/// \brief Function to fill in address structure given an address and port.
static void fillAddr(const std::string &_address, uint16_t _port,
                     sockaddr_in &_addr)
{
  // Zero out address structure.
  memset(&_addr, 0, sizeof(_addr));
  // Internet address.
  _addr.sin_family = AF_INET;

  // Resolve name.
  hostent *host;
  if ((host = gethostbyname(_address.c_str())) == NULL)
  {
    // strerror() will not work for gethostbyname() and hstrerror()
    // is supposedly obsolete.
    throw SocketException("Failed to resolve name (gethostbyname())");
  }
  _addr.sin_addr.s_addr = *(reinterpret_cast<uint64_t *>(host->h_addr_list[0]));

  // Assign port in network byte order.
  _addr.sin_port = htons(_port);
}

//////////////////////////////////////////////////
Socket::Socket(int _type, int _protocol) throw(SocketException)
{
  #ifdef WIN32
    if (!initialized)
    {
      WORD wVersionRequested;
      WSADATA wsaData;

      // Request WinSock v2.0.
      wVersionRequested = MAKEWORD(2, 0);
      // Load WinSock DLL.
      if (WSAStartup(wVersionRequested, &wsaData) != 0)
        throw SocketException("Unable to load WinSock DLL");

      initialized = true;
    }
  #endif

  // Make a new socket.
  if ((sockDesc = socket(PF_INET, _type, _protocol)) < 0)
    throw SocketException("Socket creation failed (socket())", true);
}

//////////////////////////////////////////////////
Socket::Socket(int _sockDesc)
{
  this->sockDesc = _sockDesc;
}

//////////////////////////////////////////////////
Socket::~Socket()
{
  #ifdef WIN32
    ::closesocket(this->sockDesc);
  #else
    ::close(this->sockDesc);
  #endif
  this->sockDesc = -1;
}

//////////////////////////////////////////////////
void Socket::SetLocalPort(uint16_t _localPort) throw(SocketException)
{
  // Bind the socket to its port.
  sockaddr_in localAddr;
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  // localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  localAddr.sin_port = htons(_localPort);

  if (bind(this->sockDesc, reinterpret_cast<sockaddr *>(&localAddr),
        sizeof(sockaddr_in)) < 0)
  {
    throw SocketException("Set of local port failed (bind())", true);
  }
}

//////////////////////////////////////////////////
void Socket::SetLocalAddressAndPort(const std::string &_localAddress,
                                    uint16_t _localPort)
                                    throw(SocketException)
{
  // Get the address of the requested host.
  sockaddr_in localAddr;
  fillAddr(_localAddress, _localPort, localAddr);

  if (bind(this->sockDesc, reinterpret_cast<sockaddr *>(&localAddr),
        sizeof(sockaddr_in)) < 0)
  {
    throw SocketException("Set local address and port failed (bind())", true);
  }
}

//////////////////////////////////////////////////
CommunicatingSocket::CommunicatingSocket(int _type, int _protocol)
  throw(SocketException)
  : Socket(_type, _protocol)
{
}

//////////////////////////////////////////////////
CommunicatingSocket::CommunicatingSocket(int _newConnSD)
  : Socket(_newConnSD)
{
}

//////////////////////////////////////////////////
void CommunicatingSocket::Connect(const std::string &_foreignAddress,
  uint16_t _foreignPort) throw(SocketException)
{
  // Get the address of the requested host
  sockaddr_in destAddr;
  fillAddr(_foreignAddress, _foreignPort, destAddr);

  // Try to connect to the given port
  if (::connect(this->sockDesc, reinterpret_cast<sockaddr *>(&destAddr),
          sizeof(destAddr)) < 0)
  {
    throw SocketException("Connect failed (connect())", true);
  }
}

//////////////////////////////////////////////////
void CommunicatingSocket::Send(const void *_buffer, int _bufferLen)
  throw(SocketException)
{
  if (::send(this->sockDesc, reinterpret_cast<const raw_type *>(_buffer),
          _bufferLen, 0) < 0)
  {
    throw SocketException("Send failed (send())", true);
  }
}

//////////////////////////////////////////////////
int CommunicatingSocket::Recv(void *_buffer, int _bufferLen)
  throw(SocketException)
{
  int rtn;
  if ((rtn = ::recv(this->sockDesc, reinterpret_cast<sockaddr *>(_buffer),
         _bufferLen, 0)) < 0)
  {
    throw SocketException("Received failed (recv())", true);
  }

  return rtn;
}

//////////////////////////////////////////////////
UDPSocket::UDPSocket() throw(SocketException)
  : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP)
{
  this->SetBroadcast();
}

//////////////////////////////////////////////////
UDPSocket::UDPSocket(uint16_t _localPort)  throw(SocketException)
  : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP)
{
  this->SetBroadcast();
  this->SetLocalPort(_localPort);
}

//////////////////////////////////////////////////
UDPSocket::UDPSocket(const std::string &_localAddress, uint16_t _localPort)
  throw(SocketException) : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP)
{
  this->SetBroadcast();
  this->SetLocalAddressAndPort(_localAddress, _localPort);
}

//////////////////////////////////////////////////
void UDPSocket::SetBroadcast()
{
  // If this fails, we'll hear about it when we try to send.  This will allow
  // system that cannot broadcast to continue if they don't plan to broadcast.
  int broadcastPermission = 1;
  setsockopt(this->sockDesc, SOL_SOCKET, SO_BROADCAST,
    reinterpret_cast<sockaddr *>(&broadcastPermission),
    sizeof(broadcastPermission));
  int reuseAddr = 1;
  setsockopt(this->sockDesc, SOL_SOCKET, SO_REUSEADDR,
    reinterpret_cast<sockaddr *>(&reuseAddr), sizeof(reuseAddr));
}

//////////////////////////////////////////////////
void UDPSocket::Disconnect() throw(SocketException)
{
  sockaddr_in nullAddr;
  memset(&nullAddr, 0, sizeof(nullAddr));
  nullAddr.sin_family = AF_UNSPEC;

  // Try to disconnect
  if (::connect(this->sockDesc, reinterpret_cast<sockaddr *>(&nullAddr),
          sizeof(nullAddr)) < 0)
  {
#ifdef WIN32
    if (errno != WSAEAFNOSUPPORT)
#else
    if (errno != EAFNOSUPPORT)
#endif
    {
      throw SocketException("Disconnect failed (connect())", true);
    }
  }
}

//////////////////////////////////////////////////
void UDPSocket::SendTo(const void *_buffer, int _bufferLen,
    const std::string &_foreignAddress, uint16_t _foreignPort)
    throw(SocketException)
{
  sockaddr_in destAddr;
  fillAddr(_foreignAddress, _foreignPort, destAddr);

  // Write out the whole buffer as a single message.
  if (sendto(this->sockDesc, reinterpret_cast<const raw_type *>(_buffer),
    _bufferLen, 0, reinterpret_cast<sockaddr *>(&destAddr),
    sizeof(destAddr)) != _bufferLen)
  {
    throw SocketException("Send failed (sendto())", true);
  }
}

//////////////////////////////////////////////////
int UDPSocket::RecvFrom(void *_buffer, int _bufferLen,
  std::string &_sourceAddress, uint16_t &_sourcePort)
  throw(SocketException)
{
  sockaddr_in clntAddr;
  socklen_t addrLen = sizeof(clntAddr);
  int rtn;
  if ((rtn = recvfrom(this->sockDesc, reinterpret_cast<raw_type *>(_buffer),
    _bufferLen, 0, reinterpret_cast<sockaddr *>(&clntAddr),
     reinterpret_cast<socklen_t *>(&addrLen))) < 0)
  {
    throw SocketException("Receive failed (recvfrom())", true);
  }
  _sourceAddress = inet_ntoa(clntAddr.sin_addr);
  _sourcePort = ntohs(clntAddr.sin_port);

  return rtn;
}
