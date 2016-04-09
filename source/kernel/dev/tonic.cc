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

#include <dev/tonic.h>
#include <tty.h>
#include <global.h>

bool Tonic::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  if (vid == kVendorId && did == kTonic) {
    // allocate memory
    Tonic *addr = reinterpret_cast<Tonic*>(virtmem_ctrl->Alloc(sizeof(Tonic)));

    // create instance of Tonic
    Tonic *tonic = new(addr) Tonic(bus, device, mf);
    tonic->Setup();

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
  // polling task
  // TODO: write me
}

void Tonic::Setup() {
  // TODO:
  // fetch MAC address

  // init tx packet buffer

  // init rx packet buffer

}

void Tonic::SetupNetInterface() {
  netdev_ctrl->RegisterDevice(this);
  RegisterPolling();
}

void Tonic::UpdateLinkStatus() {
  // do nothing
}

int32_t Tonic::Receive(uint8_t *buffer, uint32_t size) {
  // TODO: write me
  return -1;
}

int32_t Tonic::Transmit(const uint8_t *packet, uint32_t length) {
  // TODO: write me
  return -1;
}

void Tonic::Reset() {
  // software reset
  // TODO: write me
}
