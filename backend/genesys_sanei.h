/* sane - Scanner Access Now Easy.

   Copyright (C) 2019 Povilas Kanapickas <povilas@radix.lt>

   This file is part of the SANE package.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

   As a special exception, the authors of SANE give permission for
   additional uses of the libraries contained in this release of SANE.

   The exception is that, if you link a SANE library with other files
   to produce an executable, this does not by itself cause the
   resulting executable to be covered by the GNU General Public
   License.  Your use of that executable is in no way restricted on
   account of linking the SANE library code into it.

   This exception does not, however, invalidate any other reasons why
   the executable file might be covered by the GNU General Public
   License.

   If you submit changes to SANE to the maintainers to be included in
   a subsequent release, you agree by submitting the changes that
   those changes may be distributed with this exception intact.

   If you write modifications of your own for SANE, it is your choice
   whether to permit this exception to apply to your modifications.
   If you do not wish that, delete this exception notice.
*/

#ifndef BACKEND_GENESYS_SANEI_H
#define BACKEND_GENESYS_SANEI_H

#include "genesys_error.h"
#include "../include/sane/sanei_usb.h"

#include <cstdint>
#include <string>

class UsbDevice {
public:
    UsbDevice() = default;
    UsbDevice(const UsbDevice& other) = delete;
    UsbDevice& operator=(const UsbDevice&) = delete;

    UsbDevice(UsbDevice&& other) :
        name_(other.name_),
        is_open_(other.is_open_),
        device_num_(other.device_num_)
    {
        other.set_not_open();
    }

    ~UsbDevice();

    bool is_open() const { return is_open_; }

    int device_number() const { return device_num_; }

    const std::string& name() const { return name_; }

    void open(const char* dev_name);

    void clear_halt();
    void reset();
    void close();

    void get_vendor_product(int& vendor, int& product);

    void control_msg(int rtype, int reg, int value, int index, int length, uint8_t* data);
    void bulk_read(uint8_t* buffer, size_t* size);
    void bulk_write(const uint8_t* buffer, size_t* size);

private:

    void assert_is_open() const;
    void set_not_open();

    std::string name_;
    bool is_open_ = false;
    int device_num_ = 0;
};

void sanei_usb_testing_record_message(SANE_String_Const message);

#endif // BACKEND_GENESYS_SANEI_H
