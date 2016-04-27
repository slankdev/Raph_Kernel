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
 */

#include <mem/paging.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <dev/tonic.h>
#include <tty.h>
#include <global.h>

extern Tonic *tonic;

bool Tonic::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  if (vid == kVendorId && did == kTonic) {
    // allocate memory
    Tonic *addr = reinterpret_cast<Tonic*>(virtmem_ctrl->Alloc(sizeof(Tonic)));

    // create instance of Tonic
    Tonic *_tonic = new(addr) Tonic(bus, device, mf);
    _tonic->Setup();
    _tonic->SetupNetInterface();

    return true;
  } else {
    // no such device
    return false;
  }
}

void Tonic::GetEthAddr(uint8_t *buffer) {
  memcpy(buffer, _ethAddr, 6);
}

void Tonic::Handle() {
  uint8_t buf[kMaxFrameLength];
  int32_t len;

  if ((len = this->Receive(buf, kMaxFrameLength)) != -1) {
    Packet *packet;
    if (this->_rx_reserved.Pop(packet)) {
      memcpy(packet->buf, buf, len);
      packet->len = len;
      if (!this->_rx_buffered.Push(packet)) {
        kassert(this->_rx_reserved.Push(packet));
      }
    }
  }

  if (!this->_tx_buffered.IsEmpty()) {
    Packet *packet;
    kassert(this->_tx_buffered.Pop(packet));
    this->Transmit(packet->buf, packet->len);
    this->ReuseTxBuffer(packet);
  }
}

void Tonic::Setup() {
  // get PCI Base Address Registers
  phys_addr bar = this->ReadReg<uint32_t>(PCICtrl::kBaseAddressReg0);
  kassert((bar & 0xf) == 0);
  phys_addr mmio_addr = bar & 0xfffffff0;
  _mmioAddr = reinterpret_cast<uint32_t*>(p2v(mmio_addr));

  // Enable BusMaster
  this->WriteReg<uint16_t>(PCICtrl::kCommandReg, this->ReadReg<uint16_t>(PCICtrl::kCommandReg) | PCICtrl::kCommandRegBusMasterEnableFlag | (1 << 10));

  // TODO: disable
  WriteMmio<uint32_t>(kRegCtrl, 0);

  // fetch MAC address
  uint32_t hadr0 = ReadMmio<uint32_t>(kRegHadr0);
  uint32_t hadr1 = ReadMmio<uint32_t>(kRegHadr1);
  _ethAddr[0] = (hadr0) & 0xff;
  _ethAddr[1] = (hadr0 >> 8) & 0xff;
  _ethAddr[2] = (hadr0 >> 16) & 0xff;
  _ethAddr[3] = (hadr0 >> 24) & 0xff;
  _ethAddr[4] = (hadr1) & 0xff;
  _ethAddr[5] = (hadr1 >> 8) & 0xff;

  gtty->Printf("s", "[tonic] MAC address is ",
      "x", _ethAddr[0], "s", ":",
      "x", _ethAddr[1], "s", ":",
      "x", _ethAddr[2], "s", ":",
      "x", _ethAddr[3], "s", ":",
      "x", _ethAddr[4], "s", ":",
      "x", _ethAddr[5], "s", "\n");

  this->SetupTx();
  this->SetupRx();

  // N.B. IP address write address
  uint8_t ipaddr[4] = {192, 168, 100, 165};
  WriteMmio<uint32_t>(kRegPadr0, (ipaddr[0] << 24) | (ipaddr[1] << 16) | (ipaddr[2] << 8) | ipaddr[3]);
  gtty->Printf("s", "[tonic] IP address is ",
      "d", ipaddr[0], "s", ".",
      "d", ipaddr[1], "s", ".",
      "d", ipaddr[2], "s", ".",
      "d", ipaddr[3], "s", "\n");

  uint32_t ipaddr2 = ReadMmio<uint32_t>(kRegPadr0);
  gtty->Printf("s", "[tonic] ReadMmio(kRegPadr0) = ", "x", ipaddr2, "s", "\n");

  // TODO: enable
  WriteMmio<uint32_t>(kRegCtrl, 0xdeadbeefu);
}

void Tonic::SetupTx() {
  InitTxPacketBuffer();

  // set base address of ring buffer
  virt_addr tx_desc_buf_addr = ((virtmem_ctrl->Alloc(sizeof(TonicTxDesc) * kTxdescNumber + 15) + 15) / 16) * 16;
  _tx_desc_buf = reinterpret_cast<TonicTxDesc*>(tx_desc_buf_addr);

  // Warning: root complex discard 64bit-width data, so you must split
  //          64bit data to 32bit
  WriteMmio<uint32_t>(kRegTdba, k2p(tx_desc_buf_addr) & 0xffffffff);
  WriteMmio<uint32_t>(kRegTdba + 4, (k2p(tx_desc_buf_addr) >> 32));

  // N.B. debug
  gtty->Printf("s", "[tonic] TDBA = 0x", "x", reinterpret_cast<uint64_t>(k2p(tx_desc_buf_addr)), "s", "\n");

  // set head and tail pointer of ring
  WriteMmio<uint16_t>(kRegTdh, 0);
  WriteMmio<uint16_t>(kRegTdt, 0);
  
  // set the size of the desc ring
  WriteMmio<uint16_t>(kRegTdlen, kTxdescNumber);

  // initialize rx desc ring buffer
  for(uint32_t i = 0; i < kTxdescNumber; i++) {
    TonicTxDesc *txdesc = &_tx_desc_buf[i];
    txdesc->base_address = k2p(virtmem_ctrl->Alloc(kMaxFrameLength));
    txdesc->packet_length = 0;
  }
}

void Tonic::SetupRx() {
  InitRxPacketBuffer();

  // set base address of ring buffer
  PhysAddr paddr;
  physmem_ctrl->Alloc(paddr, PagingCtrl::ConvertNumToPageSize(sizeof(TonicRxDesc) * kRxdescNumber));
  phys_addr rx_desc_buf_paddr = paddr.GetAddr();
  virt_addr rx_desc_buf_vaddr = p2v(rx_desc_buf_paddr);
  _rx_desc_buf = reinterpret_cast<TonicRxDesc*>(rx_desc_buf_vaddr);

  // Warning: root complex discard 64bit-width data, so you must split
  //          64bit data to 32bit
  WriteMmio<uint32_t>(kRegRdba, rx_desc_buf_paddr & 0xffffffff);
  WriteMmio<uint32_t>(kRegRdba + 4, rx_desc_buf_paddr >> 32);

  gtty->Printf("s", "[tonic] RDBA = 0x", "x", reinterpret_cast<uint64_t>(rx_desc_buf_paddr), "s", "\n");

  // set head and tail pointer of ring
  WriteMmio<uint16_t>(kRegRdh, 0);
  WriteMmio<uint16_t>(kRegRdt, 0);

  // set the size of the desc ring
  WriteMmio<uint16_t>(kRegRdlen, kRxdescNumber);

  // initialize rx desc ring buffer
  for(uint32_t i = 0; i < kRxdescNumber; i++) {
    TonicRxDesc *rxdesc = &_rx_desc_buf[i];
    rxdesc->base_address = k2p(virtmem_ctrl->Alloc(kMaxFrameLength));
    rxdesc->packet_length = 0;
    gtty->Printf("s", "[tonic] rxdesc[", "d", i, "s", "].base_address = 0x", "x", rxdesc->base_address,
        "s", "; rxdesc = 0x", "x", v2p(reinterpret_cast<virt_addr>(rxdesc)), "s", "\n");
  }
}

void Tonic::SetupNetInterface() {
  netdev_ctrl->RegisterDevice(this);
  RegisterPolling();

  // N.B. temporary
  tonic = this;
}

void Tonic::UpdateLinkStatus() {
  // do nothing
}

int32_t Tonic::Receive(uint8_t *buffer, uint32_t size) {
  TonicRxDesc *rxdesc;
  uint32_t rdh = ReadMmio<uint16_t>(kRegRdh);
  uint32_t rdt = ReadMmio<uint16_t>(kRegRdt);
  uint32_t length;
  int rx_available = (kRxdescNumber - rdt + rdh) % kRxdescNumber;

  if(rx_available > 0) {

    // if the packet is on the wire
    rxdesc = _rx_desc_buf + (rdt % kRxdescNumber);
    length = size < rxdesc->packet_length ? size : rxdesc->packet_length;
    memcpy(buffer, reinterpret_cast<uint8_t*>(p2v(rxdesc->base_address)), length);
    WriteMmio<uint16_t>(kRegRdt, (rdt + 1) % kRxdescNumber);

    // N.B. test
    gtty->Printf("s", "[tonic] rx; length = ", "d", rxdesc->packet_length, "s", ";\n");
    gtty->Printf("x", buffer[0], "x", buffer[1], "s", " ",
                 "x", buffer[2], "x", buffer[3], "s", " ",
                 "x", buffer[4], "x", buffer[5], "s", " ",
                 "x", buffer[6], "x", buffer[7], "s", " ",
                 "x", buffer[8], "x", buffer[9], "s", " ",
                 "x", buffer[10], "x", buffer[11], "s", " ",
                 "x", buffer[12], "x", buffer[13], "s", " ",
                 "x", buffer[14], "x", buffer[15], "s", " ",
                 "x", buffer[16], "x", buffer[17], "s", " ",
                 "x", buffer[18], "x", buffer[19], "s", "\n");
//    for(uint32_t i = 0; i < length; i++) {
//      if(buffer[i] < 0x10) gtty->Printf("d", 0);
//      gtty->Printf("x", buffer[i]);
//      if((i+1) % 16 == 0) gtty->Printf("s", "\n");
//      else if((i+1) % 16 == 8) gtty->Printf("s", ":");
//      else gtty->Printf("s", " ");
//    }
//    gtty->Printf("s", "\n");
    // ^ test end

    return length;
  } else {
    return -1;
  }
}

int32_t Tonic::Transmit(const uint8_t *packet, uint32_t length) {
  TonicTxDesc *txdesc;
  uint32_t tdh = ReadMmio<uint16_t>(kRegTdh);
  uint32_t tdt = ReadMmio<uint16_t>(kRegTdt);
  int tx_available = kTxdescNumber - ((kTxdescNumber - tdh + tdt) % kTxdescNumber);

  if(tx_available > 0) {
    // if _tx_desc_buf is not full
    txdesc = _tx_desc_buf + (tdt % kTxdescNumber);
    memcpy(reinterpret_cast<uint8_t*>(p2v(txdesc->base_address)), packet, length);
    txdesc->packet_length = length;
    WriteMmio<uint16_t>(kRegTdh, (tdh + 1) % kTxdescNumber);
    return length;
  } else {
    return -1;
  }
}

void Tonic::Reset() {
  // software reset
  // TODO: write me
}
