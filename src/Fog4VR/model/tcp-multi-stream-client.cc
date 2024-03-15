/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2016 Technische Universitaet Berlin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "tcp-multi-stream-client.h"
#include <math.h>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include "ns3/global-value.h"
#include <ns3/core-module.h>
#include "tcp-stream-server.h"
#include <unistd.h>
#include <iterator>
#include <numeric>
#include <iomanip>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <errno.h>
#include "tcp-stream-controller.h"

namespace ns3 {

template <typename T>
std::string ToString (T val)
{
  std::stringstream stream;
  stream << val;
  return stream.str ();
}

NS_LOG_COMPONENT_DEFINE ("TcpMultiStreamClientApplication");

NS_OBJECT_ENSURE_REGISTERED (TcpMultiStreamClient);

void
TcpMultiStreamClient::Controller (controllerEvent event)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_UNCOND(m_currentPlaybackIndex);
  if (state == initial)
    {
      std::pair <uint32_t, double> requestReply;
      requestReply=sendRequest (conId,conSize,m_clientId,serverId,polId);
      serverId=requestReply.first;
      HandoverApplication(choiceServer(conId,conSize,m_clientId,requestReply.first));
      if (handover)
      {
        HandoverApplication(newip);
        handover=false;
      }
      NS_LOG_UNCOND(m_segmentCounter); NS_LOG_UNCOND(m_peerAddress);
      RequestRepIndex ();
      state = downloading;
      m_downloadRequestSent = Simulator::Now ().GetMicroSeconds ();
      Simulator::Schedule (Seconds (requestReply.second), &TcpMultiStreamClient::StartSend,this);
      //Send (m_videoData.segmentSize.at (m_currentRepIndex).at (m_segmentCounter));
      return;
    }

  if (state == downloading)
    {
      if (handover)
      {
        HandoverApplication(newip);
        handover=false;
      }

      PlaybackHandle ();
      if (m_currentPlaybackIndex <= m_lastSegmentIndex)
        {
          /*  e_d  */
          m_segmentCounter++; NS_LOG_UNCOND(m_segmentCounter);
          RequestRepIndex (); NS_LOG_UNCOND(m_peerAddress);
          state = downloadingPlaying;
          SendMultiple (m_videoData.segmentSize.at (m_currentRepIndex).at (m_segmentCounter));
        }
      else
        {
          /*  e_df  */
          state = playing;
        }
      controllerEvent ev = playbackFinished;
       //std::cerr << "Client " << m_clientId << " " << Simulator::Now ().GetSeconds () << "\n";
      Simulator::Schedule (MicroSeconds (m_videoData.segmentDuration), &TcpMultiStreamClient::Controller, this, ev);
      return;
    }


  else if (state == downloadingPlaying)
    {
      if (event == downloadFinished)
        {
          if (handover)
            {
              HandoverApplication(newip);
              handover=false;
            }
          if (m_segmentCounter < m_lastSegmentIndex)
            {
              m_segmentCounter++; NS_LOG_UNCOND(m_segmentCounter);
              RequestRepIndex (); NS_LOG_UNCOND(m_peerAddress);
            }

          if (m_bDelay > 0 && m_segmentCounter <= m_lastSegmentIndex)
            {
              /*  e_dirs */
              state = playing;
              controllerEvent ev = irdFinished;
              Simulator::Schedule (MicroSeconds (m_bDelay), &TcpMultiStreamClient::Controller, this, ev);
            }
          else if (m_segmentCounter == m_lastSegmentIndex)
            {
              /*  e_df  */
              state = playing;
            }
          else
            {
              /*  e_d  */
              SendMultiple (m_videoData.segmentSize.at (m_currentRepIndex).at (m_segmentCounter));
            }
        }
      else if (event == playbackFinished)
        {
          if (!PlaybackHandle ())
            {
              /*  e_pb  */
              controllerEvent ev = playbackFinished; //NS_LOG_UNCOND("playback finished");
               //std::cerr << "FIRST CASE. Client " << m_clientId << " " << Simulator::Now ().GetSeconds () << "\n";
              Simulator::Schedule (MicroSeconds (m_videoData.segmentDuration), &TcpMultiStreamClient::Controller, this, ev);
            }
          else
            {
              /*  e_pu  */
              state = downloading;
            }
        }
      return;
    }


  else if (state == playing)
    {
      if (event == irdFinished)
        {
          /*  e_irc  */
          //NS_LOG_UNCOND("maoe");
          state = downloadingPlaying;
          SendMultiple (m_videoData.segmentSize.at (m_currentRepIndex).at (m_segmentCounter));
        }
      else if (event == playbackFinished && m_currentPlaybackIndex < m_lastSegmentIndex)
        {
          /*  e_pb  */
           //std::cerr << "SECOND CASE. Client " << m_clientId << " " << Simulator::Now ().GetSeconds () << "\n";
          PlaybackHandle (); //NS_LOG_UNCOND("playback finished antes do final");
          controllerEvent ev = playbackFinished;
          Simulator::Schedule (MicroSeconds (m_videoData.segmentDuration), &TcpMultiStreamClient::Controller, this, ev);
        }
      else if (event == playbackFinished && m_currentPlaybackIndex >= m_lastSegmentIndex)
        {
       //NS_LOG_UNCOND("playback finished no final");
          /*  e_pf  */
          NS_LOG_UNCOND("end");
          state = terminal;
          finishedRequest(conId,conSize,m_clientId,serverId);
          StopApplication ();
        }
      return;
    }
}
//break ns3::TcpMultiStreamClient::Controller if m_segmentCounter == 25
TypeId
TcpMultiStreamClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpMultiStreamClient")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<TcpMultiStreamClient> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&TcpMultiStreamClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("AuxiliaryAddresses",
                   "A list of aditional addresses to connect",
                   AttributeContainerValue <AddressValue> (),
                   MakeAttributeContainerAccessor <AddressValue> (&TcpMultiStreamClient::m_auxiliary_peers),
                   MakeAttributeContainerChecker <AddressValue> (MakeAddressChecker ()))
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TcpMultiStreamClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("SegmentDuration",
                   "The duration of a segment in microseconds",
                   UintegerValue (2000000),
                   MakeUintegerAccessor (&TcpMultiStreamClient::m_segmentDuration),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("SegmentSizeFilePath",
                   "The relative path (from ns-3.x directory) to the file containing the segment sizes in bytes",
                   StringValue ("bitrates.txt"),
                   MakeStringAccessor (&TcpMultiStreamClient::m_segmentSizeFilePath),
                   MakeStringChecker ())
    .AddAttribute ("SimulationId",
                   "The ID of the current simulation, for logging purposes",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TcpMultiStreamClient::m_simulationId),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NumberOfClients",
                   "The total number of clients for this simulation, for logging purposes",
                   UintegerValue (1),
                   MakeUintegerAccessor (&TcpMultiStreamClient::m_numberOfClients),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ClientId",
                   "The ID of the this client object, for logging purposes",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TcpMultiStreamClient::m_clientId),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ServerId",
                   "The ID of the initial server for the client object, for logging purposes",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TcpMultiStreamClient::serverId),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("PolId",
                   "The ID of the politica for the client object, for logging purposes",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TcpMultiStreamClient::polId),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ContentId",
                   "The ID of the requested content for the client object",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TcpMultiStreamClient::conId),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ContentSize",
                   "The size of the requested content for the client object",
                   DoubleValue (0),
                   MakeDoubleAccessor (&TcpMultiStreamClient::conSize),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

TcpMultiStreamClient::TcpMultiStreamClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_data = 0;
  m_dataSize = 0;
  state = initial;
  handover = false;
  m_currentRepIndex = 0;
  m_segmentCounter = 0;
  m_bDelay = 0;
  m_bytesReceived = 0;
  for(auto address : m_auxiliary_peers){
	  m_socketBytesReceived[address] = 0;
  }
  m_socketBytesReceived[m_peerAddress] = 0;
  m_segmentsInBuffer = 0;
  m_bufferUnderrun = false;
  m_currentPlaybackIndex = 0;
  bufferUnderrunCount = 0;
  bufferUnderrunTotalTime = 0;
  throughput=0;
  underrunBegin=0;
}

void
TcpMultiStreamClient::Initialise (std::string algorithm, uint16_t clientId)
{
  NS_LOG_FUNCTION (this);
  m_videoData.segmentDuration = m_segmentDuration;
  if (ReadInBitrateValues (ToString (m_segmentSizeFilePath)) == -1)
    {
      NS_LOG_ERROR ("Opening test bitrate file failed. Terminating.\n");
      Simulator::Stop ();
      Simulator::Destroy ();
    }
  m_lastSegmentIndex = (int64_t) m_videoData.segmentSize.at (0).size () - 1;
  m_highestRepIndex = m_videoData.averageBitrate.size () - 1;
  if (algorithm == "tobasco")
    {
      algo = new TobascoAlgorithm (m_videoData, m_playbackData, m_bufferData, m_throughput);
    }
  else if (algorithm == "panda")
    {
      algo = new PandaAlgorithm (m_videoData, m_playbackData, m_bufferData, m_throughput);
    }
  else if (algorithm == "festive")
    {
      algo = new FestiveAlgorithm (m_videoData, m_playbackData, m_bufferData, m_throughput);
    }
  else
    {
      NS_LOG_ERROR ("Invalid algorithm name entered. Terminating.");
      StopApplication ();
      Simulator::Stop ();
      Simulator::Destroy ();
    }
  m_algoName = algorithm;
  InitializeLogFiles (ToString (m_simulationId), ToString (m_clientId), ToString (m_numberOfClients), ToString (serverId), ToString (polId));

}

TcpMultiStreamClient::~TcpMultiStreamClient ()
{ //NS_LOG_UNCOND("nao eh aqui no client");
  NS_LOG_FUNCTION (this);
  m_socket = 0;

  delete algo;
  algo = NULL;
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void
TcpMultiStreamClient::RequestRepIndex ()
{
  NS_LOG_FUNCTION (this);
  algorithmReply answer;
  answer = algo->GetNextRep ( m_segmentCounter, m_clientId );
  m_currentRepIndex = answer.nextRepIndex;
  NS_ASSERT_MSG (answer.nextRepIndex <= m_highestRepIndex, "The algorithm returned a representation index that's higher than the maximum");
  m_playbackData.playbackIndex.push_back (answer.nextRepIndex);
  m_bDelay = answer.nextDownloadDelay;
   //std::cerr << m_segmentCounter << "\n";
  LogAdaptation (answer);
}

void
TcpMultiStreamClient::StartSend ()
{
  SendMultiple (m_videoData.segmentSize.at (m_currentRepIndex).at (m_segmentCounter));
}

template <typename T>
void
TcpMultiStreamClient::SendAuxiliary (T & message, uint16_t index)
{
  NS_LOG_FUNCTION (this << " ip " << m_auxiliary_peers[index] << " index " << index);
  PreparePacket (message);
  Ptr<Packet> p;
  p = Create<Packet> (m_data, m_dataSize);
  if (m_segmentCounter!=0)
  {
    m_downloadRequestSent = Simulator::Now ().GetMicroSeconds ();
  }
  m_auxiliary_sockets[index]->Send (p);
}

void
TcpMultiStreamClient::SendMultiple (uint32_t numBytes, std::vector<int> serverIds)
{
  NS_LOG_FUNCTION (this);
  int16_t numServers = serverIds.size()+1;
  uint32_t toSend = numBytes/numServers;
  uint32_t remaining = numBytes;
  Address server;

  for(int i=0;i<numServers-1;i++) {
	  SendAuxiliary(toSend, serverIds[i]);
	  server = m_auxiliary_peers[serverIds[i]];
	  m_subsegment_size[server] = toSend;
	  remaining -= toSend;
  }

  Send(remaining);
  m_subsegment_size[m_peerAddress] = remaining;
}

template <typename T>
void
TcpMultiStreamClient::Send (T & message)
{
  NS_LOG_FUNCTION (this);
  PreparePacket (message);
  Ptr<Packet> p;
  p = Create<Packet> (m_data, m_dataSize);
  if (m_segmentCounter!=0)
  {
    m_downloadRequestSent = Simulator::Now ().GetMicroSeconds ();
  }
  m_socket->Send (p);
}

void
TcpMultiStreamClient::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address socket_from;
  Address from;
  if (m_bytesReceived == 0)
    {
      m_transmissionStartReceivingSegment = Simulator::Now ().GetMicroSeconds ();
    }
  uint32_t packetSize;
  while ( (packet = socket->RecvFrom (socket_from)) )
    {
		from = InetSocketAddress::ConvertFrom(socket_from).GetIpv4();
      packetSize = packet->GetSize ();
      m_bytesReceived += packetSize;
      totalBytes += packetSize;
	  m_socketBytesReceived[from] += packetSize;
	  NS_LOG_INFO("socket received: " << m_socketBytesReceived[from]
			  << " expected: " << m_subsegment_size[from] << "\n");
	if(m_socketBytesReceived[from] == m_subsegment_size[from]){
		LogDownload (from, m_subsegment_size[from]);
		m_socketBytesReceived[from] = 0;
      if (m_bytesReceived == m_videoData.segmentSize.at (m_currentRepIndex).at (m_segmentCounter))
        {
          serverIpv4=socket_from;
          NS_LOG_UNCOND(m_bytesReceived);
          m_transmissionEndReceivingSegment = Simulator::Now ().GetMicroSeconds ();
          SegmentReceivedHandle ();
        }
    	}
	}
}

int
TcpMultiStreamClient::ReadInBitrateValues (std::string segmentSizeFile)
{
  NS_LOG_FUNCTION (this);
  std::ifstream myfile;
  myfile.open (segmentSizeFile.c_str ());
  if (!myfile)
    {
      return -1;
    }
  std::string temp;
  int64_t averageByteSizeTemp = 0;
  while (std::getline (myfile, temp))
    {
      if (temp.empty ())
        {
          break;
        }
      std::istringstream buffer (temp);
      std::vector<int64_t> line ((std::istream_iterator<int64_t> (buffer)),
                                 std::istream_iterator<int64_t>());
      m_videoData.segmentSize.push_back (line);
      averageByteSizeTemp = (int64_t) std::accumulate ( line.begin (), line.end (), 0.0) / line.size ();
      m_videoData.averageBitrate.push_back ((8.0 * averageByteSizeTemp) / (m_videoData.segmentDuration / 1000000.0));
    }
  NS_ASSERT_MSG (!m_videoData.segmentSize.empty (), "No segment sizes read from file.");
  return 1;
}

void
TcpMultiStreamClient::SegmentReceivedHandle ()
{
  NS_LOG_FUNCTION (this);

  m_bufferData.timeNow.push_back (m_transmissionEndReceivingSegment);
  if (m_segmentCounter > 0)
    { //if a buffer underrun is encountered, the old buffer level will be set to 0, because the buffer can not be negative
      m_bufferData.bufferLevelOld.push_back (std::max (m_bufferData.bufferLevelNew.back () -
                                                       (m_transmissionEndReceivingSegment - m_throughput.transmissionEnd.back ()), (int64_t)0));
    }
  else //first segment
    {
      m_bufferData.bufferLevelOld.push_back (0);
    }
  m_bufferData.bufferLevelNew.push_back (m_bufferData.bufferLevelOld.back () + m_videoData.segmentDuration);

  m_throughput.bytesReceived.push_back (m_videoData.segmentSize.at (m_currentRepIndex).at (m_segmentCounter));
  m_throughput.transmissionStart.push_back (m_transmissionStartReceivingSegment);
  m_throughput.transmissionRequested.push_back (m_downloadRequestSent);
  m_throughput.transmissionEnd.push_back (m_transmissionEndReceivingSegment);

  LogBuffer ();

  m_segmentsInBuffer++;
  m_bytesReceived = 0;
  if (m_segmentCounter == m_lastSegmentIndex)
    {
      m_bDelay = 0;
    }

  controllerEvent event = downloadFinished;
  Controller (event);

}

bool
TcpMultiStreamClient::PlaybackHandle ()
{
  std::cout << m_segmentsInBuffer << " : "<< "server" << m_peerAddress << std::endl;
  NS_LOG_FUNCTION (this);
  int64_t timeNow = Simulator::Now ().GetMicroSeconds ();
  // if we got called and there are no segments left in the buffer, there is a buffer underrun
  if (m_segmentsInBuffer == 0 && m_currentPlaybackIndex < m_lastSegmentIndex && !m_bufferUnderrun)
    {
      bufferUnderrunCount++;
      m_bufferUnderrun = true;
      underrunBegin = timeNow / (double)1000000;
      bufferUnderrunLog << std::setfill (' ') << std::setw (0) << Ipv4Address::ConvertFrom (m_peerAddress) << ";";
      bufferUnderrunLog << std::setfill (' ') << std::setw (0) << timeNow / (double)1000000 << ";";
      bufferUnderrunLog.flush ();
      return true;
    }
  else
  {

    if (m_segmentsInBuffer > 0)
    {
      if (m_bufferUnderrun)
        {
          m_bufferUnderrun = false;
          double delta = (timeNow / (double)1000000) - underrunBegin;
          bufferUnderrunTotalTime = bufferUnderrunTotalTime + delta;
          bufferUnderrunLog << std::setfill (' ') << std::setw (0) << timeNow / (double)1000000 << ";";
          bufferUnderrunLog << std::setfill (' ') << std::setw (0) << delta << ";";
          bufferUnderrunLog << std::setfill (' ') << std::setw (0) << bufferUnderrunTotalTime << ";\n";
          //bufferUnderrunLog << std::setfill (' ') << std::setw (0) << Ipv4Address::ConvertFrom (m_peerAddress) << ";\n";
          bufferUnderrunLog.flush ();
        }
      m_playbackData.playbackStart.push_back (timeNow);
      if(m_currentPlaybackIndex==0)
      {
        playbackStart=timeNow  / (double)1000000;
        NS_LOG_UNCOND(playbackStart);
      }
      LogPlayback ();
      m_segmentsInBuffer--;
      m_currentPlaybackIndex++;
      return false;
    }
  }
  NS_LOG_UNCOND("fudeu");
  NS_LOG_UNCOND(m_clientId);
  return true;
}

void
TcpMultiStreamClient::SetRemote (Address ip, uint16_t port, uint16_t polId)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
  polId=polId;
}

void
TcpMultiStreamClient::SetRemote (Ipv4Address ip, uint16_t port, uint16_t polId)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
  polId=polId;
}

void
TcpMultiStreamClient::SetRemote (Ipv6Address ip, uint16_t port, uint16_t polId)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
  polId=polId;
}

void
TcpMultiStreamClient::DoDispose (void)
{//NS_LOG_UNCOND("Morreu");
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

double
TcpMultiStreamClient::GetBufferUnderrunTotalTime()
{
  return bufferUnderrunTotalTime;
}

uint64_t
TcpMultiStreamClient::GetBufferUnderrunCount()
{
  return bufferUnderrunCount;
}

uint64_t
TcpMultiStreamClient::GetRepIndex()
{

  return m_currentRepIndex;
}

double
TcpMultiStreamClient::GetPlaybackStart()
{
  return playbackStart;
}

double
TcpMultiStreamClient::GetThroughput()
{
    Time now = Simulator::Now ();
    double Throughput = totalBytes * (double) 8 / 1e6;
    totalBytes=0;
    LogThroughput (Throughput);
    std::cout << now.GetSeconds () << "s: \t" << Throughput << " Mbit/s" << std::endl;
    return (Throughput);
}

std::string
TcpMultiStreamClient::GetServerAddress()
{
  std::string a = ToString(Ipv4Address::ConvertFrom (m_peerAddress));
  return a;
}

void
TcpMultiStreamClient::SetHandover(Address ip)
{
  handover=true;
  newip=ip;
}

std::string
TcpMultiStreamClient::GetNewServerAddress()
{
  std::string a = ToString(Ipv4Address::ConvertFrom (newip));
  return a;
}

bool
TcpMultiStreamClient::checkHandover()
{
  return handover;
}

void
TcpMultiStreamClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this); //NS_LOG_UNCOND("client StartApplication 471");
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType (m_peerAddress) == true)
        {
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType (m_peerAddress) == true)
        {
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort));
        }
      m_socket->SetConnectCallback (
        MakeCallback (&TcpMultiStreamClient::ConnectionSucceeded, this),
        MakeCallback (&TcpMultiStreamClient::ConnectionFailed, this));
      m_socket->SetRecvCallback (MakeCallback (&TcpMultiStreamClient::HandleRead, this));

      NS_LOG_UNCOND("additional peers:");
      for(auto address : m_auxiliary_peers) {
          NS_LOG_UNCOND(Ipv4Address::ConvertFrom(address));
          Ptr<Socket> aux = Socket::CreateSocket (GetNode (), tid);
          if (Ipv4Address::IsMatchingType (address) == true)
            {
              aux->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (address), m_peerPort));
            }
          else if (Ipv6Address::IsMatchingType (address) == true)
            {
              aux->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (address), m_peerPort));
            }
            aux->SetConnectCallback (
            MakeCallback (&TcpMultiStreamClient::ConnectionSucceeded, this),
            MakeCallback (&TcpMultiStreamClient::ConnectionFailed, this));
            aux->SetRecvCallback (MakeCallback (&TcpMultiStreamClient::HandleRead, this));
            m_auxiliary_sockets.push_back(aux);
      }
    }
}

int
TcpMultiStreamClient::GetIpIndex(Address ip)
{
    for(uint16_t i=0; i<m_auxiliary_peers.size(); i++)
    {
        if(ip == m_auxiliary_peers[i])
            return i;
    }
    return -1;
}

void
TcpMultiStreamClient::HandoverApplication (Address ip)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_UNCOND("client HandoverApplication");
  NS_LOG_UNCOND(m_segmentCounter);

  bufferUnderrunCount=0;
  bufferUnderrunTotalTime=0;
  controllerState temp = state;
  m_peerAddress = ip;
  state = terminal;
  NS_LOG_UNCOND(ip);
  if (Ipv4Address::IsMatchingType (m_peerAddress) == true)
  {
    if (m_socket != 0)
    {
        int index = GetIpIndex(ip);
        NS_ASSERT_MSG(index != -1, "The server is not on the peer list");
        Ptr<Socket> temp = m_socket;
        m_socket = m_auxiliary_sockets[index];
        m_auxiliary_sockets[index] = temp;
    }
  }
  state = temp;
}

void
TcpMultiStreamClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  state = terminal;
  //NS_LOG_UNCOND("StopApplication antes do final");
  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
  downloadLog.close ();
  playbackLog.close ();
  adaptationLog.close ();
  bufferLog.close ();
  throughputLog.close ();
  bufferUnderrunLog.close ();
  //NS_LOG_UNCOND("StopApplication do final valendo");
}

bool
TcpMultiStreamClient::check ()
{//NS_LOG_UNCOND(state);
  if (state==terminal)
  {
    return true;
  }
  else
  {
    return false;
  }
}

template <typename T>
void
TcpMultiStreamClient::PreparePacket (T & message)
{
  NS_LOG_FUNCTION (this << message);
  std::ostringstream ss;
  ss << message;
  ss.str ();
  uint32_t dataSize = ss.str ().size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }
  memcpy (m_data, ss.str ().c_str (), dataSize);
}

void
TcpMultiStreamClient::ConnectionSucceeded (Ptr<Socket> socket)
{ //NS_LOG_UNCOND("client ConnectionSucceeded 571");
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Tcp Stream Client connection succeeded");
  if (state==initial && socket == m_socket)
  {
    NS_LOG_LOGIC ("Controller called");
    controllerEvent event = init;
    Controller (event);
  }
}

void
TcpMultiStreamClient::ConnectionFailed (Ptr<Socket> socket)
{ //NS_LOG_UNCOND("client ConnectionFailed  580");
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Tcp Stream Client connection failed");
}

void
TcpMultiStreamClient::LogThroughput (double packetSize)
{
  NS_LOG_FUNCTION (this);
  throughputLog << std::setfill (' ') << std::setw (0) << Simulator::Now ().GetMicroSeconds ()  / (double) 1000000 << ";"
                << std::setfill (' ') << std::setw (0) << packetSize << ";"
                << std::setfill (' ') << std::setw (0) << Ipv4Address::ConvertFrom (m_peerAddress) << ";\n";
  throughputLog.flush ();
}

void
TcpMultiStreamClient::LogDownload (Address server, uint64_t socket_bytes)
{
  NS_LOG_FUNCTION (this);
  downloadLog << std::setfill (' ') << std::setw (0) << m_segmentCounter << ";"
              << std::setfill (' ') << std::setw (0) << m_downloadRequestSent / (double)1000000 << ";"
              << std::setfill (' ') << std::setw (0) << m_transmissionStartReceivingSegment / (double)1000000 << ";"
              << std::setfill (' ') << std::setw (0) << m_transmissionEndReceivingSegment / (double)1000000 << ";"
              << std::setfill (' ') << std::setw (0) << m_videoData.segmentSize.at (m_currentRepIndex).at (m_segmentCounter) << ";"
              << std::setfill (' ') << std::setw (0) << socket_bytes << ";"
              << std::setfill (' ') << std::setw (0) << Ipv4Address::ConvertFrom (server) << ";"
              << std::setfill (' ') << std::setw (0) << "Y;\n";
  downloadLog.flush ();
}

void
TcpMultiStreamClient::LogBuffer ()
{
  NS_LOG_FUNCTION (this);
  bufferLog << std::setfill (' ') << std::setw (0) << m_transmissionEndReceivingSegment / (double)1000000 << ";"
            << std::setfill (' ') << std::setw (0) << m_bufferData.bufferLevelOld.back () / (double)1000000 << "\n"
            << std::setfill (' ') << std::setw (0) << m_transmissionEndReceivingSegment / (double)1000000 << ";"
            << std::setfill (' ') << std::setw (0) << m_bufferData.bufferLevelNew.back () / (double)1000000 << ";\n";
  bufferLog.flush ();
}

void
TcpMultiStreamClient::LogAdaptation (algorithmReply answer)
{
  NS_LOG_FUNCTION (this);
  adaptationLog << std::setfill (' ') << std::setw (0) << m_segmentCounter << ";"
                << std::setfill (' ') << std::setw (0) << m_currentRepIndex << ";"
                << std::setfill (' ') << std::setw (0) << answer.decisionTime / (double)1000000 << ";"
                << std::setfill (' ') << std::setw (0) << answer.decisionCase << ";"
                << std::setfill (' ') << std::setw (0) << answer.delayDecisionCase << ";\n";
  adaptationLog.flush ();
}

void
TcpMultiStreamClient::LogPlayback ()
{
  NS_LOG_FUNCTION (this);
  playbackLog << std::setfill (' ') << std::setw (0) << m_currentPlaybackIndex << ";"
              << std::setfill (' ') << std::setw (0) << Simulator::Now ().GetMicroSeconds ()  / (double)1000000 << ";"
              << std::setfill (' ') << std::setw (0) << m_playbackData.playbackIndex.at (m_currentPlaybackIndex) << ";"
              << std::setfill (' ') << std::setw (0) << InetSocketAddress::ConvertFrom (serverIpv4).GetIpv4 () << ";\n";
  playbackLog.flush ();
}

void
TcpMultiStreamClient::InitializeLogFiles (std::string simulationId, std::string clientId, std::string numberOfClients, std::string serverId, std::string pol)
{
  NS_LOG_FUNCTION (this);

  std::string dLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "cl" + clientId + "_" + "server" + serverId + "_" + "downloadLog.csv";
  downloadLog.open (dLog.c_str ());
  downloadLog << "Segment_Index;Download_Request_Sent;Download_Start;Download_End;Segment_Size;Socket_Bytes;Server;Download_OK\n";
  downloadLog.flush ();

  std::string pLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "cl" + clientId + "_" + "server" + serverId + "_" + "playbackLog.csv";
  playbackLog.open (pLog.c_str ());
  playbackLog << "Segment_Index;Playback_Start;Quality_Level;Server_Address\n";
  playbackLog.flush ();

  std::string aLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "cl" + clientId + "_" + "server" + serverId + "_" + "adaptationLog.csv";
  adaptationLog.open (aLog.c_str ());
  adaptationLog << "Segment_Index;Rep_Level;Decision_Point_Of_Time;Case;DelayCase\n";
  adaptationLog.flush ();

  std::string bLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "cl" + clientId + "_" + "server" + serverId + "_" + "bufferLog.csv";
  bufferLog.open (bLog.c_str ());
  bufferLog << "Time_Now;Buffer_Level \n";
  bufferLog.flush ();

  std::string tLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "cl" + clientId + "_" + "server" + serverId + "_" + "throughputLog.csv";
  throughputLog.open (tLog.c_str ());
  throughputLog << "Time_Now;MBytes_Received;Server_Address\n";
  throughputLog.flush ();

  std::string buLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "cl" + clientId + "_" + "server" + serverId + "_" + "bufferUnderrunLog.csv";
  bufferUnderrunLog.open (buLog.c_str ());
  bufferUnderrunLog << ("Server_Address;Buffer_Underrun_Started_At;Until;Buffer_Underrun_Duration;bufferUnderrunTotalTime\n");
  bufferUnderrunLog.flush ();
}

} // Namespace ns3
