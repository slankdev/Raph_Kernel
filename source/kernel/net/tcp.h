/*
 *
 * Copyright (c) 2016 Project Raphine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Levelfour
 * 
 */

#ifndef __RAPH_KERNEL_NET_TCP_H__
#define __RAPH_KERNEL_NET_TCP_H__

#include <stdint.h>
#include <stdlib.h>
#include "socket.h"
#include "layer.h"
#include "ip.h"

struct TCPHeader {
  // source port
  uint16_t sport;
  // destination port
  uint16_t dport;
  // sequence number
  uint32_t seqNumber;
  // acknowledge number
  uint32_t ackNumber;
  // TCP header length (upper 4bit) | Reserved (lower 4bit)
  uint8_t headerLen;
  // Session flags (lower 6bit) + Reserved (upper 2bit)
  uint8_t flag;
  // window size
  uint16_t windowSize;
  // checksum
  uint16_t checksum;
  // urgent pointer
  uint16_t urgentPointer;
} __attribute__ ((packed));

class TCPCtrl : public L4Ctrl {
  uint8_t _tcpSessionType;
  uint32_t _sequence;
  uint32_t _ack;

  static const uint32_t kBasicBufsize     = 0x10;
  static const uint8_t kSrcPortOffset     = 0;
  static const uint8_t kDstPortOffset     = 2;
  static const uint8_t kSeqOffset         = 4;
  static const uint8_t kAckOffset         = 8;
  static const uint8_t kSessionTypeOffset = 13;
  static const uint8_t kWindowSizeOffset  = 14;
  static const uint8_t kFlagFIN           = 1 << 0;
  static const uint8_t kFlagSYN           = 1 << 1;
  static const uint8_t kFlagRST           = 1 << 2;
  static const uint8_t kFlagPSH           = 1 << 3;
  static const uint8_t kFlagACK           = 1 << 4;
  static const uint8_t kFlagURG           = 1 << 5;

  inline void InitSequence() { _sequence = rand(); }

public:
  TCPCtrl() : _tcpSessionType(kFlagRST), _sequence(0), _ack(0) {}
  virtual int32_t GenerateHeader(uint8_t *header,
                                 uint32_t length,
                                 uint16_t sport,
                                 uint16_t dport);
  virtual bool FilterPacket(uint8_t *packet,
                            uint16_t sport,
                            uint16_t dport);
};

#endif // __RAPH_KERNEL_NET_TCP_H__