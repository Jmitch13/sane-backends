/*  sane - Scanner Access Now Easy.

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

#define DEBUG_DECLARE_ONLY

#include "genesys_low.h"

StaticInit<std::vector<Genesys_Gpo>> s_gpo;

void genesys_init_gpo_tables()
{
    s_gpo.init();

    Genesys_Gpo gpo;
    gpo.gpo_id = GPO_UMAX;
    gpo.regs = {
        { 0x66, 0x11 },
        { 0x67, 0x00 },
        { 0x68, 0x51 },
        { 0x69, 0x20 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_ST12;
    gpo.regs = {
        { 0x66, 0x11 },
        { 0x67, 0x00 },
        { 0x68, 0x51 },
        { 0x69, 0x20 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_ST24;
    gpo.regs = {
        { 0x66, 0x00 },
        { 0x67, 0x00 },
        { 0x68, 0x51 },
        { 0x69, 0x20 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_5345; // bits 11-12 are for bipolar V-ref input voltage
    gpo.regs = {
        { 0x66, 0x30 },
        { 0x67, 0x18 },
        { 0x68, 0xa0 },
        { 0x69, 0x18 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_HP2400;
    gpo.regs = {
        { 0x66, 0x30 },
        { 0x67, 0x00 },
        { 0x68, 0x31 },
        { 0x69, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_HP2300;
    gpo.regs = {
        { 0x66, 0x00 },
        { 0x67, 0x00 },
        { 0x68, 0x00 },
        { 0x69, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CANONLIDE35;
    gpo.regs = {
        { 0x6c, 0x02 },
        { 0x6d, 0x80 },
        { 0x6e, 0xef },
        { 0x6f, 0x80 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_XP200;
    gpo.regs = {
        { 0x66, 0x30 },
        { 0x67, 0x00 },
        { 0x68, 0xb0 },
        { 0x69, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_HP3670;
    gpo.regs = {
        { 0x66, 0x00 },
        { 0x67, 0x00 },
        { 0x68, 0x00 },
        { 0x69, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_XP300;
    gpo.regs = {
        { 0x6c, 0x09 },
        { 0x6d, 0xc6 },
        { 0x6e, 0xbb },
        { 0x6f, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_DP665;
    gpo.regs = {
        { 0x6c, 0x18 },
        { 0x6d, 0x00 },
        { 0x6e, 0xbb },
        { 0x6f, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_DP685;
    gpo.regs = {
        { 0x6c, 0x3f },
        { 0x6d, 0x46 },
        { 0x6e, 0xfb },
        { 0x6f, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CANONLIDE200;
    gpo.regs = {
        { 0x6c, 0xfb }, // 0xfb when idle , 0xf9/0xe9 (1200) when scanning
        { 0x6d, 0x20 },
        { 0x6e, 0xff },
        { 0x6f, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CANONLIDE700;
    gpo.regs = {
        { 0x6c, 0xdb },
        { 0x6d, 0xff },
        { 0x6e, 0xff },
        { 0x6f, 0x80 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_KVSS080;
    gpo.regs = {
        { 0x6c, 0xf5 },
        { 0x6d, 0x20 },
        { 0x6e, 0x7e },
        { 0x6f, 0xa1 },
        { 0xa6, 0x06 },
        { 0xa7, 0x0f },
        { 0xa8, 0x00 },
        { 0xa9, 0x08 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_G4050;
    gpo.regs = {
        { 0x6c, 0x20 },
        { 0x6d, 0x00 },
        { 0x6e, 0xfc },
        { 0x6f, 0x00 },
        { 0xa6, 0x08 },
        { 0xa7, 0x1e },
        { 0xa8, 0x3e },
        { 0xa9, 0x06 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_HP_N6310;
    gpo.regs = {
        { 0x6c, 0xa3 },
        { 0x6d, 0x00 },
        { 0x6e, 0x7f },
        { 0x6f, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CANONLIDE110;
    gpo.regs = {
        { 0x6c, 0xfb },
        { 0x6d, 0x20 },
        { 0x6e, 0xff },
        { 0x6f, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CANONLIDE120;
    gpo.regs = {
        { 0x6c, 0xfb },
        { 0x6d, 0x20 },
        { 0x6e, 0xff },
        { 0x6f, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CANONLIDE210;
    gpo.regs = {
        { 0x6c, 0xfb },
        { 0x6d, 0x20 },
        { 0x6e, 0xff },
        { 0x6f, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_PLUSTEK_3600;
    gpo.regs = {
        { 0x6c, 0x02 },
        { 0x6d, 0x00 },
        { 0x6e, 0x1e },
        { 0x6f, 0x80 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CS4400F;
    gpo.regs = {
        { 0x6c, 0x01 },
        { 0x6d, 0x7f },
        { 0x6e, 0xff },
        { 0x6f, 0x00 },
        { 0xa6, 0x00 },
        { 0xa7, 0xff },
        { 0xa8, 0x07 },
        { 0xa9, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CS8400F;
    gpo.regs = {
        { 0x6c, 0x9a },
        { 0x6d, 0xdf },
        { 0x6e, 0xfe },
        { 0x6f, 0x60 },
        { 0xa6, 0x00 },
        { 0xa7, 0x03 },
        { 0xa8, 0x00 },
        { 0xa9, 0x02 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CS8600F;
    gpo.regs = {
        { 0x6c, 0x20 },
        { 0x6d, 0x7c },
        { 0x6e, 0xff },
        { 0x6f, 0x00 },
        { 0xa6, 0x00 },
        { 0xa7, 0xff },
        { 0xa8, 0x00 },
        { 0xa9, 0x00 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_IMG101;
    gpo.regs = {
        { 0x6c, 0x41 },
        { 0x6d, 0xa4 },
        { 0x6e, 0x13 },
        { 0x6f, 0xa7 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_PLUSTEK3800;
    gpo.regs = {
        { 0x6c, 0x41 },
        { 0x6d, 0xa4 },
        { 0x6e, 0x13 },
        { 0x6f, 0xa7 },
    };
    s_gpo->push_back(gpo);


    gpo = Genesys_Gpo();
    gpo.gpo_id = GPO_CANONLIDE80;
    gpo.regs = {
        { 0x6c, 0x28 },
        { 0x6d, 0x90 },
        { 0x6e, 0x75 },
        { 0x6f, 0x80 },
    };
    s_gpo->push_back(gpo);
}
