/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#ifdef _WIN32
  // For socket(), connect(), send(), and recv().
  #include <Winsock2.h>
  #include <Ws2def.h>
  #include <Ws2ipdef.h>
  #include <Ws2tcpip.h>
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
#endif

#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

#include "Optitrack.hh"

#ifdef _WIN32
  static bool initialized = false;
#endif

/////////////////////////////////////////////////
Optitrack::Optitrack(const std::string &_localIP, float _threshold)
  : myIPAddress(_localIP),
    threshold(_threshold)
{
#ifdef _WIN32
  if (!initialized)
  {
    WSADATA wsaData;

    // Request WinSock v2.0.
    WORD wVersionRequested = MAKEWORD(2, 0);
    // Load WinSock DLL.
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
     std::cerr << "Unable to load WinSock DLL" << std::endl;
     return;
    }

    initialized = true;
  }
#endif

  // Make a new socket for receiving OptiTrack updates.
  if ((this->dataSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    std::cerr << "Socket creation failed." << std::endl;
    return;
  }

  // Socket option: SO_REUSEADDR.
  int value = 1;
  if (setsockopt(this->dataSocket, SOL_SOCKET, SO_REUSEADDR,
    reinterpret_cast<const char *>(&value), sizeof(value)) != 0)
  {
    std::cerr << "Error setting socket option (SO_REUSEADDR)." << std::endl;
    return;
  }

#ifdef SO_REUSEPORT
  // Socket option: SO_REUSEPORT.
  int reusePort = 1;
  if (setsockopt(this->dataSocket, SOL_SOCKET, SO_REUSEPORT,
        reinterpret_cast<const char *>(&reusePort), sizeof(reusePort)) != 0)
  {
    std::cerr << "Error setting socket option (SO_REUSEPORT)." << std::endl;
    return;
  }
#endif

  // Join the multicast group.
  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(this->MulticastAddress.c_str());
  mreq.imr_interface.s_addr = inet_addr(this->myIPAddress.c_str());
  if (setsockopt(this->dataSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
    reinterpret_cast<const char*>(&mreq), sizeof(mreq)) != 0)
  {
    std::cerr << "Error setting socket option (IP_ADD_MEMBERSHIP)."
              << std::endl;
    return;
  }

  // Bind the socket to the "Optitrack tracking updates" port.
  struct sockaddr_in mySocketAddr;
  memset(&mySocketAddr, 0, sizeof(mySocketAddr));
  mySocketAddr.sin_family = AF_INET;
  mySocketAddr.sin_port = htons(this->PortData);
  mySocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);;
  if (bind(this->dataSocket, (struct sockaddr *)&mySocketAddr,
    sizeof(struct sockaddr)) < 0)
  {
    std::cerr << "Binding to a local port failed." << std::endl;
    return;
  }
}

/////////////////////////////////////////////////
void Optitrack::Receive()
{
  char buffer[20000];
  socklen_t addr_len = sizeof(struct sockaddr);
  sockaddr_in theirAddress;

  while (1)
  {
    // Block until we receive a datagram from the network (from anyone
    // including ourselves)
    if (recvfrom(this->dataSocket, buffer, sizeof(buffer), 0,
         (sockaddr *)&theirAddress, &addr_len) < 0)
    {
      std::cerr << "Optitrack::RunReceptionTask() Recvfrom failed" << std::endl;
      continue;
    }

    // Dispatch the data received.
    this->Unpack(buffer);
  }
}

/////////////////////////////////////////////////
void Optitrack::Unpack(char *pData)
{
  int major = this->NatNetVersionMajor;
  int minor = this->NatNetVersionMinor;

  char *ptr = pData;

  // message ID
  int MessageID = 0;
  memcpy(&MessageID, ptr, 2); ptr += 2;

  // size
  int nBytes = 0;
  memcpy(&nBytes, ptr, 2); ptr += 2;

  if (MessageID == 7)      // FRAME OF MOCAP DATA packet
  {
    // frame number
    int frameNumber = 0; memcpy(&frameNumber, ptr, 4); ptr += 4;

    // number of data sets (markersets, rigidbodies, etc)
    int nMarkerSets = 0; memcpy(&nMarkerSets, ptr, 4); ptr += 4;

    for (int i = 0; i < nMarkerSets; i++)
    {
      // Markerset name
      std::string szName(ptr);
      int nDataBytes = (int) strlen(ptr) + 1;
      ptr += nDataBytes;
      // std::cout << "Model Name: " << szName << std::endl;

      // marker data
      int nMarkers = 0;
      memcpy(&nMarkers, ptr, 4);
      ptr += 4;
      // std::cout << "Marker Count: " << nMarkers << std::endl;

      for (int j = 0; j < nMarkers; j++)
      {
        float x = 0; memcpy(&x, ptr, 4); ptr += 4;
        float y = 0; memcpy(&y, ptr, 4); ptr += 4;
        float z = 0; memcpy(&z, ptr, 4); ptr += 4;
        // std::cout << "\tMarker " << j << " : [x="
        //          << x << ",y=" << y << ",z=" << z << "]" << std::endl;
      }
    }

    // unidentified markers
    int nOtherMarkers = 0; memcpy(&nOtherMarkers, ptr, 4); ptr += 4;
    for(int j=0; j < nOtherMarkers; j++)
    {
        float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
        float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
        float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
    }

    // rigid bodies
    int nRigidBodies = 0;
    memcpy(&nRigidBodies, ptr, 4); ptr += 4;
    // std::cout << "Rigid Body Count : " << nRigidBodies << std::endl;
    for (int j = 0; j < nRigidBodies; j++)
    {
      // rigid body pos/ori
      int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
      float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
      float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
      float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
      float qx = 0; memcpy(&qx, ptr, 4); ptr += 4;
      float qy = 0; memcpy(&qy, ptr, 4); ptr += 4;
      float qz = 0; memcpy(&qz, ptr, 4); ptr += 4;
      float qw = 0; memcpy(&qw, ptr, 4); ptr += 4;
      // std::cout << "ID : " << ID << std::endl;
      // std::cout << "pos: [" << x << "," << y << "," << z << "]" << std::endl;
      // std::cout << "ori: [" << qx << "," << qy << ","
      //                       << qz << "," << qw << "]" << std::endl;

      /// ID=1 is the "HeadTracker".
      if (ID == 1)
      {
        static float prevX = x;
        static float prevY = y;
        static float prevZ = z;

        if ((std::abs(x - prevX) > threshold) ||
            (std::abs(y - prevY) > threshold) ||
            (std::abs(z - prevZ) > threshold))
        {
          std::cout << "The body has moved!" << std::endl;
          prevX = x;
          prevY = y;
          prevZ = z;
        }
      }

      // associated marker positions
      int nRigidMarkers = 0;  memcpy(&nRigidMarkers, ptr, 4); ptr += 4;
      int nMarkerBytes = nRigidMarkers*3*sizeof(float);
      float* markerData = (float*)malloc(nMarkerBytes);
      memcpy(markerData, ptr, nMarkerBytes);
      ptr += nMarkerBytes;

      if (major >= 2)
      {
        // associated marker IDs
        nMarkerBytes = nRigidMarkers*sizeof(int);
        int* markerIDs = (int*)malloc(nMarkerBytes);
        memcpy(markerIDs, ptr, nMarkerBytes);
        ptr += nMarkerBytes;

        // associated marker sizes
        nMarkerBytes = nRigidMarkers*sizeof(float);
        float* markerSizes = (float*)malloc(nMarkerBytes);
        memcpy(markerSizes, ptr, nMarkerBytes);
        ptr += nMarkerBytes;

        if (markerIDs)
          free(markerIDs);
        if (markerSizes)
          free(markerSizes);
      }

      if (markerData)
        free(markerData);

      if (major >= 2)
      {
        // Mean marker error
        float fError = 0.0f; memcpy(&fError, ptr, 4); ptr += 4;
      }

      // 2.6 and later
      if (((major == 2) && (minor >= 6)) || (major > 2) || (major == 0))
      {
        // params
        short params = 0; memcpy(&params, ptr, 2); ptr += 2;
      }
    } // next rigid body
  }
  else
  {
    std::cerr << "Unrecognized Packet Type." << std::endl;
  }
}
