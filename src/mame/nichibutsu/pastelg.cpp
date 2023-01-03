// license:BSD-3-Clause
// copyright-holders:Takahiro Nogi
/******************************************************************************

    Game Driver for Nichibutsu Mahjong series.

    Pastel Gal
    (c)1985 Nihon Bussan Co.,Ltd.

    Driver by Takahiro Nogi 2000/06/07 -

******************************************************************************/
/******************************************************************************
Memo:

- Custom chip used by pastelg PCB is 1411M1.

- Some games display "GFXROM BANK OVER!!" or "GFXROM ADDRESS OVER!!"
  in Debug build.

- Screen flip is not perfect.

******************************************************************************/

#include "emu.h"
#include "pastelg.h"

#include "cpu/z80/z80.h"
#include "machine/nvram.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "speaker.h"


void threeds_state::machine_start()
{
	save_item(NAME(m_mux_data));
}

uint8_t pastelg_state::sndrom_r()
{
	return m_voice_rom[blitter_src_addr_r() & 0x7fff];
}

void pastelg_common_state::prg_map(address_map &map)
{
	map(0x0000, 0xbfff).rom();
	map(0xe000, 0xe7ff).ram().share("nvram");
}

uint8_t pastelg_common_state::irq_ack_r()
{
	m_maincpu->set_input_line(0, CLEAR_LINE);
	return 0;
}

void pastelg_state::io_map(address_map &map)
{
	map.global_mask(0xff);
//  map(0x00, 0x00).nopw();
	map(0x00, 0x7f).r(m_nb1413m3, FUNC(nb1413m3_device::sndrom_r));
	map(0x81, 0x81).r("aysnd", FUNC(ay8910_device::data_r));
	map(0x82, 0x83).w("aysnd", FUNC(ay8910_device::data_address_w));
	map(0x90, 0x90).portr("SYSTEM");
	map(0x90, 0x96).w(FUNC(pastelg_state::blitter_w));
	map(0xa0, 0xa0).rw(m_nb1413m3, FUNC(nb1413m3_device::inputport1_r), FUNC(nb1413m3_device::inputportsel_w));
	map(0xb0, 0xb0).r(m_nb1413m3, FUNC(nb1413m3_device::inputport2_r)).w(FUNC(pastelg_state::romsel_w));
	map(0xc0, 0xc0).r(FUNC(pastelg_state::sndrom_r));
	map(0xc0, 0xcf).writeonly().share("clut");
	map(0xd0, 0xd0).r(FUNC(pastelg_state::irq_ack_r)).w("dac", FUNC(dac_byte_interface::data_w));
	map(0xe0, 0xe0).portr("DSWC");
}


uint8_t threeds_state::inputport1_r()
{
	switch(m_mux_data)
	{
		case 0x01: return m_p1_keys[0]->read();
		case 0x02: return m_p1_keys[1]->read();
		case 0x04: return m_p1_keys[2]->read();
		case 0x08: return m_p1_keys[3]->read();
		case 0x10: return m_p1_keys[4]->read();
	}

	return 0xff;
}

uint8_t threeds_state::inputport2_r()
{
	switch(m_mux_data)
	{
		case 0x01: return m_p2_keys[0]->read();
		case 0x02: return m_p2_keys[1]->read();
		case 0x04: return m_p2_keys[2]->read();
		case 0x08: return m_p2_keys[3]->read();
		case 0x10: return m_p2_keys[4]->read();
	}

	return 0xff;
}

void threeds_state::inputportsel_w(uint8_t data)
{
	m_mux_data = ~data;
}

void threeds_state::io_map(address_map &map)
{
	map.global_mask(0xff);
	map(0x81, 0x81).r("aysnd", FUNC(ay8910_device::data_r));
	map(0x82, 0x83).w("aysnd", FUNC(ay8910_device::data_address_w));
	map(0x90, 0x90).portr("SYSTEM").w(FUNC(threeds_state::romsel_w));
	map(0xf0, 0xf6).w(FUNC(threeds_state::blitter_w));
	map(0xa0, 0xa0).rw(FUNC(threeds_state::inputport1_r), FUNC(threeds_state::inputportsel_w));
	map(0xb0, 0xb0).r(FUNC(threeds_state::inputport2_r)).w(FUNC(threeds_state::output_w)); //writes: bit 3 is coin lockout, bit 1 is coin counter
	map(0xc0, 0xcf).writeonly().share("clut");
	map(0xc0, 0xc0).r(FUNC(threeds_state::rom_readback_r));
	map(0xd0, 0xd0).r(FUNC(threeds_state::irq_ack_r)).w("dac", FUNC(dac_byte_interface::data_w));
}

static INPUT_PORTS_START( pastelg )
	PORT_START("DSWA")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
	PORT_DIPNAME( 0x04, 0x04, "DIPSW 1-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 1-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 1-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIPSW 1-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIPSW 1-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 1-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSWB")
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x04, 0x04, "No. of tiles on final match" )
	PORT_DIPSETTING(    0x04, "20" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 2-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 2-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, "SANGEN Rush" )
	PORT_DIPSETTING(    0x60, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Infinite ) )
	PORT_DIPNAME( 0x80, 0x80, "DIPSW 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSWC")
	PORT_DIPNAME( 0x03, 0x03, "Change Rate" )
	PORT_DIPSETTING(    0x03, "Type-A" )
	PORT_DIPSETTING(    0x02, "Type-B" )
	PORT_DIPSETTING(    0x01, "Type-C" )
	PORT_DIPSETTING(    0x00, "Type-D" )
	PORT_DIPNAME( 0x04, 0x00, "Open CPU's hand on Player's Reach" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIPSW 3-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIPSW 3-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "YAKUMAN cut" )
	PORT_DIPSETTING(    0x60, "10%" )
	PORT_DIPSETTING(    0x40, "30%" )
	PORT_DIPSETTING(    0x20, "50%" )
	PORT_DIPSETTING(    0x00, "90%" )
	PORT_DIPNAME( 0x80, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("nb1413m3", nb1413m3_device, busyflag_r)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )         //
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MEMORY_RESET )   // MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )       // ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )                 // TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )          // COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE4 )       // CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )       // SERVICE

	PORT_INCLUDE( nbmjcontrols )
INPUT_PORTS_END

static INPUT_PORTS_START( threeds )
	PORT_START("DSWA")
	PORT_DIPNAME( 0x07,   0x04, "Game Out Rate" ) PORT_DIPLOCATION("DSWA:1,2,3")
	PORT_DIPSETTING(      0x07, "55" )
	PORT_DIPSETTING(      0x06, "60" )
	PORT_DIPSETTING(      0x05, "65" )
	PORT_DIPSETTING(      0x04, "70" )
	PORT_DIPSETTING(      0x03, "75" )
	PORT_DIPSETTING(      0x02, "80" )
	PORT_DIPSETTING(      0x01, "85" )
	PORT_DIPSETTING(      0x00, "90" )
	PORT_DIPNAME( 0x08,   0x08, DEF_STR( Unknown ) )  PORT_DIPLOCATION("DSWA:4")
	PORT_DIPSETTING(      0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30,   0x20, DEF_STR( Difficulty ) )  PORT_DIPLOCATION("DSWA:5,6")
	PORT_DIPSETTING(      0x30, "0" )
	PORT_DIPSETTING(      0x20, "1" )
	PORT_DIPSETTING(      0x10, "2" )
	PORT_DIPSETTING(      0x00, "3" )
	PORT_DIPNAME( 0xc0,   0xc0, DEF_STR( Coinage ) )  PORT_DIPLOCATION("DSWA:7,8")
	PORT_DIPSETTING (     0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (     0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (     0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING (     0x00, "1 Coin/10 Credits" )

	PORT_START("DSWB")
	PORT_DIPNAME( 0x01,   0x01, DEF_STR( Flip_Screen ) )  PORT_DIPLOCATION("DSWB:1")
	PORT_DIPSETTING(      0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02,   0x02, DEF_STR( Unknown ) )  PORT_DIPLOCATION("DSWB:2")
	PORT_DIPSETTING(      0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04,   0x04, DEF_STR( Unknown ) )  PORT_DIPLOCATION("DSWB:3")
	PORT_DIPSETTING(      0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08,   0x08, DEF_STR( Unknown ) )  PORT_DIPLOCATION("DSWB:4")
	PORT_DIPSETTING(      0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10,   0x10, DEF_STR( Unknown ) )  PORT_DIPLOCATION("DSWB:5")
	PORT_DIPSETTING(      0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20,   0x20, DEF_STR( Unknown ) )  PORT_DIPLOCATION("DSWB:6")
	PORT_DIPSETTING(      0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40,   0x40, DEF_STR( Unknown ) )  PORT_DIPLOCATION("DSWB:7")
	PORT_DIPSETTING(      0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80,   0x80, DEF_STR( Unknown ) )  PORT_DIPLOCATION("DSWB:8")
	PORT_DIPSETTING(      0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )

	PORT_START("PL1_KEY0")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("1P Start / Deal")
	PORT_BIT( 0x38, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40,   0x40, "1P-Side Character Test Mode" ) //only combined with the service mode
	PORT_DIPSETTING(      0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL1_KEY1")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_BET ) PORT_NAME("1P Bet") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("1P Hold 5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("1P Hold 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("1P Hold 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL1_KEY2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("1P Change Dealer") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL1_KEY3")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("1P Hold 4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("1P Hold 2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL1_KEY4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL ) PORT_NAME("1P Small")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_BIG ) PORT_NAME("1P Big")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP ) PORT_NAME("1P Flip Flop")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP ) PORT_NAME("1P Double Up")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE ) PORT_NAME("1P Take Score")
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL2_KEY0")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 ) PORT_NAME("2P Start / Deal")
	PORT_BIT( 0x38, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40,   0x40, "2P-Side Character Test Mode" ) //only combined with the service mode
	PORT_DIPSETTING(      0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL2_KEY1")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_BET ) PORT_NAME("2P Bet") PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("2P Hold 5") PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("2P Hold 3") PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("2P Hold 1") PORT_PLAYER(2)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL2_KEY2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("2P Change Dealer") PORT_PLAYER(2)
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL2_KEY3")
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("2P Hold 4") PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("2P Hold 2") PORT_PLAYER(2)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PL2_KEY4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_SMALL ) PORT_NAME("2P Small") PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_BIG ) PORT_NAME("2P Big") PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP ) PORT_NAME("2P Flip Flop") PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_DOUBLE_UP ) PORT_NAME("2P Double Up") PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_SCORE ) PORT_NAME("2P Take Score") PORT_PLAYER(2)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("nb1413m3", nb1413m3_device, busyflag_r)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )         //
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MEMORY_RESET )   // MEMORY RESET
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )       // ANALYZER
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )                 // TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )          // COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE4 )       // CREDIT CLEAR
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )       // SERVICE
INPUT_PORTS_END



static INPUT_PORTS_START( galds )
	PORT_INCLUDE(threeds)

	PORT_MODIFY("SYSTEM")
	// this increases the tip? (has this feature been ripped out of the parent set? there is strange corruption of the line under the 'tip' display)
	PORT_DIPNAME( 0x01,   0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02,   0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
INPUT_PORTS_END


void pastelg_state::pastelg(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, 19968000/4);    // unknown divider, galds definitely relies on this for correct voice pitch
	m_maincpu->set_addrmap(AS_PROGRAM, &pastelg_state::prg_map);
	m_maincpu->set_addrmap(AS_IO, &pastelg_state::io_map);
	m_maincpu->set_vblank_int("screen", FUNC(pastelg_state::irq0_line_assert)); // nmiclock not written, chip is 1411M1 instead of 1413M3

	NB1413M3(config, m_nb1413m3, 0, NB1413M3_PASTELG);

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);

	/* video hardware */
	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_refresh_hz(60);
	m_screen->set_vblank_time(ATTOSECONDS_IN_USEC(0));
	m_screen->set_size(256, 256);
	m_screen->set_visarea(0, 256-1, 16, 240-1);
	m_screen->set_screen_update(FUNC(pastelg_state::screen_update));
	m_screen->set_palette("palette");

	PALETTE(config, "palette", FUNC(pastelg_state::palette), 32);

	/* sound hardware */
	SPEAKER(config, "speaker").front_center();

	ay8910_device &aysnd(AY8910(config, "aysnd", 1250000));
	aysnd.port_a_read_callback().set_ioport("DSWA");
	aysnd.port_b_read_callback().set_ioport("DSWB");
	aysnd.add_route(ALL_OUTPUTS, "speaker", 0.35);

	DAC_8BIT_R2R(config, "dac", 0).add_route(ALL_OUTPUTS, "speaker", 0.25); // unknown DAC
}

/*

Produttore  Nichibutsu
N.revisione TD-1412a
CPU
1x custom Nichibutsu PG14111 (DIL40)(main?)
1x custom Nichibutsu PG14112 (DIL40)(sound?)
1x custom Nichibutsu PG14113 (DIL20)(PAL)
1x custom Nichibutsu PG14114 (DIL20)(PAL)
1x custom Nichibutsu PG1411M1XBA (DIL28)(maybe it's ram)
1x oscillator 19.968MHz
ROMs
7x MBM27256
3x MBM27128
2x PROM MB7112E
Note
1x 18x2 edge connector
1x 10x2 edge connector
2x trimmer (MAIN, SUB)
2x 8 switches dip

*/

void threeds_state::threeds(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, 19968000/4);    // unknown divider, galds definitely relies on this for correct voice pitch
	m_maincpu->set_addrmap(AS_PROGRAM, &threeds_state::prg_map);
	m_maincpu->set_addrmap(AS_IO, &threeds_state::io_map);
	m_maincpu->set_vblank_int("screen", FUNC(threeds_state::irq0_line_assert));

	NB1413M3(config, m_nb1413m3, 0);
	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);

	/* video hardware */
	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_refresh_hz(60);
	m_screen->set_vblank_time(ATTOSECONDS_IN_USEC(0));
	m_screen->set_size(256, 256);
	m_screen->set_visarea(0, 256-1, 16, 240-1);
	m_screen->set_screen_update(FUNC(threeds_state::screen_update));
	m_screen->set_palette("palette");

	PALETTE(config, "palette", FUNC(threeds_state::palette), 32);

	/* sound hardware */
	SPEAKER(config, "speaker").front_center();

	ay8910_device &aysnd(AY8910(config, "aysnd", 1250000));
	aysnd.port_a_read_callback().set_ioport("DSWB");
	aysnd.port_b_read_callback().set_ioport("DSWA");
	aysnd.add_route(ALL_OUTPUTS, "speaker", 0.35);

	DAC_8BIT_R2R(config, "dac", 0).add_route(ALL_OUTPUTS, "speaker", 0.25); // unknown DAC
}


ROM_START( pastelg )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pgal_09.bin",  0x00000, 0x04000, CRC(1e494af3) SHA1(1597a7da22ecfbb1df83cf9d0acc7a8be461bc2c) )
	ROM_LOAD( "pgal_10.bin",  0x04000, 0x04000, CRC(677cccea) SHA1(a294bf4e3c5e74291160a0858371961868afc1d1) )
	ROM_LOAD( "pgal_11.bin",  0x08000, 0x04000, CRC(c2ccea38) SHA1(0374e8aa0e7961426e417ffe6e1a0d8dc7fd9ecf) )

	ROM_REGION( 0x08000, "voice", 0 )
	ROM_LOAD( "pgal_08.bin",  0x00000, 0x08000, CRC(895961a1) SHA1(f02d517f46cc490db02c4feb369e2a386c764297) )

	ROM_REGION( 0x38000, "blitter", 0 )
	ROM_LOAD( "pgal_01.bin",  0x00000, 0x08000, CRC(1bb14d52) SHA1(b3974e3c9b56a752ddcb206f7bb2bc658b0e77f1) )
	ROM_LOAD( "pgal_02.bin",  0x08000, 0x08000, CRC(ea85673a) SHA1(85ef2bb736fe5229ce4153197db8a57bca982a8b) )
	ROM_LOAD( "pgal_03.bin",  0x10000, 0x08000, CRC(40011248) SHA1(935f442a47e02bf8c6ccb324c7fad1b481b8b19a) )
	ROM_LOAD( "pgal_04.bin",  0x18000, 0x08000, CRC(10613a66) SHA1(ad11f99f402e5b247d086cfccafea351da30c084) )
	ROM_LOAD( "pgal_05.bin",  0x20000, 0x08000, CRC(6a152703) SHA1(5dd46d876453c5c79f5a382d77234c690da75001) )
	ROM_LOAD( "pgal_06.bin",  0x28000, 0x08000, CRC(f56acfe8) SHA1(2f4ad3990f2d4d4a9fcec7adab119459423b308b) )
	ROM_LOAD( "pgal_07.bin",  0x30000, 0x08000, CRC(fa4226dc) SHA1(2313449521f81a191e87f1e4c0f3473f3c27dd9d) )

	ROM_REGION( 0x0040, "proms", 0 ) // color
	ROM_LOAD( "pgal_bp1.bin", 0x0000, 0x0020, CRC(2b7fc61a) SHA1(278830e8728ea143208376feb20fff56de88ae1c) )
	ROM_LOAD( "pgal_bp2.bin", 0x0020, 0x0020, CRC(4433021e) SHA1(e0d6619a193d26ad24788d4af5ef01ee89cffacd) )
ROM_END

ROM_START( threeds )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ft9.9a",    0x00000, 0x04000, CRC(bc0e7cfd) SHA1(4e84f573fb2c1228757d34b8bc69649b145d9707) ) // labeled FT9  (red label)
	ROM_LOAD( "ft10.10a",  0x04000, 0x04000, CRC(e185d9f5) SHA1(98d4a824ed6a89e42543fb87daed33ef606bcced) ) // labeled FT10 (red label)
	ROM_LOAD( "ft11.11a",  0x08000, 0x04000, CRC(d1fb728b) SHA1(46e8e6ccdc1b78da29c969cd9290158c96bac4c4) ) // labeled FT11 (red label)

	ROM_REGION( 0x38000, "blitter", 0 )
	ROM_LOAD( "1.1a",  0x00000, 0x08000, CRC(5734ca7d) SHA1(d22b9e604cc4e2c0bb4eb32ded06bb5fa519965f) ) // roms 1 through 7 where labeled simply "1" through "7" in black labels
	ROM_LOAD( "2.2a",  0x08000, 0x08000, CRC(c7f21718) SHA1(4b2956d499e4db63e7f2329420e3d0313e6360ed) )
	ROM_LOAD( "3.3a",  0x10000, 0x08000, CRC(87bd0a9e) SHA1(a0443017ef4c19f0135c4f764a96457f02cda743) )
	ROM_LOAD( "4.4a",  0x18000, 0x08000, CRC(b75ecf2b) SHA1(50b8f27988dd24ff475a500d361db3c7a7051f40) )
	ROM_LOAD( "5.5a",  0x20000, 0x08000, CRC(22ee5cf6) SHA1(09725a73f5f107e6fcb1994d94a50748726318b0) )
	ROM_LOAD( "6.6a",  0x28000, 0x08000, CRC(d86ebe8d) SHA1(2ede43899501ae27db26b48f53f010a4f0df0307) )
	ROM_LOAD( "7.7a",  0x30000, 0x08000, CRC(6704950a) SHA1(fd60ff2351deb87f19e517cfaedc7ac3dd4aac8d) )

	ROM_REGION( 0x0040, "proms", 0 ) // color
	ROM_LOAD( "mb7112e.7h", 0x0000, 0x0020, CRC(2c4f7343) SHA1(7b069c4a4d68ef308d1c1f773ece4b124428da3f) )
	ROM_LOAD( "mb7112e.7j", 0x0020, 0x0020, CRC(181f2a88) SHA1(a75ea981127fc667bb6b9f2ae2766aa2147ff04a) )
ROM_END

ROM_START( threedsa ) // Dumped from an original Nichibutsu PCB: TD1412a
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bo9.9a",    0x00000, 0x04000, CRC(352577cf) SHA1(26cea6ee0f303b1a2c2ba3325351656da89d588b) )
	ROM_LOAD( "bo10.10a",  0x04000, 0x04000, CRC(d191fb4b) SHA1(42882cfc1c0ed82f320a1a95ecbedfaa91d5dd94) )
	ROM_LOAD( "bo11.11a",  0x08000, 0x04000, CRC(17318e4b) SHA1(6814141cb643c997b2fafcdc5da2c49e19647d85) )

	ROM_REGION( 0x38000, "blitter", 0 )
	ROM_LOAD( "1.1a",  0x00000, 0x08000, CRC(5734ca7d) SHA1(d22b9e604cc4e2c0bb4eb32ded06bb5fa519965f) )
	ROM_LOAD( "2.2a",  0x08000, 0x08000, CRC(c7f21718) SHA1(4b2956d499e4db63e7f2329420e3d0313e6360ed) )
	ROM_LOAD( "3.3a",  0x10000, 0x08000, CRC(87bd0a9e) SHA1(a0443017ef4c19f0135c4f764a96457f02cda743) )
	ROM_LOAD( "4.4a",  0x18000, 0x08000, CRC(b75ecf2b) SHA1(50b8f27988dd24ff475a500d361db3c7a7051f40) )
	ROM_LOAD( "5.5a",  0x20000, 0x08000, CRC(22ee5cf6) SHA1(09725a73f5f107e6fcb1994d94a50748726318b0) )
	ROM_LOAD( "6.6a",  0x28000, 0x08000, CRC(d86ebe8d) SHA1(2ede43899501ae27db26b48f53f010a4f0df0307) )
	ROM_LOAD( "7.7a",  0x30000, 0x08000, CRC(6704950a) SHA1(fd60ff2351deb87f19e517cfaedc7ac3dd4aac8d) )

	ROM_REGION( 0x0040, "proms", 0 ) // color
	ROM_LOAD( "mb7112e.7h", 0x0000, 0x0020, CRC(2c4f7343) SHA1(7b069c4a4d68ef308d1c1f773ece4b124428da3f) )
	ROM_LOAD( "mb7112e.7j", 0x0020, 0x0020, CRC(181f2a88) SHA1(a75ea981127fc667bb6b9f2ae2766aa2147ff04a) )
ROM_END

// might be a bootleg (or licensed) board? had a sub-board (containing only logic) marked "Sky Dragon" and there were no Nichibutsu markings on any of the PCBs or chips
ROM_START( galds )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dg8.ic3",  0x00000, 0x04000, CRC(06c6a98f) SHA1(828deef4e725cafef5088fc2dfab63b62ae0feb0) )
	ROM_LOAD( "dg9.ic2",  0x04000, 0x04000, CRC(a53eca09) SHA1(001ebb04378e6d7dd5ad15b272e1346655b91eee))
	ROM_LOAD( "dg10.ic1", 0x08000, 0x04000, CRC(6c380c64) SHA1(3f6b037a8a40fd5c4bc3b469ee3c9f1e1bd302a0) )

	ROM_REGION( 0x38000, "blitter", 0 ) // the same as threeds
	ROM_LOAD( "dg1.ic11",  0x00000, 0x08000, CRC(5734ca7d) SHA1(d22b9e604cc4e2c0bb4eb32ded06bb5fa519965f) )
	ROM_LOAD( "dg2.ic10",  0x08000, 0x08000, CRC(c7f21718) SHA1(4b2956d499e4db63e7f2329420e3d0313e6360ed) )
	ROM_LOAD( "dg3.ic9",   0x10000, 0x08000, CRC(87bd0a9e) SHA1(a0443017ef4c19f0135c4f764a96457f02cda743) )
	ROM_LOAD( "dg4.ic8",   0x18000, 0x08000, CRC(b75ecf2b) SHA1(50b8f27988dd24ff475a500d361db3c7a7051f40) )
	ROM_LOAD( "dg5.ic7",   0x20000, 0x08000, CRC(22ee5cf6) SHA1(09725a73f5f107e6fcb1994d94a50748726318b0) )
	ROM_LOAD( "dg6.ic6",   0x28000, 0x08000, CRC(d86ebe8d) SHA1(2ede43899501ae27db26b48f53f010a4f0df0307) )
	ROM_LOAD( "dg7.ic5",   0x30000, 0x08000, CRC(6704950a) SHA1(fd60ff2351deb87f19e517cfaedc7ac3dd4aac8d) )

	ROM_REGION( 0x0040, "proms", 0 ) // color
	ROM_LOAD( "mb7112e.7h", 0x0000, 0x0020, CRC(2c4f7343) SHA1(7b069c4a4d68ef308d1c1f773ece4b124428da3f) )
	ROM_LOAD( "mb7112e.7j", 0x0020, 0x0020, CRC(181f2a88) SHA1(a75ea981127fc667bb6b9f2ae2766aa2147ff04a) )
ROM_END



GAME( 1985, pastelg,  0,       pastelg, pastelg, pastelg_state, empty_init, ROT0, "Nichibutsu", "Pastel Gal (Japan 851224)", MACHINE_SUPPORTS_SAVE )
GAME( 1985, threeds,  0,       threeds, threeds, threeds_state, empty_init, ROT0, "Nichibutsu", "Three Ds - Three Dealers Casino House (set 1)", MACHINE_SUPPORTS_SAVE )
GAME( 1985, threedsa, threeds, threeds, threeds, threeds_state, empty_init, ROT0, "Nichibutsu", "Three Ds - Three Dealers Casino House (set 2)", MACHINE_SUPPORTS_SAVE )
GAME( 1985, galds,    threeds, threeds, galds,   threeds_state, empty_init, ROT0, "Nihon System Corp.", "Gals Ds - Three Dealers Casino House (bootleg?)", MACHINE_SUPPORTS_SAVE )
