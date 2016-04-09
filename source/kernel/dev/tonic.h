/*
 *
 * Copyright (c) 2016 Raphine Project
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
 * 16/04/09: created by Levelfour
 * 
 */

#ifndef __RAPH_KERNEL_DEV_TONIC_H__
#define __RAPH_KERNEL_DEV_TONIC_H__

#include <stdint.h>
#include <polling.h>
#include <dev/eth.h>

struct TonicRxDesc {
  uint64_t baseAddress;  /* address of buffer */
  uint16_t packetLength; /*  */
};

struct TonicTxDesc {
  uint64_t baseAddress;
  uint16_t packetLength;
};

class Tonic : public DevEthernet, Polling {
public:
  Tonic(uint8_t bus, uint8_t device, bool mf) : DevEthernet(bus, device, mf) {}
  static bool InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf);

  // from Polling
  void Handle() override;

  void Setup();
  virtual void SetupNetInterface() override;
  virtual void UpdateLinkStatus() override;
  virtual void GetEthAddr(uint8_t *buffer);

private:
  static const uint32_t kVendorId = 0x10ee; /* Xilinx */
  static const uint32_t kTonic    = 0x6024; /* ML605 */

  // Memory Mapped I/O Base Address
  volatile uint32_t *_mmioAddr = nullptr;

  // MAC address
  uint8_t _ethAddr[6];

  int32_t Receive(uint8_t *buffer, uint32_t size);
  int32_t Transmit(const uint8_t *packet, uint32_t length);

  // software reset
  void Reset();
};

#endif /* __RAPH_KERNEL_DEV_TONIC_H__ */
