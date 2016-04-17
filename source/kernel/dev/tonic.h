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
#include <global.h>
#include <spinlock.h>
#include <polling.h>
#include <dev/eth.h>

struct TonicRxDesc {
  uint64_t base_address;  /* address of buffer */
  uint16_t packet_length; /* length */
  uint8_t  reserved[6];
} __attribute__ ((packed));

struct TonicTxDesc {
  uint64_t base_address;  /* address of buffer */
  uint16_t packet_length; /* length */
  uint8_t  reserved[6];
} __attribute__ ((packed));

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

  SpinLock _lock;

  // Memory Mapped I/O Base Address
  volatile uint32_t *_mmioAddr = nullptr;

  // Controller Register Space
  static const uint32_t kRegCtrl   = 0x00;
  static const uint32_t kRegHadr0  = 0x04;
  static const uint32_t kRegHadr1  = 0x08;
  static const uint32_t kRegPadr0  = 0x0c;
  static const uint32_t kRegPadr1  = 0x10;
  static const uint32_t kRegPadr2  = 0x14;
  static const uint32_t kRegPadr3  = 0x18;
  static const uint32_t kRegTdba   = 0x1c;
  static const uint32_t kRegTdh    = 0x24;
  static const uint32_t kRegTdt    = 0x26;
  static const uint32_t kRegTdlen  = 0x28;
  static const uint32_t kRegRdba   = 0x2c;
  static const uint32_t kRegRdh    = 0x34;
  static const uint32_t kRegRdt    = 0x36;
  static const uint32_t kRegRdlen  = 0x38;

  // packet buffer
  static const uint32_t kTxdescNumber = 8;
  static const uint32_t kRxdescNumber = 8;
  TonicTxDesc *_tx_desc_buf;
  TonicRxDesc *_rx_desc_buf;

  static const uint32_t kMaxFrameLength = 1518;

  // MAC address
  uint8_t _ethAddr[6];

  void SetupRx();
  void SetupTx();

  int32_t Receive(uint8_t *buffer, uint32_t size);
  int32_t Transmit(const uint8_t *packet, uint32_t length);

  template<class T>
    void WriteMmio(uint32_t offset, T data) {
    *(reinterpret_cast<volatile T*>(_mmioAddr) + offset / sizeof(T)) = data;
  }

  template<class T>
    T ReadMmio(uint32_t offset) {
    return *(reinterpret_cast<volatile T*>(_mmioAddr) + offset / sizeof(T));
  }

  // software reset
  void Reset();
};

#endif /* __RAPH_KERNEL_DEV_TONIC_H__ */
