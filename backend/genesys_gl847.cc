/* sane - Scanner Access Now Easy.

   Copyright (C) 2010-2013 Stéphane Voltz <stef.dev@free.fr>


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

#include "genesys_gl847.h"

#include <vector>

/****************************************************************************
 Mid level functions
 ****************************************************************************/

static SANE_Bool
gl847_get_fast_feed_bit (Genesys_Register_Set * regs)
{
  GenesysRegister *r = NULL;

  r = sanei_genesys_get_address (regs, REG02);
  if (r && (r->value & REG02_FASTFED))
    return SANE_TRUE;
  return SANE_FALSE;
}

static SANE_Bool
gl847_get_filter_bit (Genesys_Register_Set * regs)
{
  GenesysRegister *r = NULL;

  r = sanei_genesys_get_address (regs, REG04);
  if (r && (r->value & REG04_FILTER))
    return SANE_TRUE;
  return SANE_FALSE;
}

static SANE_Bool
gl847_get_lineart_bit (Genesys_Register_Set * regs)
{
  GenesysRegister *r = NULL;

  r = sanei_genesys_get_address (regs, REG04);
  if (r && (r->value & REG04_LINEART))
    return SANE_TRUE;
  return SANE_FALSE;
}

static SANE_Bool
gl847_get_bitset_bit (Genesys_Register_Set * regs)
{
  GenesysRegister *r = NULL;

  r = sanei_genesys_get_address (regs, REG04);
  if (r && (r->value & REG04_BITSET))
    return SANE_TRUE;
  return SANE_FALSE;
}

static SANE_Bool
gl847_get_gain4_bit (Genesys_Register_Set * regs)
{
  GenesysRegister *r = NULL;

  r = sanei_genesys_get_address (regs, 0x06);
  if (r && (r->value & REG06_GAIN4))
    return SANE_TRUE;
  return SANE_FALSE;
}

static SANE_Bool
gl847_test_buffer_empty_bit (SANE_Byte val)
{
  if (val & REG41_BUFEMPTY)
    return SANE_TRUE;
  return SANE_FALSE;
}

static SANE_Bool
gl847_test_motor_flag_bit (SANE_Byte val)
{
  if (val & REG41_MOTORENB)
    return SANE_TRUE;
  return SANE_FALSE;
}

/**
 * compute the step multiplier used
 */
static int
gl847_get_step_multiplier (Genesys_Register_Set * regs)
{
  GenesysRegister *r = NULL;
  int value = 1;

  r = sanei_genesys_get_address (regs, 0x9d);
  if (r != NULL)
    {
      value = (r->value & 0x0f)>>1;
      value = 1 << value;
    }
  DBG (DBG_io, "%s: step multiplier is %d\n", __func__, value);
  return value;
}

/** @brief sensor specific settings
*/
static void gl847_setup_sensor(Genesys_Device * dev, const Genesys_Sensor& sensor,
                               const SensorProfile& sensor_profile, Genesys_Register_Set* regs)
{
    DBG_HELPER(dbg);
  uint16_t exp;

    for (uint16_t addr = 0x16; addr < 0x1e; addr++) {
        regs->set8(addr, sensor.custom_regs.get_value(addr));
    }

    for (uint16_t addr = 0x52; addr < 0x52 + 9; addr++) {
        regs->set8(addr, sensor.custom_regs.get_value(addr));
    }

    for (const auto& reg : sensor_profile.custom_regs) {
        regs->set8(reg.address, reg.value);
    }

  /* if no calibration has been done, set default values for exposures */
  exp = sensor.exposure.red;
    if (exp == 0) {
        exp = sensor_profile.exposure.red;
    }
    regs->set16(REG_EXPR, exp);

  exp = sensor.exposure.green;
    if (exp == 0) {
        exp = sensor_profile.exposure.green;
    }
    regs->set16(REG_EXPG, exp);

  exp = sensor.exposure.blue;
    if (exp == 0) {
        exp = sensor_profile.exposure.blue;
    }
    regs->set16(REG_EXPB, exp);

    dev->segment_order = sensor_profile.segment_order;
}


/** @brief set all registers to default values .
 * This function is called only once at the beginning and
 * fills register startup values for registers reused across scans.
 * Those that are rarely modified or not modified are written
 * individually.
 * @param dev device structure holding register set to initialize
 */
static void
gl847_init_registers (Genesys_Device * dev)
{
    DBG_HELPER(dbg);
  int lide700=0;
  uint8_t val;

  /* 700F class needs some different initial settings */
  if (dev->model->model_id == MODEL_CANON_LIDE_700F)
    {
       lide700 = 1;
    }

  dev->reg.clear();

  SETREG (0x01, 0x82);
  SETREG (0x02, 0x18);
  SETREG (0x03, 0x50);
  SETREG (0x04, 0x12);
  SETREG (0x05, 0x80);
  SETREG (0x06, 0x50);		/* FASTMODE + POWERBIT */
  SETREG (0x08, 0x10);
  SETREG (0x09, 0x01);
  SETREG (0x0a, 0x00);
  SETREG (0x0b, 0x01);
  SETREG (0x0c, 0x02);

  /* LED exposures */
  SETREG (0x10, 0x00);
  SETREG (0x11, 0x00);
  SETREG (0x12, 0x00);
  SETREG (0x13, 0x00);
  SETREG (0x14, 0x00);
  SETREG (0x15, 0x00);

  SETREG (0x16, 0x10);
  SETREG (0x17, 0x08);
  SETREG (0x18, 0x00);

  /* EXPDMY */
  SETREG (0x19, 0x50);

  SETREG (0x1a, 0x34);
  SETREG (0x1b, 0x00);
  SETREG (0x1c, 0x02);
  SETREG (0x1d, 0x04);
  SETREG (0x1e, 0x10);
  SETREG (0x1f, 0x04);
  SETREG (0x20, 0x02);
  SETREG (0x21, 0x10);
  SETREG (0x22, 0x7f);
  SETREG (0x23, 0x7f);
  SETREG (0x24, 0x10);
  SETREG (0x25, 0x00);
  SETREG (0x26, 0x00);
  SETREG (0x27, 0x00);
  SETREG (0x2c, 0x09);
  SETREG (0x2d, 0x60);
  SETREG (0x2e, 0x80);
  SETREG (0x2f, 0x80);
  SETREG (0x30, 0x00);
  SETREG (0x31, 0x10);
  SETREG (0x32, 0x15);
  SETREG (0x33, 0x0e);
  SETREG (0x34, 0x40);
  SETREG (0x35, 0x00);
  SETREG (0x36, 0x2a);
  SETREG (0x37, 0x30);
  SETREG (0x38, 0x2a);
  SETREG (0x39, 0xf8);
  SETREG (0x3d, 0x00);
  SETREG (0x3e, 0x00);
  SETREG (0x3f, 0x00);
  SETREG (0x52, 0x03);
  SETREG (0x53, 0x07);
  SETREG (0x54, 0x00);
  SETREG (0x55, 0x00);
  SETREG (0x56, 0x00);
  SETREG (0x57, 0x00);
  SETREG (0x58, 0x2a);
  SETREG (0x59, 0xe1);
  SETREG (0x5a, 0x55);
  SETREG (0x5e, 0x41);
  SETREG (0x5f, 0x40);
  SETREG (0x60, 0x00);
  SETREG (0x61, 0x21);
  SETREG (0x62, 0x40);
  SETREG (0x63, 0x00);
  SETREG (0x64, 0x21);
  SETREG (0x65, 0x40);
  SETREG (0x67, 0x80);
  SETREG (0x68, 0x80);
  SETREG (0x69, 0x20);
  SETREG (0x6a, 0x20);

  /* CK1MAP */
  SETREG (0x74, 0x00);
  SETREG (0x75, 0x00);
  SETREG (0x76, 0x3c);

  /* CK3MAP */
  SETREG (0x77, 0x00);
  SETREG (0x78, 0x00);
  SETREG (0x79, 0x9f);

  /* CK4MAP */
  SETREG (0x7a, 0x00);
  SETREG (0x7b, 0x00);
  SETREG (0x7c, 0x55);

  SETREG (0x7d, 0x00);

  /* NOTE: autoconf is a non working option */
  SETREG (0x87, 0x02);
  SETREG (0x9d, 0x06);
  SETREG (0xa2, 0x0f);
  SETREG (0xbd, 0x18);
  SETREG (0xfe, 0x08);

  /* gamma[0] and gamma[256] values */
  SETREG (0xbe, 0x00);
  SETREG (0xc5, 0x00);
  SETREG (0xc6, 0x00);
  SETREG (0xc7, 0x00);
  SETREG (0xc8, 0x00);
  SETREG (0xc9, 0x00);
  SETREG (0xca, 0x00);

  /* LiDE 700 fixups */
  if(lide700)
    {
      SETREG (0x5f, 0x04);
      SETREG (0x7d, 0x80);

      /* we write to these registers only once */
      val=0;
      dev->write_register(REG7E, val);
      dev->write_register(REG9E, val);
      dev->write_register(REG9F, val);
      dev->write_register(REGAB, val);
    }

    const auto& sensor = sanei_genesys_find_sensor_any(dev);
    sanei_genesys_set_dpihw(dev->reg, sensor, sensor.optical_res);

  /* initalize calibration reg */
  dev->calib_reg = dev->reg;
}

/**@brief send slope table for motor movement
 * Send slope_table in machine byte order
 * @param dev device to send slope table
 * @param table_nr index of the slope table in ASIC memory
 * Must be in the [0-4] range.
 * @param slope_table pointer to 16 bit values array of the slope table
 * @param steps number of elements in the slope table
 */
static void gl847_send_slope_table(Genesys_Device* dev, int table_nr,
                                   const std::vector<uint16_t>& slope_table,
                                   int steps)
{
    DBG_HELPER_ARGS(dbg, "table_nr = %d, steps = %d", table_nr, steps);
  int i;
  char msg[10000];

  /* sanity check */
  if(table_nr<0 || table_nr>4)
    {
        throw SaneException("invalid table number %d", table_nr);
    }

  std::vector<uint8_t> table(steps * 2);
  for (i = 0; i < steps; i++)
    {
      table[i * 2] = slope_table[i] & 0xff;
      table[i * 2 + 1] = slope_table[i] >> 8;
    }

  if (DBG_LEVEL >= DBG_io)
    {
      sprintf (msg, "write slope %d (%d)=", table_nr, steps);
      for (i = 0; i < steps; i++)
	{
	  sprintf (msg+strlen(msg), "%d", slope_table[i]);
	}
      DBG (DBG_io, "%s: %s\n", __func__, msg);
    }

    // slope table addresses are fixed
    sanei_genesys_write_ahb(dev, 0x10000000 + 0x4000 * table_nr, steps * 2, table.data());
}

/**
 * Set register values of Analog Device type frontend
 * */
static void gl847_set_ad_fe(Genesys_Device* dev, uint8_t set)
{
    DBG_HELPER(dbg);
  int i;
  uint8_t val8;

    // wait for FE to be ready
    sanei_genesys_get_status(dev, &val8);
  while (val8 & REG41_FEBUSY)
    {
      sanei_genesys_sleep_ms(10);
        sanei_genesys_get_status(dev, &val8);
    };

  if (set == AFE_INIT)
    {
      DBG(DBG_proc, "%s(): setting DAC %u\n", __func__, dev->model->dac_type);

      dev->frontend = dev->frontend_initial;
    }

    // reset DAC
    sanei_genesys_fe_write_data(dev, 0x00, 0x80);

    // write them to analog frontend
    sanei_genesys_fe_write_data(dev, 0x00, dev->frontend.regs.get_value(0x00));

    sanei_genesys_fe_write_data(dev, 0x01, dev->frontend.regs.get_value(0x01));

    for (i = 0; i < 3; i++) {
        sanei_genesys_fe_write_data(dev, 0x02 + i, dev->frontend.get_gain(i));
    }
    for (i = 0; i < 3; i++) {
        sanei_genesys_fe_write_data(dev, 0x05 + i, dev->frontend.get_offset(i));
    }
}

static void gl847_homsnr_gpio(Genesys_Device* dev)
{
    DBG_HELPER(dbg);
    uint8_t val;

    if (dev->model->gpo_type == GPO_CANONLIDE700) {
        val = dev->read_register(REG6C);
        val &= ~REG6C_GPIO10;
        dev->write_register(REG6C, val);
    } else {
        val = dev->read_register(REG6C);
        val |= REG6C_GPIO10;
        dev->write_register(REG6C, val);
    }
}

// Set values of analog frontend
static void gl847_set_fe(Genesys_Device* dev, const Genesys_Sensor& sensor, uint8_t set)
{
    DBG_HELPER_ARGS(dbg, "%s", set == AFE_INIT ? "init" :
                               set == AFE_SET ? "set" :
                               set == AFE_POWER_SAVE ? "powersave" : "huh?");

    (void) sensor;

    uint8_t val = dev->read_register(REG04);
    uint8_t frontend_type = val & REG04_FESET;

    // route to AD devices
    if (frontend_type == 0x02) {
        gl847_set_ad_fe(dev, set);
        return;
    }

    throw SaneException("unsupported frontend type %d", frontend_type);
}


// @brief set up motor related register for scan
static void gl847_init_motor_regs_scan(Genesys_Device* dev,
                                       const Genesys_Sensor& sensor,
                                       Genesys_Register_Set* reg,
                                       unsigned int scan_exposure_time,
                                       float scan_yres,
                                       int scan_step_type,
                                       unsigned int scan_lines,
                                       unsigned int scan_dummy,
                                       unsigned int feed_steps,
                                       unsigned int flags)
{
    DBG_HELPER_ARGS(dbg, "scan_exposure_time=%d, can_yres=%g, scan_step_type=%d, scan_lines=%d, "
                         "scan_dummy=%d, feed_steps=%d, flags=%x",
                    scan_exposure_time, scan_yres, scan_step_type, scan_lines, scan_dummy,
                    feed_steps, flags);
  int use_fast_fed;
  unsigned int fast_dpi;
    std::vector<uint16_t> scan_table;
    std::vector<uint16_t> fast_table;
  int scan_steps, fast_steps, factor;
  unsigned int feedl, dist;
  GenesysRegister *r;
  uint32_t z1, z2;
  unsigned int min_restep = 0x20;
    uint8_t val;
  int fast_step_type;
  unsigned int ccdlmt,tgtime;

  /* get step multiplier */
  factor = gl847_get_step_multiplier (reg);

  use_fast_fed=0;
  /* no fast fed since feed works well */
  if(dev->settings.yres==4444 && feed_steps>100
     && ((flags & MOTOR_FLAG_FEED)==0))
    {
      use_fast_fed=1;
    }
  DBG(DBG_io, "%s: use_fast_fed=%d\n", __func__, use_fast_fed);

    reg->set24(REG_LINCNT, scan_lines);
  DBG(DBG_io, "%s: lincnt=%d\n", __func__, scan_lines);

  /* compute register 02 value */
  r = sanei_genesys_get_address (reg, REG02);
  r->value = 0x00;
  sanei_genesys_set_motor_power(*reg, true);

  if (use_fast_fed)
    r->value |= REG02_FASTFED;
  else
    r->value &= ~REG02_FASTFED;

  if (flags & MOTOR_FLAG_AUTO_GO_HOME)
    r->value |= REG02_AGOHOME | REG02_NOTHOME;

  if ((flags & MOTOR_FLAG_DISABLE_BUFFER_FULL_MOVE)
      ||(scan_yres>=sensor.optical_res))
    {
      r->value |= REG02_ACDCDIS;
    }

  /* scan and backtracking slope table */
  sanei_genesys_slope_table(scan_table,
                            &scan_steps,
                            scan_yres,
                            scan_exposure_time,
                            dev->motor.base_ydpi,
                            scan_step_type,
                            factor,
                            dev->model->motor_type,
                            gl847_motor_profiles);
    gl847_send_slope_table(dev, SCAN_TABLE, scan_table, scan_steps * factor);
    gl847_send_slope_table(dev, BACKTRACK_TABLE, scan_table, scan_steps * factor);

  /* fast table */
  fast_dpi=sanei_genesys_get_lowest_ydpi(dev);
  fast_step_type=scan_step_type;
  if(scan_step_type>=2)
    {
      fast_step_type=2;
    }

  sanei_genesys_slope_table(fast_table,
                            &fast_steps,
                            fast_dpi,
                            scan_exposure_time,
                            dev->motor.base_ydpi,
                            fast_step_type,
                            factor,
                            dev->model->motor_type,
                            gl847_motor_profiles);

  /* manual override of high start value */
  fast_table[0]=fast_table[1];

    gl847_send_slope_table(dev, STOP_TABLE, fast_table, fast_steps * factor);
    gl847_send_slope_table(dev, FAST_TABLE, fast_table, fast_steps * factor);
    gl847_send_slope_table(dev, HOME_TABLE, fast_table, fast_steps * factor);

  /* correct move distance by acceleration and deceleration amounts */
  feedl=feed_steps;
  if (use_fast_fed)
    {
        feedl<<=fast_step_type;
        dist=(scan_steps+2*fast_steps)*factor;
        /* TODO read and decode REGAB */
        r = sanei_genesys_get_address (reg, 0x5e);
        dist += (r->value & 31);
        /* FEDCNT */
        r = sanei_genesys_get_address (reg, REG_FEDCNT);
        dist += r->value;
    }
  else
    {
      feedl<<=scan_step_type;
      dist=scan_steps*factor;
      if (flags & MOTOR_FLAG_FEED)
        dist *=2;
    }
  DBG(DBG_io2, "%s: scan steps=%d\n", __func__, scan_steps);
  DBG(DBG_io2, "%s: acceleration distance=%d\n", __func__, dist);

  /* check for overflow */
    if (dist < feedl) {
        feedl -= dist;
    } else {
        feedl = 0;
    }

    reg->set24(REG_FEEDL, feedl);
  DBG(DBG_io ,"%s: feedl=%d\n", __func__, feedl);

  r = sanei_genesys_get_address (reg, REG0C);
  ccdlmt=(r->value & REG0C_CCDLMT)+1;

  r = sanei_genesys_get_address (reg, REG1C);
  tgtime=1<<(r->value & REG1C_TGTIME);

    // hi res motor speed GPIO
    uint8_t effective = dev->read_register(REG6C);

  /* if quarter step, bipolar Vref2 */
  if (scan_step_type > 1)
    {
      if (scan_step_type < 3)
        {
          val = effective & ~REG6C_GPIO13;
        }
      else
        {
          val = effective | REG6C_GPIO13;
        }
    }
  else
    {
      val = effective;
    }
    dev->write_register(REG6C, val);

    // effective scan
    effective = dev->read_register(REG6C);
    val = effective | REG6C_GPIO10;
    dev->write_register(REG6C, val);

  min_restep=scan_steps/2-1;
  if (min_restep < 1)
    min_restep = 1;
  r = sanei_genesys_get_address (reg, REG_FWDSTEP);
  r->value = min_restep;
  r = sanei_genesys_get_address (reg, REG_BWDSTEP);
  r->value = min_restep;

  sanei_genesys_calculate_zmod(use_fast_fed,
			         scan_exposure_time*ccdlmt*tgtime,
				 scan_table,
				 scan_steps*factor,
				 feedl,
                                 min_restep*factor,
                                 &z1,
                                 &z2);

  DBG(DBG_info, "%s: z1 = %d\n", __func__, z1);
    reg->set24(REG60, z1 | (scan_step_type << (16+REG60S_STEPSEL)));

  DBG(DBG_info, "%s: z2 = %d\n", __func__, z2);
    reg->set24(REG63, z2 | (scan_step_type << (16+REG63S_FSTPSEL)));

  r = sanei_genesys_get_address (reg, 0x1e);
  r->value &= 0xf0;		/* 0 dummy lines */
  r->value |= scan_dummy;	/* dummy lines */

  r = sanei_genesys_get_address (reg, REG67);
  r->value = REG67_MTRPWM;

  r = sanei_genesys_get_address (reg, REG68);
  r->value = REG68_FASTPWM;

  r = sanei_genesys_get_address (reg, REG_STEPNO);
  r->value = scan_steps;

  r = sanei_genesys_get_address (reg, REG_FASTNO);
  r->value = scan_steps;

  r = sanei_genesys_get_address (reg, REG_FSHDEC);
  r->value = scan_steps;

  r = sanei_genesys_get_address (reg, REG_FMOVNO);
  r->value = fast_steps;

  r = sanei_genesys_get_address (reg, REG_FMOVDEC);
  r->value = fast_steps;
}


/** @brief set up registers related to sensor
 * Set up the following registers
   0x01
   0x03
   0x10-0x015     R/G/B exposures
   0x19           EXPDMY
   0x2e           BWHI
   0x2f           BWLO
   0x04
   0x87
   0x05
   0x2c,0x2d      DPISET
   0x30,0x31      STRPIXEL
   0x32,0x33      ENDPIXEL
   0x35,0x36,0x37 MAXWD [25:2] (>>2)
   0x38,0x39      LPERIOD
   0x34           DUMMY
 */
static void gl847_init_optical_regs_scan(Genesys_Device* dev, const Genesys_Sensor& sensor,
                                         Genesys_Register_Set* reg, unsigned int exposure_time,
                                         const ScanSession& session)
{
    DBG_HELPER_ARGS(dbg, "exposure_time=%d", exposure_time);
    unsigned dpihw;
  GenesysRegister *r;

    // resolution is divided according to ccd_pixels_per_system_pixel()
    unsigned ccd_pixels_per_system_pixel = sensor.ccd_pixels_per_system_pixel();
    DBG(DBG_io2, "%s: ccd_pixels_per_system_pixel=%d\n", __func__, ccd_pixels_per_system_pixel);

    // to manage high resolution device while keeping good low resolution scanning speed, we make
    // hardware dpi vary
    dpihw = sensor.get_register_hwdpi(session.params.xres * ccd_pixels_per_system_pixel);
  DBG(DBG_io2, "%s: dpihw=%d\n", __func__, dpihw);

    // sensor parameters
    const auto& sensor_profile = get_sensor_profile(dev->model->asic_type, sensor, dpihw, 1);
    gl847_setup_sensor(dev, sensor, sensor_profile, reg);

    unsigned start = session.params.startx;
    if (session.num_staggered_lines > 0) {
        start |= 1;
    }

    // start and end coordinate in optical dpi coordinates
    unsigned startx = start + sensor.CCD_start_xoffset * ccd_pixels_per_system_pixel;
    unsigned endx = startx + session.optical_pixels;

  /* sensors are built from 600 dpi segments for LiDE 100/200
   * and 1200 dpi for the 700F */

    // compute pixel coordinate in the given dpihw space, taking segments into account
    startx /= session.hwdpi_divisor * session.segment_count;
    endx /= session.hwdpi_divisor * session.segment_count;
    dev->deseg.pixel_groups = (endx - startx) / ccd_pixels_per_system_pixel;
    dev->deseg.conseq_pixel_dist_bytes = 0;
    dev->deseg.skip_bytes = 0;

  /* use a segcnt rounded to next even number */
    endx += ((session.conseq_pixel_dist_bytes + 1) & 0xfffe) * (session.segment_count - 1) * ccd_pixels_per_system_pixel;

    gl847_set_fe(dev, sensor, AFE_SET);

  /* enable shading */
  r = sanei_genesys_get_address (reg, REG01);
  r->value &= ~REG01_SCAN;
  r->value |= REG01_SHDAREA;
    if ((session.params.flags & SCAN_FLAG_DISABLE_SHADING) ||
        (dev->model->flags & GENESYS_FLAG_NO_CALIBRATION))
    {
      r->value &= ~REG01_DVDSET;
    }
  else
    {
      r->value |= REG01_DVDSET;
    }

  r = sanei_genesys_get_address (reg, REG03);
  r->value &= ~REG03_AVEENB;

    sanei_genesys_set_lamp_power(dev, sensor, *reg,
                                 !(session.params.flags & SCAN_FLAG_DISABLE_LAMP));

  /* BW threshold */
  r = sanei_genesys_get_address (reg, 0x2e);
  r->value = dev->settings.threshold;
  r = sanei_genesys_get_address (reg, 0x2f);
  r->value = dev->settings.threshold;

  /* monochrome / color scan */
  r = sanei_genesys_get_address (reg, REG04);
  switch (session.params.depth)
    {
    case 1:
      r->value &= ~REG04_BITSET;
      r->value |= REG04_LINEART;
      break;
    case 8:
      r->value &= ~(REG04_LINEART | REG04_BITSET);
      break;
    case 16:
      r->value &= ~REG04_LINEART;
      r->value |= REG04_BITSET;
      break;
    }

  r->value &= ~(REG04_FILTER | REG04_AFEMOD);
  if (session.params.channels == 1)
    {
      switch (session.params.color_filter)
	{

           case ColorFilter::RED:
               r->value |= 0x14;
               break;
           case ColorFilter::BLUE:
               r->value |= 0x1c;
               break;
           case ColorFilter::GREEN:
               r->value |= 0x18;
               break;
           default:
               break; // should not happen
	}
    } else {
        r->value |= 0x10; // mono
    }

    sanei_genesys_set_dpihw(*reg, sensor, dpihw);

    // enable gamma tables
    r = sanei_genesys_get_address (reg, REG05);
    if (session.params.flags & SCAN_FLAG_DISABLE_GAMMA) {
        r->value &= ~REG05_GMMENB;
    } else {
        r->value |= REG05_GMMENB;
    }

  /* CIS scanners can do true gray by setting LEDADD */
  /* we set up LEDADD only when asked */
  if (dev->model->is_cis == SANE_TRUE)
    {
      r = sanei_genesys_get_address (reg, 0x87);
      r->value &= ~REG87_LEDADD;
        if (session.enable_ledadd) {
            r->value |= REG87_LEDADD;
        }
      /* RGB weighting
      r = sanei_genesys_get_address (reg, 0x01);
      r->value &= ~REG01_TRUEGRAY;
        if (session.enable_ledadd) {
            r->value |= REG01_TRUEGRAY;
	}*/
    }

  /* words(16bit) before gamma, conversion to 8 bit or lineart*/
    dev->deseg.raw_channel_bytes = multiply_by_depth_ceil(((endx - startx) * session.params.xres) / dpihw,
                                                          session.params.depth);
    dev->deseg.pixel_groups = multiply_by_depth_ceil(dev->deseg.pixel_groups, session.params.depth);

    dev->deseg.curr_byte = 0;
  dev->line_interp = 0;

    unsigned dpiset = session.params.xres * ccd_pixels_per_system_pixel;
    reg->set16(REG_DPISET, dpiset);
    DBG (DBG_io2, "%s: dpiset used=%d\n", __func__, dpiset);

    reg->set16(REG_STRPIXEL, startx / ccd_pixels_per_system_pixel);
    reg->set16(REG_ENDPIXEL, endx / ccd_pixels_per_system_pixel);
    DBG(DBG_io2, "%s: startx=%d\n", __func__, startx / ccd_pixels_per_system_pixel);
    DBG(DBG_io2, "%s: endx  =%d\n", __func__, endx / ccd_pixels_per_system_pixel);

  DBG (DBG_io2, "%s: pixels     =%d\n", __func__, session.optical_pixels);
  DBG (DBG_io2, "%s: depth      =%d\n", __func__, session.params.depth);
  DBG (DBG_io2, "%s: dev->bpl   =%lu\n", __func__, (unsigned long) dev->deseg.raw_channel_bytes);
  DBG (DBG_io2, "%s: dev->len   =%lu\n", __func__, (unsigned long) dev->deseg.pixel_groups);
  DBG (DBG_io2, "%s: dev->segnb =%lu\n", __func__, (unsigned long) dev->session.segment_count);

    dev->deseg.raw_line_bytes = dev->deseg.raw_channel_bytes * session.params.channels;

    dev->oe_buffer.clear();
    dev->oe_buffer.alloc(dev->deseg.raw_line_bytes);

  /* MAXWD is expressed in 4 words unit */
    reg->set24(REG_MAXWD, (dev->deseg.raw_line_bytes >> 2));
    DBG(DBG_io2, "%s: words_per_line used=%d\n", __func__, dev->deseg.raw_line_bytes);

    reg->set16(REG_LPERIOD, exposure_time);
  DBG(DBG_io2, "%s: exposure_time used=%d\n", __func__, exposure_time);

  r = sanei_genesys_get_address (reg, 0x34);
  r->value = sensor.dummy_pixel;
}

static void gl847_compute_session(Genesys_Device* dev, ScanSession& s,
                                  const Genesys_Sensor& sensor)
{
    DBG_HELPER(dbg);

    // in case of dynamic lineart, we use an internal 8 bit gray scan to generate 1 lineart data
    if (s.params.flags & SCAN_FLAG_DYNAMIC_LINEART) {
        s.params.depth = 8;
    }

    compute_session(dev, s, sensor);

    s.computed = true;

    DBG(DBG_info, "%s ", __func__);
    debug_dump(DBG_info, s);
}

// set up registers for an actual scan this function sets up the scanner to scan in normal or single
// line mode
static void gl847_init_scan_regs(Genesys_Device* dev, const Genesys_Sensor& sensor,
                                 Genesys_Register_Set* reg, const ScanSession& session)
{
    DBG_HELPER(dbg);
    session.assert_computed();

  int move;
  unsigned int mflags; /**> motor flags */
  int exposure_time;

  int slope_dpi = 0;
  int dummy = 0;
  int scan_step_type = 1;

    dummy = 3 - session.params.channels;

/* slope_dpi */
/* cis color scan is effectively a gray scan with 3 gray lines per color
   line and a FILTER of 0 */
    if (dev->model->is_cis) {
        slope_dpi = session.params.yres * session.params.channels;
    } else {
        slope_dpi = session.params.yres;
    }

  slope_dpi = slope_dpi * (1 + dummy);

    exposure_time = get_sensor_profile(dev->model->asic_type, sensor,
                                       session.params.xres, 1).exposure_lperiod;
  scan_step_type = sanei_genesys_compute_step_type(gl847_motor_profiles, dev->model->motor_type,
                                                   exposure_time);

  DBG(DBG_info, "%s : exposure_time=%d pixels\n", __func__, exposure_time);
  DBG(DBG_info, "%s : scan_step_type=%d\n", __func__, scan_step_type);

  /* we enable true gray for cis scanners only, and just when doing
   * scan since color calibration is OK for this mode
   */
    gl847_init_optical_regs_scan(dev, sensor, reg, exposure_time, session);

    move = session.params.starty;
    DBG(DBG_info, "%s: move=%d steps\n", __func__, move);

  mflags=0;
    if (session.params.flags & SCAN_FLAG_DISABLE_BUFFER_FULL_MOVE) {
        mflags |= MOTOR_FLAG_DISABLE_BUFFER_FULL_MOVE;
    }
    if (session.params.flags & SCAN_FLAG_FEEDING) {
        mflags |= MOTOR_FLAG_FEED;
  }

    gl847_init_motor_regs_scan(dev, sensor, reg, exposure_time, slope_dpi, scan_step_type,
                               dev->model->is_cis ? session.output_line_count * session.params.channels
                                                  : session.output_line_count,
                               dummy, move, mflags);

    dev->read_buffer.clear();
    dev->read_buffer.alloc(session.buffer_size_read);

    dev->lines_buffer.clear();
    dev->lines_buffer.alloc(session.buffer_size_lines);

    dev->shrink_buffer.clear();
    dev->shrink_buffer.alloc(session.buffer_size_shrink);

    dev->out_buffer.clear();
    dev->out_buffer.alloc(session.buffer_size_out);

    dev->read_bytes_left_after_deseg = session.output_line_bytes * session.output_line_count;

    DBG(DBG_info, "%s: desegment bytes to read = %lu\n", __func__,
        (u_long) dev->read_bytes_left_after_deseg);

  dev->read_active = SANE_TRUE;

    dev->session = session;
    dev->current_setup.pixels = session.output_pixels;
    dev->current_setup.lines = session.output_line_count;
  dev->current_setup.exposure_time = exposure_time;
    dev->current_setup.xres = session.params.xres;
  dev->current_setup.ccd_size_divisor = session.ccd_size_divisor;
    dev->current_setup.stagger = session.num_staggered_lines;
    dev->current_setup.max_shift = session.max_color_shift_lines + session.num_staggered_lines;

    dev->total_bytes_read = 0;
    dev->total_bytes_to_read =
            multiply_by_depth_ceil(session.params.get_requested_pixels() * session.params.lines,
                                   session.params.depth) * session.params.channels;

  DBG(DBG_info, "%s: total bytes to send = %lu\n", __func__, (u_long) dev->total_bytes_to_read);
/* END TODO */
}

static void
gl847_calculate_current_setup(Genesys_Device * dev, const Genesys_Sensor& sensor)
{
  int start;

  int exposure_time;

  int slope_dpi = 0;
  int dummy = 0;

    DBG(DBG_info, "%s ", __func__);
    debug_dump(DBG_info, dev->settings);

  /* start */
  start = SANE_UNFIX (dev->model->x_offset);
  start += dev->settings.tl_x;
  start = (start * sensor.optical_res) / MM_PER_INCH;

    ScanSession session;
    session.params.xres = dev->settings.xres;
    session.params.yres = dev->settings.yres;
    session.params.startx = start; // not used
    session.params.starty = 0; // not used
    session.params.pixels = dev->settings.pixels;
    session.params.requested_pixels = dev->settings.requested_pixels;
    session.params.lines = dev->settings.lines;
    session.params.depth = dev->settings.get_depth();
    session.params.channels = dev->settings.get_channels();
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = dev->settings.scan_mode;
    session.params.color_filter = dev->settings.color_filter;
    session.params.flags = 0;

    gl847_compute_session(dev, session, sensor);

  /* compute scan parameters values */
  /* pixels are allways given at half or full CCD optical resolution */
  /* use detected left margin  and fixed value */

  /* compute correct pixels number */
    dummy = 3 - session.params.channels;

  /* slope_dpi */
  /* cis color scan is effectively a gray scan with 3 gray lines per color
   line and a FILTER of 0 */
    if (dev->model->is_cis) {
        slope_dpi = session.params.yres * session.params.channels;
    } else {
        slope_dpi = session.params.yres;
    }

  slope_dpi = slope_dpi * (1 + dummy);

    exposure_time = get_sensor_profile(dev->model->asic_type, sensor,
                                       session.params.xres, 1).exposure_lperiod;
  DBG(DBG_info, "%s : exposure_time=%d pixels\n", __func__, exposure_time);

    dev->session = session;
    dev->current_setup.pixels = session.output_pixels;
    dev->current_setup.lines = session.output_line_count;
  dev->current_setup.exposure_time = exposure_time;
    dev->current_setup.xres = session.params.xres;
    dev->current_setup.ccd_size_divisor = session.ccd_size_divisor;
    dev->current_setup.stagger = session.num_staggered_lines;
    dev->current_setup.max_shift = session.max_color_shift_lines + session.num_staggered_lines;
}

// for fast power saving methods only, like disabling certain amplifiers
static void gl847_save_power(Genesys_Device* dev, SANE_Bool enable)
{
    DBG_HELPER_ARGS(dbg, "enable = %d", enable);
    (void) dev;
}

static void gl847_set_powersaving(Genesys_Device* dev, int delay /* in minutes */)
{
    (void) dev;
    DBG_HELPER_ARGS(dbg, "delay = %d", delay);
}

static void gl847_start_action(Genesys_Device* dev)
{
    DBG_HELPER(dbg);
    dev->write_register(0x0f, 0x01);
}

static void gl847_stop_action(Genesys_Device* dev)
{
    DBG_HELPER(dbg);
    uint8_t val;
  unsigned int loop;

    // post scan gpio : without that HOMSNR is unreliable
    gl847_homsnr_gpio(dev);
  sanei_genesys_get_status(dev, &val);
  if (DBG_LEVEL >= DBG_io)
    {
      sanei_genesys_print_status (val);
    }

    uint8_t val40 = dev->read_register(REG40);

  /* only stop action if needed */
  if (!(val40 & REG40_DATAENB) && !(val40 & REG40_MOTMFLG))
    {
      DBG(DBG_info, "%s: already stopped\n", __func__);
      return;
    }

  /* ends scan */
  val = dev->reg.get8(REG01);
  val &= ~REG01_SCAN;
    dev->reg.set8(REG01, val);
    dev->write_register(REG01, val);

  sanei_genesys_sleep_ms(100);

  loop = 10;
  while (loop > 0)
    {
        sanei_genesys_get_status(dev, &val);
      if (DBG_LEVEL >= DBG_io)
	{
	  sanei_genesys_print_status (val);
	}
        val40 = dev->read_register(REG40);

      /* if scanner is in command mode, we are done */
      if (!(val40 & REG40_DATAENB) && !(val40 & REG40_MOTMFLG)
	  && !(val & REG41_MOTORENB))
	{
      return;
	}

      sanei_genesys_sleep_ms(100);
      loop--;
    }

    throw SaneException(SANE_STATUS_IO_ERROR, "could not stop motor");
}

// Send the low-level scan command
static void gl847_begin_scan(Genesys_Device* dev, const Genesys_Sensor& sensor,
                             Genesys_Register_Set* reg, SANE_Bool start_motor)
{
    DBG_HELPER(dbg);
    (void) sensor;
  uint8_t val;
  GenesysRegister *r;

    // clear GPIO 10
    if (dev->model->gpo_type != GPO_CANONLIDE700) {
        val = dev->read_register(REG6C);
        val &= ~REG6C_GPIO10;
        dev->write_register(REG6C, val);
    }

  val = REG0D_CLRLNCNT;
    dev->write_register(REG0D, val);
  val = REG0D_CLRMCNT;
    dev->write_register(REG0D, val);

    val = dev->read_register(REG01);
    val |= REG01_SCAN;
    dev->write_register(REG01, val);
  r = sanei_genesys_get_address (reg, REG01);
  r->value = val;

    if (start_motor) {
        dev->write_register(REG0F, 1);
    } else {
        dev->write_register(REG0F, 0);
    }
}


// Send the stop scan command
static void gl847_end_scan(Genesys_Device* dev, Genesys_Register_Set* reg, SANE_Bool check_stop)
{
    (void) reg;
    DBG_HELPER_ARGS(dbg, "check_stop = %d", check_stop);

    if (dev->model->is_sheetfed != SANE_TRUE) {
        gl847_stop_action(dev);
    }
}

/** rewind scan
 * Move back by the same amount of distance than previous scan.
 * @param dev device to rewind
 */
#if 0                           /* disabled to fix #7 */
static void gl847_rewind(Genesys_Device* dev)
{
    DBG_HELPER(dbg);
  uint8_t byte;


    // set motor reverse
    uint8_t byte = dev->read_register(0x02);
    byte |= 0x04;
    dev->write_register(0x02, byte);

    // and start scan, then wait completion
    gl847_begin_scan(dev, dev->reg, SANE_TRUE);
  do
    {
      sanei_genesys_sleep_ms(100);
        byte = dev->read_register(REG40);
    }
  while(byte & REG40_MOTMFLG);
    gl847_end_scan(dev, dev->reg, SANE_TRUE);

    // restore direction
    byte = dev->read_register(0x02);
    byte &= 0xfb;
    dev->write_register(0x02, byte);
}
#endif

/** Park head
 * Moves the slider to the home (top) position slowly
 * @param dev device to park
 * @param wait_until_home true to make the function waiting for head
 * to be home before returning, if fals returne immediately
*/
static void gl847_slow_back_home(Genesys_Device* dev, SANE_Bool wait_until_home)
{
    DBG_HELPER_ARGS(dbg, "wait_until_home = %d", wait_until_home);
  Genesys_Register_Set local_reg;
  GenesysRegister *r;
  float resolution;
  uint8_t val;
  int loop = 0;
  ScanColorMode scan_mode;

    // post scan gpio : without that HOMSNR is unreliable
    gl847_homsnr_gpio(dev);

    // first read gives HOME_SENSOR true
    sanei_genesys_get_status(dev, &val);

  if (DBG_LEVEL >= DBG_io)
    {
      sanei_genesys_print_status (val);
    }
  sanei_genesys_sleep_ms(100);

    // second is reliable
    sanei_genesys_get_status(dev, &val);

  if (DBG_LEVEL >= DBG_io)
    {
      sanei_genesys_print_status (val);
    }

  /* is sensor at home? */
  if (val & HOMESNR)
    {
      DBG(DBG_info, "%s: already at home, completed\n", __func__);
      dev->scanhead_position_in_steps = 0;
        return;
    }

  local_reg = dev->reg;

  resolution=sanei_genesys_get_lowest_ydpi(dev);

  const auto& sensor = sanei_genesys_find_sensor_any(dev);

  /* TODO add scan_mode to the API */
  scan_mode = dev->settings.scan_mode;
  dev->settings.scan_mode = ScanColorMode::LINEART;

    ScanSession session;
    session.params.xres = resolution;
    session.params.yres = resolution;
    session.params.startx = 100;
    session.params.starty = 30000;
    session.params.pixels = 100;
    session.params.lines = 100;
    session.params.depth = 8;
    session.params.channels = 1;
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = ScanColorMode::GRAY;
    session.params.color_filter = ColorFilter::RED;
    session.params.flags = SCAN_FLAG_DISABLE_SHADING |
                           SCAN_FLAG_DISABLE_GAMMA |
                           SCAN_FLAG_IGNORE_LINE_DISTANCE;
    gl847_compute_session(dev, session, sensor);

    gl847_init_scan_regs(dev, sensor, &local_reg, session);

  dev->settings.scan_mode = scan_mode;

    // clear scan and feed count
    dev->write_register(REG0D, REG0D_CLRLNCNT | REG0D_CLRMCNT);

  /* set up for reverse */
  r = sanei_genesys_get_address (&local_reg, REG02);
  r->value |= REG02_MTRREV;

    dev->write_registers(local_reg);

    try {
        gl847_start_action(dev);
    } catch (...) {
        catch_all_exceptions(__func__, [&]() { gl847_stop_action(dev); });
        // restore original registers
        catch_all_exceptions(__func__, [&]()
        {
            dev->write_registers(dev->reg);
        });
        throw;
    }

    // post scan gpio : without that HOMSNR is unreliable
    gl847_homsnr_gpio(dev);

  if (wait_until_home)
    {
      while (loop < 300)	/* do not wait longer then 30 seconds */
	{
        sanei_genesys_get_status(dev, &val);

	  if (val & HOMESNR)	/* home sensor */
	    {
	      DBG(DBG_info, "%s: reached home position\n", __func__);
              gl847_stop_action (dev);
              dev->scanhead_position_in_steps = 0;
            return;
	    }
          sanei_genesys_sleep_ms(100);
	  ++loop;
	}

        // when we come here then the scanner needed too much time for this, so we better stop
        // the motor
        catch_all_exceptions(__func__, [&](){ gl847_stop_action(dev); });
        throw SaneException(SANE_STATUS_IO_ERROR, "timeout while waiting for scanhead to go home");
    }

  DBG(DBG_info, "%s: scanhead is still moving\n", __func__);
}

// Automatically set top-left edge of the scan area by scanning a 200x200 pixels area at 600 dpi
// from very top of scanner
static void gl847_search_start_position(Genesys_Device* dev)
{
    DBG_HELPER(dbg);
  int size;
  Genesys_Register_Set local_reg;
  int steps;

  int pixels = 600;
  int dpi = 300;

  local_reg = dev->reg;

  /* sets for a 200 lines * 600 pixels */
  /* normal scan with no shading */

    // FIXME: the current approach of doing search only for one resolution does not work on scanners
    // whith employ different sensors with potentially different settings.
    const auto& sensor = sanei_genesys_find_sensor(dev, dpi, 1, ScanMethod::FLATBED);

    ScanSession session;
    session.params.xres = dpi;
    session.params.yres = dpi;
    session.params.startx =  0;
    session.params.starty =  0; /*we should give a small offset here~60 steps */
    session.params.pixels = 600;
    session.params.lines = dev->model->search_lines;
    session.params.depth = 8;
    session.params.channels =  1;
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = ScanColorMode::GRAY;
    session.params.color_filter = ColorFilter::GREEN;
    session.params.flags = SCAN_FLAG_DISABLE_SHADING |
                           SCAN_FLAG_DISABLE_GAMMA |
                           SCAN_FLAG_IGNORE_LINE_DISTANCE;
    gl847_compute_session(dev, session, sensor);

    gl847_init_scan_regs(dev, sensor, &local_reg, session);

    // send to scanner
    dev->write_registers(local_reg);

  size = pixels * dev->model->search_lines;

  std::vector<uint8_t> data(size);

    gl847_begin_scan(dev, sensor, &local_reg, SANE_TRUE);

        // waits for valid data
        do {
            sanei_genesys_test_buffer_empty(dev, &steps);
        } while (steps);

    // now we're on target, we can read data
    sanei_genesys_read_data_from_scanner(dev, data.data(), size);

    if (DBG_LEVEL >= DBG_data) {
        sanei_genesys_write_pnm_file("gl847_search_position.pnm", data.data(), 8, 1, pixels,
                                     dev->model->search_lines);
    }

    gl847_end_scan(dev, &local_reg, SANE_TRUE);

  /* update regs to copy ASIC internal state */
  dev->reg = local_reg;

    // TODO: find out where sanei_genesys_search_reference_point stores information,
    // and use that correctly
    for (auto& sensor_update : sanei_genesys_find_sensors_all_for_write(dev, ScanMethod::FLATBED)) {
        sanei_genesys_search_reference_point(dev, sensor_update, data.data(), 0, dpi, pixels,
                                             dev->model->search_lines);
    }
}

// sets up register for coarse gain calibration
// todo: check it for scanners using it
static void gl847_init_regs_for_coarse_calibration(Genesys_Device* dev,
                                                   const Genesys_Sensor& sensor,
                                                   Genesys_Register_Set& regs)
{
    DBG_HELPER(dbg);

    ScanSession session;
    session.params.xres = dev->settings.xres;
    session.params.yres = dev->settings.yres;
    session.params.startx = 0;
    session.params.starty = 0;
    session.params.pixels = sensor.optical_res / sensor.ccd_pixels_per_system_pixel();
    session.params.lines = 20;
    session.params.depth = 16;
    session.params.channels = dev->settings.get_channels();
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = dev->settings.scan_mode;
    session.params.color_filter = dev->settings.color_filter;
    session.params.flags = SCAN_FLAG_DISABLE_SHADING |
                           SCAN_FLAG_DISABLE_GAMMA |
                           SCAN_FLAG_SINGLE_LINE |
                           SCAN_FLAG_IGNORE_LINE_DISTANCE;
    gl847_compute_session(dev, session, sensor);

    gl847_init_scan_regs(dev, sensor, &regs, session);

  DBG(DBG_info, "%s: optical sensor res: %d dpi, actual res: %d\n", __func__,
      sensor.optical_res / sensor.ccd_pixels_per_system_pixel(), dev->settings.xres);

    dev->write_registers(regs);
}

/** @brief moves the slider to steps at motor base dpi
 * @param dev device to work on
 * @param steps number of steps to move in base_dpi line count
 * */
static void gl847_feed(Genesys_Device* dev, unsigned int steps)
{
    DBG_HELPER_ARGS(dbg, "steps=%d", steps);
  Genesys_Register_Set local_reg;
  GenesysRegister *r;
  float resolution;
  uint8_t val;

  local_reg = dev->reg;

  resolution=sanei_genesys_get_lowest_ydpi(dev);
    const auto& sensor = sanei_genesys_find_sensor(dev, resolution, 3, ScanMethod::FLATBED);

    ScanSession session;
    session.params.xres = resolution;
    session.params.yres = resolution;
    session.params.startx = 0;
    session.params.starty = steps;
    session.params.pixels = 100;
    session.params.lines = 3;
    session.params.depth = 8;
    session.params.channels = 3;
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = ScanColorMode::COLOR_SINGLE_PASS;
    session.params.color_filter = dev->settings.color_filter;
    session.params.flags = SCAN_FLAG_DISABLE_SHADING |
                           SCAN_FLAG_DISABLE_GAMMA |
                           SCAN_FLAG_FEEDING |
                           SCAN_FLAG_IGNORE_LINE_DISTANCE;
    gl847_compute_session(dev, session, sensor);

    gl847_init_scan_regs(dev, sensor, &local_reg, session);

    // set exposure to zero
    local_reg.set24(REG_EXPR,0);
    local_reg.set24(REG_EXPG,0);
    local_reg.set24(REG_EXPB,0);

    // clear scan and feed count
    dev->write_register(REG0D, REG0D_CLRLNCNT);
    dev->write_register(REG0D, REG0D_CLRMCNT);

  /* set up for no scan */
  r = sanei_genesys_get_address(&local_reg, REG01);
  r->value &= ~REG01_SCAN;

    // send registers
    dev->write_registers(local_reg);

    try {
        gl847_start_action(dev);
    } catch (...) {
        catch_all_exceptions(__func__, [&]() { gl847_stop_action(dev); });
        // restore original registers
        catch_all_exceptions(__func__, [&]()
        {
            dev->write_registers(dev->reg);
        });
        throw;
    }

    // wait until feed count reaches the required value, but do not exceed 30s
    do {
        sanei_genesys_get_status(dev, &val);
    }
  while (!(val & FEEDFSH));

    // then stop scanning
    gl847_stop_action(dev);
}


// init registers for shading calibration
static void gl847_init_regs_for_shading(Genesys_Device* dev, const Genesys_Sensor& sensor,
                                        Genesys_Register_Set& regs)
{
    DBG_HELPER(dbg);
  float move;

  dev->calib_channels = 3;

  /* initial calibration reg values */
  regs = dev->reg;

    dev->calib_resolution = sensor.get_register_hwdpi(dev->settings.xres);
  dev->calib_total_bytes_to_read = 0;
  dev->calib_lines = dev->model->shading_lines;
  if(dev->calib_resolution==4800)
    dev->calib_lines *= 2;
  dev->calib_pixels = (sensor.sensor_pixels*dev->calib_resolution)/sensor.optical_res;
  DBG(DBG_io, "%s: calib_lines  = %d\n", __func__, (int)dev->calib_lines);
  DBG(DBG_io, "%s: calib_pixels = %d\n", __func__, (int)dev->calib_pixels);

  /* this is aworkaround insufficent distance for slope
   * motor acceleration TODO special motor slope for shading  */
  move=1;
  if(dev->calib_resolution<1200)
    {
      move=40;
    }

    ScanSession session;
    session.params.xres = dev->calib_resolution;
    session.params.yres = dev->calib_resolution;
    session.params.startx = 0;
    session.params.starty = move;
    session.params.pixels = dev->calib_pixels;
    session.params.lines = dev->calib_lines;
    session.params.depth = 16;
    session.params.channels = dev->calib_channels;
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = ScanColorMode::COLOR_SINGLE_PASS;
    session.params.color_filter = dev->settings.color_filter;
    session.params.flags = SCAN_FLAG_DISABLE_SHADING |
                           SCAN_FLAG_DISABLE_GAMMA |
                           SCAN_FLAG_DISABLE_BUFFER_FULL_MOVE |
                           SCAN_FLAG_IGNORE_LINE_DISTANCE;
    gl847_compute_session(dev, session, sensor);

    gl847_init_scan_regs(dev, sensor, &regs, session);

    dev->write_registers(regs);

  /* we use GENESYS_FLAG_SHADING_REPARK */
  dev->scanhead_position_in_steps = 0;
}

/** @brief set up registers for the actual scan
 */
static void gl847_init_regs_for_scan(Genesys_Device* dev, const Genesys_Sensor& sensor)
{
    DBG_HELPER(dbg);
  int flags;
  float move;
  int move_dpi;
  float start;

    debug_dump(DBG_info, dev->settings);

  /* steps to move to reach scanning area:
     - first we move to physical start of scanning
     either by a fixed steps amount from the black strip
     or by a fixed amount from parking position,
     minus the steps done during shading calibration
     - then we move by the needed offset whitin physical
     scanning area

     assumption: steps are expressed at maximum motor resolution

     we need:
     SANE_Fixed y_offset;
     SANE_Fixed y_size;
     SANE_Fixed y_offset_calib;
     mm_to_steps()=motor dpi / 2.54 / 10=motor dpi / MM_PER_INCH */

  /* if scanner uses GENESYS_FLAG_SEARCH_START y_offset is
     relative from origin, else, it is from parking position */

  move_dpi = dev->motor.base_ydpi;

  move = SANE_UNFIX (dev->model->y_offset);
  move += dev->settings.tl_y;
  move = (move * move_dpi) / MM_PER_INCH;
  move -= dev->scanhead_position_in_steps;
  DBG(DBG_info, "%s: move=%f steps\n", __func__, move);

  /* fast move to scan area */
  /* we don't move fast the whole distance since it would involve
   * computing acceleration/deceleration distance for scan
   * resolution. So leave a remainder for it so scan makes the final
   * move tuning */
    if (dev->settings.get_channels() * dev->settings.yres >= 600 && move > 700) {
        gl847_feed(dev, move-500);
      move=500;
    }

  DBG(DBG_info, "%s: move=%f steps\n", __func__, move);
  DBG(DBG_info, "%s: move=%f steps\n", __func__, move);

  /* start */
  start = SANE_UNFIX (dev->model->x_offset);
  start += dev->settings.tl_x;
  start = (start * sensor.optical_res) / MM_PER_INCH;

  flags = 0;

  /* emulated lineart from gray data is required for now */
  if(dev->settings.scan_mode == ScanColorMode::LINEART
     && dev->settings.dynamic_lineart)
    {
      flags |= SCAN_FLAG_DYNAMIC_LINEART;
    }

  /* backtracking isn't handled well, so don't enable it */
  flags |= SCAN_FLAG_DISABLE_BUFFER_FULL_MOVE;

    ScanSession session;
    session.params.xres = dev->settings.xres;
    session.params.yres = dev->settings.yres;
    session.params.startx = start;
    session.params.starty = move;
    session.params.pixels = dev->settings.pixels;
    session.params.requested_pixels = dev->settings.requested_pixels;
    session.params.lines = dev->settings.lines;
    session.params.depth = dev->settings.get_depth();
    session.params.channels = dev->settings.get_channels();
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = dev->settings.scan_mode;
    session.params.color_filter = dev->settings.color_filter;
    session.params.flags = flags;
    gl847_compute_session(dev, session, sensor);

    gl847_init_scan_regs(dev, sensor, &dev->reg, session);
}


/**
 * Send shading calibration data. The buffer is considered to always hold values
 * for all the channels.
 */
static void gl847_send_shading_data(Genesys_Device* dev, const Genesys_Sensor& sensor,
                                    uint8_t* data, int size)
{
    DBG_HELPER_ARGS(dbg, "writing %d bytes of shading data", size);
  uint32_t addr, length, i, x, factor, pixels;
  uint32_t dpiset, dpihw, strpixel, endpixel;
    uint32_t lines;
  uint8_t val,*ptr,*src;

  /* shading data is plit in 3 (up to 5 with IR) areas
     write(0x10014000,0x00000dd8)
     URB 23429  bulk_out len  3544  wrote 0x33 0x10 0x....
     write(0x1003e000,0x00000dd8)
     write(0x10068000,0x00000dd8)
   */
  length = (uint32_t) (size / 3);
    strpixel = dev->reg.get16(REG_STRPIXEL);
    endpixel = dev->reg.get16(REG_ENDPIXEL);

  /* compute deletion factor */
    dpiset = dev->reg.get16(REG_DPISET);
  DBG(DBG_io2, "%s: STRPIXEL=%d, ENDPIXEL=%d, PIXELS=%d, DPISET=%d\n", __func__, strpixel, endpixel,
      endpixel-strpixel, dpiset);
    dpihw = sensor.get_register_hwdpi(dpiset);
  factor=dpihw/dpiset;
  DBG(DBG_io2, "%s: factor=%d\n", __func__, factor);

  if(DBG_LEVEL>=DBG_data)
    {
      dev->binary=fopen("binary.pnm","wb");
        lines = dev->reg.get24(REG_LINCNT);
        unsigned channels = dev->session.params.channels;
      if(dev->binary!=NULL)
        {
          fprintf(dev->binary,"P5\n%d %d\n%d\n",(endpixel-strpixel)/factor*channels,lines/channels,255);
        }
    }

  pixels=endpixel-strpixel;

  /* since we're using SHDAREA, substract startx coordinate from shading */
  strpixel-=((sensor.CCD_start_xoffset*600)/sensor.optical_res);

  /* turn pixel value into bytes 2x16 bits words */
  strpixel*=2*2;
  pixels*=2*2;

  std::vector<uint8_t> buffer(pixels, 0);

  DBG(DBG_io2, "%s: using chunks of %d (0x%04x) bytes\n", __func__, pixels, pixels);

  /* base addr of data has been written in reg D0-D4 in 4K word, so AHB address
   * is 8192*reg value */

  /* write actual color channel data */
  for(i=0;i<3;i++)
    {
      /* build up actual shading data by copying the part from the full width one
       * to the one corresponding to SHDAREA */
      ptr = buffer.data();

      /* iterate on both sensor segment */
      for(x=0;x<pixels;x+=4*factor)
        {
          /* coefficient source */
          src=(data+strpixel+i*length)+x;

          /* coefficient copy */
          ptr[0]=src[0];
          ptr[1]=src[1];
          ptr[2]=src[2];
          ptr[3]=src[3];

          /* next shading coefficient */
          ptr+=4;
        }

        val = dev->read_register(0xd0+i);
        addr = val * 8192 + 0x10000000;
        sanei_genesys_write_ahb(dev, addr, pixels, buffer.data());
    }
}

/** @brief calibrates led exposure
 * Calibrate exposure by scanning a white area until the used exposure gives
 * data white enough.
 * @param dev device to calibrate
 */
static SensorExposure gl847_led_calibration(Genesys_Device* dev, const Genesys_Sensor& sensor,
                                            Genesys_Register_Set& regs)
{
    DBG_HELPER(dbg);
  int num_pixels;
  int total_size;
  int used_res;
  int i, j;
  int val;
  int channels, depth;
  int avg[3], top[3], bottom[3];
  int turn;
  uint16_t exp[3];
  float move;
  SANE_Bool acceptable;

  move = SANE_UNFIX (dev->model->y_offset_calib);
  move = (move * (dev->motor.base_ydpi/4)) / MM_PER_INCH;
  if(move>20)
    {
        gl847_feed(dev, move);
    }
  DBG(DBG_io, "%s: move=%f steps\n", __func__, move);

  /* offset calibration is always done in color mode */
  channels = 3;
  depth=16;
    used_res = sensor.get_register_hwdpi(dev->settings.xres);
    const auto& sensor_profile = get_sensor_profile(dev->model->asic_type, sensor, used_res, 1);
  num_pixels = (sensor.sensor_pixels*used_res)/sensor.optical_res;

  /* initial calibration reg values */
  regs = dev->reg;

    ScanSession session;
    session.params.xres = used_res;
    session.params.yres = used_res;
    session.params.startx = 0;
    session.params.starty = 0;
    session.params.pixels = num_pixels;
    session.params.lines = 1;
    session.params.depth = depth;
    session.params.channels = channels;
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = ScanColorMode::COLOR_SINGLE_PASS;
    session.params.color_filter = dev->settings.color_filter;
    session.params.flags = SCAN_FLAG_DISABLE_SHADING |
                           SCAN_FLAG_DISABLE_GAMMA |
                           SCAN_FLAG_SINGLE_LINE |
                           SCAN_FLAG_IGNORE_LINE_DISTANCE;
    gl847_compute_session(dev, session, sensor);

    gl847_init_scan_regs(dev, sensor, &regs, session);

  total_size = num_pixels * channels * (depth/8) * 1;	/* colors * bytes_per_color * scan lines */
  std::vector<uint8_t> line(total_size);

    // initial loop values and boundaries
    exp[0] = sensor_profile.exposure.red;
    exp[1] = sensor_profile.exposure.green;
    exp[2] = sensor_profile.exposure.blue;

  bottom[0]=29000;
  bottom[1]=29000;
  bottom[2]=29000;

  top[0]=41000;
  top[1]=51000;
  top[2]=51000;

  turn = 0;

  /* no move during led calibration */
  sanei_genesys_set_motor_power(regs, false);
  do
    {
        // set up exposure
        regs.set16(REG_EXPR,exp[0]);
        regs.set16(REG_EXPG,exp[1]);
        regs.set16(REG_EXPB,exp[2]);

        // write registers and scan data
        dev->write_registers(regs);

      DBG(DBG_info, "%s: starting line reading\n", __func__);
        gl847_begin_scan(dev, sensor, &regs, SANE_TRUE);
        sanei_genesys_read_data_from_scanner(dev, line.data(), total_size);

        // stop scanning
        gl847_stop_action(dev);

      if (DBG_LEVEL >= DBG_data)
	{
          char fn[30];
          snprintf(fn, 30, "gl847_led_%02d.pnm", turn);
          sanei_genesys_write_pnm_file(fn, line.data(), depth, channels, num_pixels, 1);
	}

      /* compute average */
      for (j = 0; j < channels; j++)
	{
	  avg[j] = 0;
	  for (i = 0; i < num_pixels; i++)
	    {
	      if (dev->model->is_cis)
		val =
		  line[i * 2 + j * 2 * num_pixels + 1] * 256 +
		  line[i * 2 + j * 2 * num_pixels];
	      else
		val =
		  line[i * 2 * channels + 2 * j + 1] * 256 +
		  line[i * 2 * channels + 2 * j];
	      avg[j] += val;
	    }

	  avg[j] /= num_pixels;
	}

      DBG(DBG_info, "%s: average: %d,%d,%d\n", __func__, avg[0], avg[1], avg[2]);

      /* check if exposure gives average within the boundaries */
      acceptable = SANE_TRUE;
      for(i=0;i<3;i++)
        {
          if(avg[i]<bottom[i])
            {
              exp[i]=(exp[i]*bottom[i])/avg[i];
              acceptable = SANE_FALSE;
            }
          if(avg[i]>top[i])
            {
              exp[i]=(exp[i]*top[i])/avg[i];
              acceptable = SANE_FALSE;
            }
        }

      turn++;
    }
  while (!acceptable && turn < 100);

  DBG(DBG_info, "%s: acceptable exposure: %d,%d,%d\n", __func__, exp[0], exp[1], exp[2]);

    // set these values as final ones for scan
    dev->reg.set16(REG_EXPR, exp[0]);
    dev->reg.set16(REG_EXPG, exp[1]);
    dev->reg.set16(REG_EXPB, exp[2]);

    // go back home
    if (move>20) {
        gl847_slow_back_home(dev, SANE_TRUE);
    }

    return { exp[0], exp[1], exp[2] };
}

/**
 * set up GPIO/GPOE for idle state
 */
static void gl847_init_gpio(Genesys_Device* dev)
{
    DBG_HELPER(dbg);
  int idx=0;

  /* search GPIO profile */
  while(gpios[idx].sensor_id!=0 && dev->model->gpo_type!=gpios[idx].sensor_id)
    {
      idx++;
    }
  if(gpios[idx].sensor_id==0)
    {
        throw SaneException("failed to find GPIO profile for sensor_id=%d", dev->model->ccd_type);
    }

    dev->write_register(REGA7, gpios[idx].ra7);
    dev->write_register(REGA6, gpios[idx].ra6);

    dev->write_register(REG6E, gpios[idx].r6e);
    dev->write_register(REG6C, 0x00);

    dev->write_register(REG6B, gpios[idx].r6b);
    dev->write_register(REG6C, gpios[idx].r6c);
    dev->write_register(REG6D, gpios[idx].r6d);
    dev->write_register(REG6E, gpios[idx].r6e);
    dev->write_register(REG6F, gpios[idx].r6f);

    dev->write_register(REGA8, gpios[idx].ra8);
    dev->write_register(REGA9, gpios[idx].ra9);
}

/**
 * set memory layout by filling values in dedicated registers
 */
static void gl847_init_memory_layout(Genesys_Device* dev)
{
    DBG_HELPER(dbg);
  int idx = 0;
  uint8_t val;

  /* point to per model memory layout */
  idx = 0;
  if (dev->model->model_id == MODEL_CANON_LIDE_100)
    {
      idx = 0;
    }
  if (dev->model->model_id == MODEL_CANON_LIDE_200)
    {
      idx = 1;
    }
  if (dev->model->model_id == MODEL_CANON_CANOSCAN_5600F)
    {
      idx = 2;
    }
  if (dev->model->model_id == MODEL_CANON_LIDE_700F)
    {
      idx = 3;
    }

  /* CLKSET nd DRAMSEL */
  val = layouts[idx].dramsel;
    dev->write_register(REG0B, val);
  dev->reg.find_reg(0x0b).value = val;

  /* prevent further writings by bulk write register */
  dev->reg.remove_reg(0x0b);

  /* setup base address for shading data. */
  /* values must be multiplied by 8192=0x4000 to give address on AHB */
  /* R-Channel shading bank0 address setting for CIS */
    dev->write_register(0xd0, layouts[idx].rd0);
  /* G-Channel shading bank0 address setting for CIS */
    dev->write_register(0xd1, layouts[idx].rd1);
  /* B-Channel shading bank0 address setting for CIS */
    dev->write_register(0xd2, layouts[idx].rd2);

  /* setup base address for scanned data. */
  /* values must be multiplied by 1024*2=0x0800 to give address on AHB */
  /* R-Channel ODD image buffer 0x0124->0x92000 */
  /* size for each buffer is 0x16d*1k word */
    dev->write_register(0xe0, layouts[idx].re0);
    dev->write_register(0xe1, layouts[idx].re1);
  /* R-Channel ODD image buffer end-address 0x0291->0x148800 => size=0xB6800*/
    dev->write_register(0xe2, layouts[idx].re2);
    dev->write_register(0xe3, layouts[idx].re3);

  /* R-Channel EVEN image buffer 0x0292 */
    dev->write_register(0xe4, layouts[idx].re4);
    dev->write_register(0xe5, layouts[idx].re5);
  /* R-Channel EVEN image buffer end-address 0x03ff*/
    dev->write_register(0xe6, layouts[idx].re6);
    dev->write_register(0xe7, layouts[idx].re7);

  /* same for green, since CIS, same addresses */
    dev->write_register(0xe8, layouts[idx].re0);
    dev->write_register(0xe9, layouts[idx].re1);
    dev->write_register(0xea, layouts[idx].re2);
    dev->write_register(0xeb, layouts[idx].re3);
    dev->write_register(0xec, layouts[idx].re4);
    dev->write_register(0xed, layouts[idx].re5);
    dev->write_register(0xee, layouts[idx].re6);
    dev->write_register(0xef, layouts[idx].re7);

/* same for blue, since CIS, same addresses */
    dev->write_register(0xf0, layouts[idx].re0);
    dev->write_register(0xf1, layouts[idx].re1);
    dev->write_register(0xf2, layouts[idx].re2);
    dev->write_register(0xf3, layouts[idx].re3);
    dev->write_register(0xf4, layouts[idx].re4);
    dev->write_register(0xf5, layouts[idx].re5);
    dev->write_register(0xf6, layouts[idx].re6);
    dev->write_register(0xf7, layouts[idx].re7);
}

/* *
 * initialize ASIC from power on condition
 */
static void gl847_boot(Genesys_Device* dev, SANE_Bool cold)
{
    DBG_HELPER(dbg);

    // reset ASIC if cold boot
    if (cold) {
        dev->write_register(0x0e, 0x01);
        dev->write_register(0x0e, 0x00);
    }

    // test CHKVER
    uint8_t val = dev->read_register(REG40);
    if (val & REG40_CHKVER) {
        val = dev->read_register(0x00);
        DBG(DBG_info, "%s: reported version for genesys chip is 0x%02x\n", __func__, val);
    }

  /* Set default values for registers */
  gl847_init_registers (dev);

    // Write initial registers
    dev->write_registers(dev->reg);

  /* Enable DRAM by setting a rising edge on bit 3 of reg 0x0b */
  val = dev->reg.find_reg(0x0b).value & REG0B_DRAMSEL;
  val = (val | REG0B_ENBDRAM);
    dev->write_register(REG0B, val);
  dev->reg.find_reg(0x0b).value = val;

  /* CIS_LINE */
  SETREG (0x08, REG08_CIS_LINE);
    dev->write_register(0x08, dev->reg.find_reg(0x08).value);

    // set up end access
    sanei_genesys_write_0x8c(dev, 0x10, 0x0b);
    sanei_genesys_write_0x8c(dev, 0x13, 0x0e);

    // setup gpio
    gl847_init_gpio(dev);

    // setup internal memory layout
    gl847_init_memory_layout (dev);

  SETREG (0xf8, 0x01);
    dev->write_register(0xf8, dev->reg.find_reg(0xf8).value);
}

/**
 * initialize backend and ASIC : registers, motor tables, and gamma tables
 * then ensure scanner's head is at home
 */
static void gl847_init(Genesys_Device* dev)
{
  DBG_INIT ();
    DBG_HELPER(dbg);

    sanei_genesys_asic_init(dev, 0);
}

static void gl847_update_hardware_sensors(Genesys_Scanner* s)
{
    DBG_HELPER(dbg);
  /* do what is needed to get a new set of events, but try to not lose
     any of them.
   */
  uint8_t val;
  uint8_t scan, file, email, copy;
  switch(s->dev->model->gpo_type)
    {
      case GPO_CANONLIDE700:
        scan=0x04;
        file=0x02;
        email=0x01;
        copy=0x08;
        break;
      default:
        scan=0x01;
        file=0x02;
        email=0x04;
        copy=0x08;
    }
    val = s->dev->read_register(REG6D);

    s->buttons[BUTTON_SCAN_SW].write((val & scan) == 0);
    s->buttons[BUTTON_FILE_SW].write((val & file) == 0);
    s->buttons[BUTTON_EMAIL_SW].write((val & email) == 0);
    s->buttons[BUTTON_COPY_SW].write((val & copy) == 0);
}

/** @brief search for a full width black or white strip.
 * This function searches for a black or white stripe across the scanning area.
 * When searching backward, the searched area must completely be of the desired
 * color since this area will be used for calibration which scans forward.
 * @param dev scanner device
 * @param forward SANE_TRUE if searching forward, SANE_FALSE if searching backward
 * @param black SANE_TRUE if searching for a black strip, SANE_FALSE for a white strip
 */
static void gl847_search_strip(Genesys_Device* dev, const Genesys_Sensor& sensor, SANE_Bool forward,
                               SANE_Bool black)
{
    DBG_HELPER_ARGS(dbg, "%s %s", black ? "black" : "white", forward ? "forward" : "reverse");
  unsigned int pixels, lines, channels;
  Genesys_Register_Set local_reg;
  size_t size;
  int steps, depth;
  unsigned int pass, count, found, x, y;
  char title[80];
  GenesysRegister *r;

  gl847_set_fe(dev, sensor, AFE_SET);
    gl847_stop_action(dev);

    // set up for a gray scan at lowest dpi
    unsigned dpi = *std::min_element(dev->model->xdpi_values.begin(),
                                     dev->model->xdpi_values.end());
  channels = 1;
  /* 10 MM */
  /* lines = (10 * dpi) / MM_PER_INCH; */
  /* shading calibation is done with dev->motor.base_ydpi */
  lines = (dev->model->shading_lines * dpi) / dev->motor.base_ydpi;
  depth = 8;
  pixels = (sensor.sensor_pixels * dpi) / sensor.optical_res;
  size = pixels * channels * lines * (depth / 8);
  std::vector<uint8_t> data(size);
  dev->scanhead_position_in_steps = 0;

  local_reg = dev->reg;

    ScanSession session;
    session.params.xres = dpi;
    session.params.yres = dpi;
    session.params.startx = 0;
    session.params.starty = 0;
    session.params.pixels = pixels;
    session.params.lines = lines;
    session.params.depth = depth;
    session.params.channels = channels;
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = ScanColorMode::GRAY;
    session.params.color_filter = ColorFilter::RED;
    session.params.flags = SCAN_FLAG_DISABLE_SHADING |
                           SCAN_FLAG_DISABLE_GAMMA;
    gl847_compute_session(dev, session, sensor);

    gl847_init_scan_regs(dev, sensor, &local_reg, session);

  /* set up for reverse or forward */
  r = sanei_genesys_get_address(&local_reg, REG02);
    if (forward) {
        r->value &= ~REG02_MTRREV;
    } else {
        r->value |= REG02_MTRREV;
    }

    dev->write_registers(local_reg);

    gl847_begin_scan(dev, sensor, &local_reg, SANE_TRUE);

        // waits for valid data
        do {
            sanei_genesys_test_buffer_empty(dev, &steps);
        } while (steps);

    // now we're on target, we can read data
    sanei_genesys_read_data_from_scanner(dev, data.data(), size);

    gl847_stop_action(dev);

  pass = 0;
  if (DBG_LEVEL >= DBG_data)
    {
      sprintf(title, "gl847_search_strip_%s_%s%02d.pnm",
              black ? "black" : "white", forward ? "fwd" : "bwd", (int)pass);
      sanei_genesys_write_pnm_file(title, data.data(), depth, channels, pixels, lines);
    }

  /* loop until strip is found or maximum pass number done */
  found = 0;
  while (pass < 20 && !found)
    {
        dev->write_registers(local_reg);

            // now start scan
            gl847_begin_scan(dev, sensor, &local_reg, SANE_TRUE);

        // waits for valid data
        do {
            sanei_genesys_test_buffer_empty(dev, &steps);
        } while (steps);

        // now we're on target, we can read data
        sanei_genesys_read_data_from_scanner(dev, data.data(), size);

    gl847_stop_action(dev);

      if (DBG_LEVEL >= DBG_data)
	{
          sprintf(title, "gl847_search_strip_%s_%s%02d.pnm",
                  black ? "black" : "white", forward ? "fwd" : "bwd", (int)pass);
          sanei_genesys_write_pnm_file(title, data.data(), depth, channels, pixels, lines);
	}

      /* search data to find black strip */
      /* when searching forward, we only need one line of the searched color since we
       * will scan forward. But when doing backward search, we need all the area of the
       * same color */
      if (forward)
	{
	  for (y = 0; y < lines && !found; y++)
	    {
	      count = 0;
	      /* count of white/black pixels depending on the color searched */
	      for (x = 0; x < pixels; x++)
		{
		  /* when searching for black, detect white pixels */
		  if (black && data[y * pixels + x] > 90)
		    {
		      count++;
		    }
		  /* when searching for white, detect black pixels */
		  if (!black && data[y * pixels + x] < 60)
		    {
		      count++;
		    }
		}

	      /* at end of line, if count >= 3%, line is not fully of the desired color
	       * so we must go to next line of the buffer */
	      /* count*100/pixels < 3 */
	      if ((count * 100) / pixels < 3)
		{
		  found = 1;
		  DBG(DBG_data, "%s: strip found forward during pass %d at line %d\n", __func__,
		      pass, y);
		}
	      else
		{
		  DBG(DBG_data, "%s: pixels=%d, count=%d (%d%%)\n", __func__, pixels, count,
		      (100 * count) / pixels);
		}
	    }
	}
      else			/* since calibration scans are done forward, we need the whole area
				   to be of the required color when searching backward */
	{
	  count = 0;
	  for (y = 0; y < lines; y++)
	    {
	      /* count of white/black pixels depending on the color searched */
	      for (x = 0; x < pixels; x++)
		{
		  /* when searching for black, detect white pixels */
		  if (black && data[y * pixels + x] > 90)
		    {
		      count++;
		    }
		  /* when searching for white, detect black pixels */
		  if (!black && data[y * pixels + x] < 60)
		    {
		      count++;
		    }
		}
	    }

	  /* at end of area, if count >= 3%, area is not fully of the desired color
	   * so we must go to next buffer */
	  if ((count * 100) / (pixels * lines) < 3)
	    {
	      found = 1;
	      DBG(DBG_data, "%s: strip found backward during pass %d \n", __func__, pass);
	    }
	  else
	    {
	      DBG(DBG_data, "%s: pixels=%d, count=%d (%d%%)\n", __func__, pixels, count,
		  (100 * count) / pixels);
	    }
	}
      pass++;
    }

  if (found)
    {
      DBG(DBG_info, "%s: %s strip found\n", __func__, black ? "black" : "white");
    }
  else
    {
        throw SaneException(SANE_STATUS_UNSUPPORTED, "%s strip not found", black ? "black" : "white");
    }
}

/**
 * average dark pixels of a 8 bits scan
 */
static int
dark_average (uint8_t * data, unsigned int pixels, unsigned int lines,
	      unsigned int channels, unsigned int black)
{
  unsigned int i, j, k, average, count;
  unsigned int avg[3];
  uint8_t val;

  /* computes average value on black margin */
  for (k = 0; k < channels; k++)
    {
      avg[k] = 0;
      count = 0;
      for (i = 0; i < lines; i++)
	{
	  for (j = 0; j < black; j++)
	    {
	      val = data[i * channels * pixels + j + k];
	      avg[k] += val;
	      count++;
	    }
	}
      if (count)
	avg[k] /= count;
      DBG(DBG_info, "%s: avg[%d] = %d\n", __func__, k, avg[k]);
    }
  average = 0;
  for (i = 0; i < channels; i++)
    average += avg[i];
  average /= channels;
  DBG(DBG_info, "%s: average = %d\n", __func__, average);
  return average;
}

static void gl847_offset_calibration(Genesys_Device* dev, const Genesys_Sensor& sensor,
                                     Genesys_Register_Set& regs)
{
    DBG_HELPER(dbg);
  unsigned int channels, bpp;
  int pass = 0, avg, total_size;
    int topavg, bottomavg, lines;
  int top, bottom, black_pixels, pixels;

    // no gain nor offset for AKM AFE
    uint8_t reg04 = dev->read_register(REG04);
  if ((reg04 & REG04_FESET) == 0x02)
    {
      return;
    }

  /* offset calibration is always done in color mode */
  channels = 3;
  dev->calib_pixels = sensor.sensor_pixels;
  lines=1;
  bpp=8;
    pixels= (sensor.sensor_pixels * sensor.optical_res) / sensor.optical_res;
    black_pixels = (sensor.black_pixels * sensor.optical_res) / sensor.optical_res;
  DBG(DBG_io2, "%s: black_pixels=%d\n", __func__, black_pixels);

    ScanSession session;
    session.params.xres = sensor.optical_res;
    session.params.yres = sensor.optical_res;
    session.params.startx = 0;
    session.params.starty = 0;
    session.params.pixels = pixels;
    session.params.lines = lines;
    session.params.depth = bpp;
    session.params.channels = channels;
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = ScanColorMode::COLOR_SINGLE_PASS;
    session.params.color_filter = dev->settings.color_filter;
    session.params.flags = SCAN_FLAG_DISABLE_SHADING |
                           SCAN_FLAG_DISABLE_GAMMA |
                           SCAN_FLAG_SINGLE_LINE |
                           SCAN_FLAG_IGNORE_LINE_DISTANCE;
    gl847_compute_session(dev, session, sensor);

    gl847_init_scan_regs(dev, sensor, &regs, session);

  sanei_genesys_set_motor_power(regs, false);

  /* allocate memory for scans */
  total_size = pixels * channels * lines * (bpp/8);	/* colors * bytes_per_color * scan lines */

  std::vector<uint8_t> first_line(total_size);
  std::vector<uint8_t> second_line(total_size);

  /* init gain */
  dev->frontend.set_gain(0, 0);
  dev->frontend.set_gain(1, 0);
  dev->frontend.set_gain(2, 0);

  /* scan with no move */
  bottom = 10;
  dev->frontend.set_offset(0, bottom);
  dev->frontend.set_offset(1, bottom);
  dev->frontend.set_offset(2, bottom);

    gl847_set_fe(dev, sensor, AFE_SET);
    dev->write_registers(regs);
  DBG(DBG_info, "%s: starting first line reading\n", __func__);
    gl847_begin_scan(dev, sensor, &regs, SANE_TRUE);
    sanei_genesys_read_data_from_scanner(dev, first_line.data(), total_size);
  if (DBG_LEVEL >= DBG_data)
   {
      char fn[30];
      snprintf(fn, 30, "gl847_offset%03d.pnm", bottom);
      sanei_genesys_write_pnm_file(fn, first_line.data(), bpp, channels, pixels, lines);
   }

  bottomavg = dark_average (first_line.data(), pixels, lines, channels, black_pixels);
  DBG(DBG_io2, "%s: bottom avg=%d\n", __func__, bottomavg);

  /* now top value */
  top = 255;
  dev->frontend.set_offset(0, top);
  dev->frontend.set_offset(1, top);
  dev->frontend.set_offset(2, top);
    gl847_set_fe(dev, sensor, AFE_SET);
    dev->write_registers(regs);
  DBG(DBG_info, "%s: starting second line reading\n", __func__);
    gl847_begin_scan(dev, sensor, &regs, SANE_TRUE);
    sanei_genesys_read_data_from_scanner(dev, second_line.data(), total_size);

  topavg = dark_average(second_line.data(), pixels, lines, channels, black_pixels);
  DBG(DBG_io2, "%s: top avg=%d\n", __func__, topavg);

  /* loop until acceptable level */
  while ((pass < 32) && (top - bottom > 1))
    {
      pass++;

      /* settings for new scan */
      dev->frontend.set_offset(0, (top + bottom) / 2);
      dev->frontend.set_offset(1, (top + bottom) / 2);
      dev->frontend.set_offset(2, (top + bottom) / 2);

        // scan with no move
        gl847_set_fe(dev, sensor, AFE_SET);
        dev->write_registers(regs);
      DBG(DBG_info, "%s: starting second line reading\n", __func__);
        gl847_begin_scan(dev, sensor, &regs, SANE_TRUE);
        sanei_genesys_read_data_from_scanner(dev, second_line.data(), total_size);

      if (DBG_LEVEL >= DBG_data)
	{
          char fn[30];
          snprintf(fn, 30, "gl847_offset%03d.pnm", dev->frontend.get_offset(1));
          sanei_genesys_write_pnm_file(fn, second_line.data(), bpp, channels, pixels, lines);
	}

      avg = dark_average(second_line.data(), pixels, lines, channels, black_pixels);
      DBG(DBG_info, "%s: avg=%d offset=%d\n", __func__, avg, dev->frontend.get_offset(1));

      /* compute new boundaries */
      if (topavg == avg)
	{
	  topavg = avg;
          top = dev->frontend.get_offset(1);
	}
      else
	{
	  bottomavg = avg;
          bottom = dev->frontend.get_offset(1);
	}
    }
  DBG(DBG_info, "%s: offset=(%d,%d,%d)\n", __func__,
      dev->frontend.get_offset(0),
      dev->frontend.get_offset(1),
      dev->frontend.get_offset(2));
}

static void gl847_coarse_gain_calibration(Genesys_Device* dev, const Genesys_Sensor& sensor,
                                          Genesys_Register_Set& regs, int dpi)
{
    DBG_HELPER_ARGS(dbg, "dpi = %d", dpi);
  int pixels;
  int total_size;
  int i, j, channels;
  int max[3];
  float gain[3],coeff;
  int val, code, lines;
  int bpp;

    // no gain nor offset for AKM AFE
    uint8_t reg04 = dev->read_register(REG04);
  if ((reg04 & REG04_FESET) == 0x02)
    {
      return;
    }

  /* coarse gain calibration is always done in color mode */
  channels = 3;

  /* follow CKSEL */
  if(dev->settings.xres<sensor.optical_res)
    {
      coeff=0.9;
    }
  else
    {
      coeff=1.0;
    }
  lines=10;
  bpp=8;
    pixels = (sensor.sensor_pixels * sensor.optical_res) / sensor.optical_res;

    ScanSession session;
    session.params.xres = sensor.optical_res;
    session.params.yres = sensor.optical_res;
    session.params.startx = 0;
    session.params.starty = 0;
    session.params.pixels = pixels;
    session.params.lines = lines;
    session.params.depth = bpp;
    session.params.channels = channels;
    session.params.scan_method = dev->settings.scan_method;
    session.params.scan_mode = ScanColorMode::COLOR_SINGLE_PASS;
    session.params.color_filter = dev->settings.color_filter;
    session.params.flags = SCAN_FLAG_DISABLE_SHADING |
                           SCAN_FLAG_DISABLE_GAMMA |
                           SCAN_FLAG_SINGLE_LINE |
                           SCAN_FLAG_IGNORE_LINE_DISTANCE;
    gl847_compute_session(dev, session, sensor);

    try {
        gl847_init_scan_regs(dev, sensor, &regs, session);
    } catch (...) {
        catch_all_exceptions(__func__, [&](){ sanei_genesys_set_motor_power(regs, false); });
        throw;
    }

    sanei_genesys_set_motor_power(regs, false);

    dev->write_registers(regs);

  total_size = pixels * channels * (16/bpp) * lines;

  std::vector<uint8_t> line(total_size);

    gl847_set_fe(dev, sensor, AFE_SET);
    gl847_begin_scan(dev, sensor, &regs, SANE_TRUE);
    sanei_genesys_read_data_from_scanner(dev, line.data(), total_size);

  if (DBG_LEVEL >= DBG_data)
    sanei_genesys_write_pnm_file("gl847_gain.pnm", line.data(), bpp, channels, pixels, lines);

  /* average value on each channel */
  for (j = 0; j < channels; j++)
    {
      max[j] = 0;
      for (i = pixels/4; i < (pixels*3/4); i++)
	{
          if(bpp==16)
            {
	  if (dev->model->is_cis)
	    val =
	      line[i * 2 + j * 2 * pixels + 1] * 256 +
	      line[i * 2 + j * 2 * pixels];
	  else
	    val =
	      line[i * 2 * channels + 2 * j + 1] * 256 +
	      line[i * 2 * channels + 2 * j];
            }
          else
            {
	  if (dev->model->is_cis)
	    val = line[i + j * pixels];
	  else
	    val = line[i * channels + j];
            }

	    max[j] += val;
	}
      max[j] = max[j] / (pixels/2);

      gain[j] = ((float) sensor.gain_white_ref*coeff) / max[j];

      /* turn logical gain value into gain code, checking for overflow */
      code = 283 - 208 / gain[j];
      if (code > 255)
	code = 255;
      else if (code < 0)
	code = 0;
      dev->frontend.set_gain(j, code);

      DBG(DBG_proc, "%s: channel %d, max=%d, gain = %f, setting:%d\n", __func__, j, max[j], gain[j],
          dev->frontend.get_gain(j));
    }

    if (dev->model->is_cis) {
        uint8_t gain0 = dev->frontend.get_gain(0);
        if (gain0 > dev->frontend.get_gain(1)) {
            gain0 = dev->frontend.get_gain(1);
        }
        if (gain0 > dev->frontend.get_gain(2)) {
            gain0 = dev->frontend.get_gain(2);
        }
        dev->frontend.set_gain(0, gain0);
        dev->frontend.set_gain(1, gain0);
        dev->frontend.set_gain(2, gain0);
    }

    if (channels == 1) {
        dev->frontend.set_gain(0, dev->frontend.get_gain(1));
        dev->frontend.set_gain(2, dev->frontend.get_gain(1));
    }

    gl847_stop_action(dev);

    gl847_slow_back_home(dev, SANE_TRUE);
}


/** the gl847 command set */
Genesys_Command_Set gl847_cmd_set = {
  "gl847-generic",		/* the name of this set */

  nullptr,

  gl847_init,
  NULL, /*gl847_init_regs_for_warmup*/
  gl847_init_regs_for_coarse_calibration,
  gl847_init_regs_for_shading,
  gl847_init_regs_for_scan,

  gl847_get_filter_bit,
  gl847_get_lineart_bit,
  gl847_get_bitset_bit,
  gl847_get_gain4_bit,
  gl847_get_fast_feed_bit,
  gl847_test_buffer_empty_bit,
  gl847_test_motor_flag_bit,

  gl847_set_fe,
  gl847_set_powersaving,
  gl847_save_power,

  gl847_begin_scan,
  gl847_end_scan,

  sanei_genesys_send_gamma_table,

  gl847_search_start_position,

  gl847_offset_calibration,
  gl847_coarse_gain_calibration,
  gl847_led_calibration,

  NULL,
  gl847_slow_back_home,
  NULL, /* disable gl847_rewind, see #7 */

  NULL,
  sanei_genesys_bulk_read_data,

  gl847_update_hardware_sensors,

  NULL, /* no known gl847 sheetfed scanner */
  NULL, /* no known gl847 sheetfed scanner */
  NULL, /* no known gl847 sheetfed scanner */
  gl847_search_strip,

  sanei_genesys_is_compatible_calibration,
  NULL,
  gl847_send_shading_data,
  gl847_calculate_current_setup,
  gl847_boot
};
