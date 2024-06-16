
//============================================================
//
//  window.c - RETRO window handling
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#ifdef SDLMAME_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

// standard C headers
#include <math.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <list>
#include <memory>

// MAME headers
#include "emu.h"
#include "emuopts.h"
#include "render.h"
#include "rendlay.h"
#include "rendutil.h"
#include "ui/uimain.h"

// OSD headers
#include "window.h"
#include "osdretro.h"

#include "modules/render/drawretro.h"
#include "modules/monitor/monitor_common.h"

#include "libretro-internal/libretro_shared.h"

extern bool libretro_supports_ff_override;
extern void retro_fastforwarding(bool enabled);
static bool retro_fastforward;

extern int max_width;
extern int max_height;
extern int libretro_rotation_allow;
extern int internal_rotation_allow;
extern int norotate;

static int svg_width;
static int svg_height;

//============================================================
//  PARAMETERS
//============================================================

// these are arbitrary values since AFAIK there's no way to make X/SDL tell you
#define WINDOW_DECORATION_WIDTH (8) // should be more than plenty
#define WINDOW_DECORATION_HEIGHT (48)   // title bar + bottom drag region

// minimum window dimension
#define MIN_WINDOW_DIM                  200

#ifndef SDLMAME_WIN32
#define WMSZ_TOP            (0)
#define WMSZ_BOTTOM         (1)
#define WMSZ_BOTTOMLEFT     (2)
#define WMSZ_BOTTOMRIGHT    (3)
#define WMSZ_LEFT           (4)
#define WMSZ_TOPLEFT        (5)
#define WMSZ_TOPRIGHT       (6)
#define WMSZ_RIGHT          (7)
#endif

#define SDL_VERSION_EQUALS(v1, vnum2) (SDL_VERSIONNUM(v1.major, v1.minor, v1.patch) == vnum2)

typedef struct
{          
	int w;                      
	int h;                      
	int refresh_rate;           
        
} RETRO_DisplayMode;

class RETRO_DM_Wrapper
{
public:
	RETRO_DisplayMode mode;
};

// debugger
//static int in_background;


//============================================================
//  PROTOTYPES
//============================================================


//============================================================
//  window_init
//  (main thread)
//============================================================

bool retro_osd_interface::window_init()
{
	return true;
}

//============================================================
//  window_exit
//  (main thread)
//============================================================

void retro_osd_interface::window_exit()
{
	// free all the windows
	while (!osd_common_t::s_window_list.empty())
	{
		auto window = std::move(osd_common_t::s_window_list.back());
		s_window_list.pop_back();
		window->destroy();
	}
}


void retro_window_info::capture_pointer()
{
	if (!m_mouse_captured)
	{
		m_mouse_captured = true;
	}
}

void retro_window_info::release_pointer()
{
	if (m_mouse_captured)
	{
		m_mouse_captured = false;
	}
}

void retro_window_info::hide_pointer()
{
	if (!m_mouse_hidden)
	{
		m_mouse_hidden = true;
	}
}

void retro_window_info::show_pointer()
{
	if (m_mouse_hidden)
	{
		m_mouse_hidden = false;
	}
}


//============================================================
//  sdlwindow_resize
//============================================================

void retro_window_info::resize(int32_t width, int32_t height)
{
	osd_dim cd = get_size();
	if (width != cd.width() || height != cd.height())
	{
	    fb_width      = width;
	    fb_height     = height;
	    video_changed = 2;
	    renderer().notify_changed();
    }
}


//============================================================
//  sdlwindow_clear_surface
//============================================================

void retro_window_info::notify_changed()
{
	renderer().notify_changed();
}


//============================================================
//  sdlwindow_toggle_full_screen
//============================================================

void retro_window_info::toggle_full_screen()
{
}

void retro_window_info::modify_prescale(int dir)
{
	int new_prescale = prescale();

	if (dir > 0 && prescale() < 3)
		new_prescale = prescale() + 1;
	if (dir < 0 && prescale() > 1)
		new_prescale = prescale() - 1;

	if (new_prescale != prescale())
	{
		if (m_fullscreen && video_config.switchres)
		{
			complete_destroy();

			m_prescale = new_prescale;

			complete_create();
		}
		else
		{
			notify_changed();
			m_prescale = new_prescale;
		}
		machine().ui().popup_time(1, "Prescale %d", prescale());
	}
}

//============================================================
//  sdlwindow_update_cursor_state
//  (main or window thread)
//============================================================

void retro_window_info::update_cursor_state()
{
}

int retro_window_info::xy_to_render_target(int x, int y, int *xt, int *yt)
{
	return renderer().xy_to_render_target(x, y, xt, yt);
}

//============================================================
//  sdlwindow_video_window_create
//  (main thread)
//============================================================

int retro_window_info::window_init()
{
	int result = 1;

	// set the initial maximized state
	// FIXME: Does not belong here
	retro_options &options = downcast<retro_options &>(machine().options());
	m_startmaximized = options.maximize();

	create_target();

	result = complete_create();

	// store more reasonable geometry for SVG screens
	const screen_device *screen = screen_device_enumerator(machine().root_device()).byindex(index());
	if ((screen != nullptr) && (screen->screen_type() == SCREEN_TYPE_SVG))
	{
		const rectangle &visarea = screen->visible_area();
		svg_width  = render_round_nearest(visarea.width());
		svg_height = render_round_nearest(visarea.height());

		// limit huge size with native renderer
		if (!alternate_renderer)
		{
			if (svg_width >= 1920)
			{
				svg_width  /= 2;
				svg_height /= 2;
			}
		}
	}

	if (!alternate_renderer)
	{
		// test correct aspect
		retro_aspect = target()->current_view().effective_aspect();
		if (target()->orientation() & ORIENTATION_SWAP_XY)
		    retro_aspect = 1.0 / retro_aspect;

		int temp_width, temp_height;
		target()->compute_minimum_size(temp_width, temp_height);
		fb_width  = temp_width;
		fb_height = temp_height;

		video_changed = 2;
	}

	// reset sound timer (set in `sound_manager::update` to `retro_fps`)
	sound_timer = 0;

	// reset screen configuration
	screen_configured = 0;

	// reset machine aspect (set in `retro_window_info::update()`)
	view_aspect = 1;

	// handle error conditions
	if (result == 1)
		goto error;

	return 0;

error:
	destroy();
	return 1;
}


//============================================================
//  sdlwindow_video_window_destroy
//============================================================

void retro_window_info::complete_destroy()
{
	renderer_reset();

	// Release pointer grab and hide if needed
	show_pointer();
	release_pointer();

	// release all keys ...
	downcast<retro_osd_interface &>(machine().osd()).release_keys();
}

//============================================================
//  pick_best_mode
//============================================================

osd_dim retro_window_info::pick_best_mode()
{
   int minimum_width, minimum_height, target_width, target_height;

   osd_dim ret(0,0);

   // determine the minimum width/height for the selected target
   target()->compute_minimum_size(minimum_width, minimum_height);

   // use those as the target for now
   target_width = minimum_width * std::max(1, prescale());
   target_height = minimum_height * std::max(1, prescale());

   // if we're not stretching, allow some slop on the minimum since we can handle it
   {
      minimum_width -= 4;
      minimum_height -= 4;
   }

   ret = osd_dim(target_width, target_height);
   return ret;
}

//============================================================
//  sdlwindow_video_window_update
//  (main thread)
//============================================================

void retro_window_info::update()
{
	osd_ticks_t     event_wait_ticks;
	
	// adjust the cursor state
	//sdlwindow_update_cursor_state(machine, window);

	update_cursor_state();

	// if we're visible and running and not in the middle of a resize, draw
	if (target() != nullptr)
	{
		int temp_width, temp_height;
		float eff_aspect = view_aspect;

		eff_aspect = target()->current_view().effective_aspect();
		if (target()->orientation() & ORIENTATION_SWAP_XY)
			eff_aspect = 1.0f / eff_aspect;

		target()->compute_minimum_size(temp_width, temp_height);
		target()->set_keepaspect(false);

		if (alternate_renderer)
		{
			temp_width  = fb_width;
			temp_height = fb_height;
		}
		else if (svg_width)
		{
			/* Size limit for very big resolution SVG screens */
			temp_width  = (temp_width > svg_width) ? svg_width : temp_width;
			temp_height = (temp_height > svg_height) ? svg_height : temp_height;
		}

		if (temp_width != fb_width || temp_height != fb_height || eff_aspect != view_aspect)
		{
			m_minimum_dim = osd_dim(temp_width, temp_height);
			view_aspect   = eff_aspect;

			if (!alternate_renderer)
			{
				fb_width  = temp_width;
				fb_height = temp_height;

				/* Flip internal resolution for internal rotation */
				if ((!libretro_rotation_allow && internal_rotation_allow)
						&& target()->orientation() & ORIENTATION_SWAP_XY)
					monitor()->update_resolution(temp_height, temp_width);
				else
					monitor()->update_resolution(temp_width, temp_height);
			}

			monitor()->refresh();
			video_changed = 2;

			if (video_changed)
			{
				retro_aspect = view_aspect;
				if (((libretro_rotation_allow) || (!libretro_rotation_allow && !internal_rotation_allow))
						&& (machine().system().flags & ORIENTATION_SWAP_XY))
					retro_aspect = 1.0f / retro_aspect;

				/* Enlarge maximum geometry always */
				if (fb_width > max_width || fb_height > max_height)
				{
					max_width     = fb_width;
					max_height    = fb_height;
					video_changed = 1;
				}

				/* No reason to call av_info when not yet running */
				if (!retro_load_ok)
					video_changed = 0;
			}

			if (!this->m_fullscreen)
			{
				//Don't resize window without user interaction;
				//window_resize(blitwidth, blitheight);
			}
			else if (video_config.switchres)
			{
				osd_dim tmp = this->pick_best_mode();
				resize(tmp.width(), tmp.height());
			}
		}

		if (video_config.waitvsync && video_config.syncrefresh)
			event_wait_ticks = osd_ticks_per_second(); // block at most a second
		else
			event_wait_ticks = 0;

		/* Fast-forward shenanigans */
		if (libretro_supports_ff_override)
		{
			bool frontend_ff_enabled = false;

			environ_cb(RETRO_ENVIRONMENT_GET_FASTFORWARDING, &frontend_ff_enabled);

			/* Make sure throttle is disabled when frontend is fast-forwarding,
			 * only when throttle is enabled at startup */
			if (machine().options().throttle())
			{
				if (frontend_ff_enabled && machine().video().throttled())
					machine().video().set_throttled(false);
				else if (!frontend_ff_enabled && !machine().video().throttled())
					machine().video().set_throttled(true);
			}

			/* Make sure frontend is also fast-forwarding with the core */
			if (machine().video().fastforward() && !retro_fastforward)
			{
				retro_fastforwarding(true);
				retro_fastforward = true;
			}
			else if (!machine().video().fastforward() && retro_fastforward)
			{
				retro_fastforwarding(false);
				retro_fastforward = false;
			}
		}

		if (m_rendered_event.wait(event_wait_ticks))
		{
			const int update = 1;
			const screen_device *screen = screen_device_enumerator(machine().root_device()).byindex(index());

			// ensure the target bounds are up-to-date, and then get the primitives

			render_primitive_list &primlist = *renderer().get_primitives();

			// and redraw now

			// Some configurations require events to be polled in the worker thread
			//FIXME RETRO
			//	downcast< retro_osd_interface& >(machine().osd()).process_events_buf();

			// Check whether window has vector screens

			if ((screen != nullptr) && (screen->screen_type() == SCREEN_TYPE_VECTOR))
				renderer().set_flags(osd_renderer::FLAG_HAS_VECTOR_SCREEN);
			else
				renderer().clear_flags(osd_renderer::FLAG_HAS_VECTOR_SCREEN);

			/* Update retro_fps */
			if (screen)
			{
				float screen_refresh = screen->frame_period().as_hz();

				if (screen_refresh != retro_fps
						&& screen_refresh <= 120.0f
						&& screen_refresh >= 40.0f)
				{
					retro_fps     = screen_refresh;
					video_changed = 1;
				}
			}

			m_primlist = &primlist;

			// if no bitmap, just fill
			if (m_primlist == nullptr)
			{
			}
			// otherwise, render with our drawing system
			else
			{
				if (video_config.perftest)
					measure_fps(update);
				else
					renderer().draw(update);
			}

			/* all done, ready for next */
			m_rendered_event.set();
		}
	}
}


//============================================================
//  set_starting_view
//  (main thread)
//============================================================

void retro_window_info::set_starting_view(int index, const char *defview, const char *view)
{
	int viewindex;

	// choose non-auto over auto
	if (!strcmp(view, "auto") && !strcmp(defview, "auto") != 0)
		view = defview;

	// query the video system to help us pick a view
	viewindex = target()->configured_view(view, index, video_config.numscreens);

	// set the view
	target()->set_view(viewindex);
}


//============================================================
//  complete_create
//============================================================

int retro_window_info::complete_create()
{
	osd_dim temp(0,0);

	// clear out original mode. Needed on OSX
	if (fullscreen())
	{
		// default to the current mode exactly
		temp = monitor()->position_size().dim();

		// if we're allowed to switch resolutions, override with something better
		if (video_config.switchres)
			temp = pick_best_mode();
	}
	else if (m_windowed_dim.width() > 0)
	{
		// if we have a remembered size force the new window size to it
		temp = m_windowed_dim;
	}
	else if (m_startmaximized)
		temp = get_max_bounds(keepaspect());
	else
		temp = get_min_bounds(keepaspect());

	// create the window .....

	/* FIXME: On Ubuntu and potentially other Linux OS you should use
	 * to disable panning. This has to be done before every invocation of mame.
	 *
	 * xrandr --output HDMI-0 --panning 0x0+0+0 --fb 0x0
	 *
	 */
	//osd_printf_verbose("Enter sdl_info::create\n");

	m_extra_flags = 0;


#if 0
	// get monitor work area for centering
	osd_rect work = monitor()->usuable_position_size();
	//set_platform_window(retrowindow);
#endif
	renderer_create();

	// update monitor resolution after mode change to ensure proper pixel aspect
	monitor()->refresh();
	if (fullscreen() && video_config.switchres)
		monitor()->update_resolution(temp.width(), temp.height());

	// initialize the drawing backend
	if (renderer().create())
		return 1;

	return 0;
}


//============================================================
//  draw_video_contents
//  (window thread)
//============================================================

void retro_window_info::measure_fps(int update)
{
	const unsigned long frames_skip4fps = 100;
	static int64_t lastTime=0, sumdt=0, startTime=0;
	static unsigned long frames = 0;
	int64_t currentTime, t0;
	double dt;
	double tps;
	osd_ticks_t tps_t;

	tps_t = osd_ticks_per_second();
	tps = (double) tps_t;

	t0 = osd_ticks();

	renderer().draw(update);

	frames++;
	currentTime = osd_ticks();
	if(startTime==0||frames==frames_skip4fps)
		startTime=currentTime;
	if( frames>=frames_skip4fps )
		sumdt+=currentTime-t0;
	if( (currentTime-lastTime)>1L*osd_ticks_per_second() && frames>frames_skip4fps )
	{
		dt = (double) (currentTime-startTime) / tps; // in decimale sec.
		osd_printf_info("%6.2lfs, %4lu F, "
				"avrg game: %5.2lf FPS %.2lf ms/f, "
				"avrg video: %5.2lf FPS %.2lf ms/f, "
				"last video: %5.2lf FPS %.2lf ms/f\n",
			dt, frames-frames_skip4fps,
			(double)(frames-frames_skip4fps)/dt,                             // avrg game fps
			( (currentTime-startTime) / ((frames-frames_skip4fps)) ) * 1000.0 / osd_ticks_per_second(),
			(double)(frames-frames_skip4fps)/((double)(sumdt) / tps), // avrg vid fps
			( sumdt / ((frames-frames_skip4fps)) ) * 1000.0 / tps,
			1.0/((currentTime-t0) / osd_ticks_per_second()), // this vid fps
			(currentTime-t0) * 1000.0 / tps
		);
		lastTime = currentTime;
	}
}

int retro_window_info::wnd_extra_width()
{
	return m_fullscreen ? 0 : WINDOW_DECORATION_WIDTH;
}

int retro_window_info::wnd_extra_height()
{
	return m_fullscreen ? 0 : WINDOW_DECORATION_HEIGHT;
}


//============================================================
//  constrain_to_aspect_ratio
//  (window thread)
//============================================================

osd_rect retro_window_info::constrain_to_aspect_ratio(const osd_rect &rect, int adjustment)
{
	int32_t extrawidth = wnd_extra_width();
	int32_t extraheight = wnd_extra_height();
	int32_t propwidth, propheight;
	int32_t minwidth, minheight;
	int32_t maxwidth, maxheight;
	int32_t viswidth, visheight;
	int32_t adjwidth, adjheight;
	float pixel_aspect;

	// do not constrain aspect ratio for integer scaled views
	if (target()->scale_mode() != SCALE_FRACTIONAL)
		return rect;

	// get the pixel aspect ratio for the target monitor

	//FIXME: RETRO set it to 1 seem to correct the aspect. (alternate renderer)
	pixel_aspect = monitor()->pixel_aspect();

	// determine the proposed width/height
	propwidth = rect.width() - extrawidth;
	propheight = rect.height() - extraheight;

	// based on which edge we are adjusting, take either the width, height, or both as gospel
	// and scale to fit using that as our parameter
	switch (adjustment)
	{
		case WMSZ_BOTTOM:
		case WMSZ_TOP:
			target()->compute_visible_area(10000, propheight, pixel_aspect, target()->orientation(), propwidth, propheight);
			break;

		case WMSZ_LEFT:
		case WMSZ_RIGHT:
			target()->compute_visible_area(propwidth, 10000, pixel_aspect, target()->orientation(), propwidth, propheight);
			break;

		default:
			target()->compute_visible_area(propwidth, propheight, pixel_aspect, target()->orientation(), propwidth, propheight);
			break;
	}

	// get the minimum width/height for the current layout
	target()->compute_minimum_size(minwidth, minheight);

	// clamp against the absolute minimum
	propwidth = std::max(propwidth, MIN_WINDOW_DIM);
	propheight = std::max(propheight, MIN_WINDOW_DIM);

	// clamp against the minimum width and height
	propwidth = std::max(propwidth, minwidth);
	propheight = std::max(propheight, minheight);

	// clamp against the maximum (fit on one screen for full screen mode)
	if (m_fullscreen)
	{
		maxwidth = monitor()->position_size().width() - extrawidth;
		maxheight = monitor()->position_size().height() - extraheight;
	}
	else
	{
		maxwidth = monitor()->usuable_position_size().width() - extrawidth;
		maxheight = monitor()->usuable_position_size().height() - extraheight;

		// further clamp to the maximum width/height in the window
		if (m_win_config.width != 0)
			maxwidth = std::min(maxwidth, m_win_config.width + extrawidth);
		if (m_win_config.height != 0)
			maxheight = std::min(maxheight, m_win_config.height + extraheight);
	}

	// clamp to the maximum
	propwidth = std::min(propwidth, maxwidth);
	propheight = std::min(propheight, maxheight);

	// compute the visible area based on the proposed rectangle
	target()->compute_visible_area(propwidth, propheight, pixel_aspect, target()->orientation(), viswidth, visheight);

	// compute the adjustments we need to make
	adjwidth = (viswidth + extrawidth) - rect.width();
	adjheight = (visheight + extraheight) - rect.height();

	// based on which corner we're adjusting, constrain in different ways
	osd_rect ret(rect);

	switch (adjustment)
	{
		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMRIGHT:
		case WMSZ_RIGHT:
			ret = rect.resize(rect.width() + adjwidth, rect.height() + adjheight);
			break;

		case WMSZ_BOTTOMLEFT:
			ret = rect.move_by(-adjwidth, 0).resize(rect.width() + adjwidth, rect.height() + adjheight);
			break;

		case WMSZ_LEFT:
		case WMSZ_TOPLEFT:
		case WMSZ_TOP:
			ret = rect.move_by(-adjwidth, -adjheight).resize(rect.width() + adjwidth, rect.height() + adjheight);
			break;

		case WMSZ_TOPRIGHT:
			ret = rect.move_by(0, -adjheight).resize(rect.width() + adjwidth, rect.height() + adjheight);
			break;
}
	return ret;
}



//============================================================
//  get_min_bounds
//  (window thread)
//============================================================

osd_dim retro_window_info::get_min_bounds(int constrain)
{
	int32_t minwidth, minheight;

	//assert(GetCurrentThreadId() == window_threadid);

	// get the minimum target size
	target()->compute_minimum_size(minwidth, minheight);

	// expand to our minimum dimensions
	if (minwidth < MIN_WINDOW_DIM)
		minwidth = MIN_WINDOW_DIM;
	if (minheight < MIN_WINDOW_DIM)
		minheight = MIN_WINDOW_DIM;

	// account for extra window stuff
	minwidth += wnd_extra_width();
	minheight += wnd_extra_height();

	// if we want it constrained, figure out which one is larger
	if (constrain && target()->scale_mode() == SCALE_FRACTIONAL)
	{
		// first constrain with no height limit
		osd_rect test1(0,0,minwidth,10000);
		test1 = constrain_to_aspect_ratio(test1, WMSZ_BOTTOMRIGHT);

		// then constrain with no width limit
		osd_rect test2(0,0,10000,minheight);
		test2 = constrain_to_aspect_ratio(test2, WMSZ_BOTTOMRIGHT);

		// pick the larger
		if (test1.width() > test2.width())
		{
			minwidth = test1.width();
			minheight = test1.height();
		}
		else
		{
			minwidth = test2.width();
			minheight = test2.height();
		}
	}

	// remove extra window stuff
	minwidth -= wnd_extra_width();
	minheight -= wnd_extra_height();

	return osd_dim(minwidth, minheight);
}

//============================================================
//  get_size
//============================================================

osd_dim retro_window_info::get_size()
{
	return osd_dim(fb_width, fb_height);
}


//============================================================
//  get_max_bounds
//  (window thread)
//============================================================

osd_dim retro_window_info::get_max_bounds(int constrain)
{
	//assert(GetCurrentThreadId() == window_threadid);

	// compute the maximum client area
	// m_monitor->refresh();
	osd_rect maximum = monitor()->usuable_position_size();

	// clamp to the window's max
	int tempw = maximum.width();
	int temph = maximum.height();
	if (m_win_config.width != 0)
	{
		int temp = m_win_config.width + wnd_extra_width();
		if (temp < maximum.width())
			tempw = temp;
	}
	if (m_win_config.height != 0)
	{
		int temp = m_win_config.height + wnd_extra_height();
		if (temp < maximum.height())
			temph = temp;
	}

	maximum = maximum.resize(tempw, temph);

	// constrain to fit
	if (constrain && target()->scale_mode() == SCALE_FRACTIONAL)
		maximum = constrain_to_aspect_ratio(maximum, WMSZ_BOTTOMRIGHT);

	// remove extra window stuff
	maximum = maximum.resize(maximum.width() - wnd_extra_width(), maximum.height() - wnd_extra_height());

	return maximum.dim();
}

//============================================================
//  construction and destruction
//============================================================

retro_window_info::retro_window_info(
		running_machine &a_machine,
		render_module &renderprovider,
		int index,
		std::shared_ptr<osd_monitor_info> a_monitor,
		const osd_window_config *config)
	: osd_window_t(a_machine, renderprovider, index, std::move(a_monitor), *config)
	, m_startmaximized(0)
	// Following three are used by input code to defer resizes
	, m_minimum_dim(0, 0)
	, m_windowed_dim(0, 0)
	, m_rendered_event(0, 1)
	, m_extra_flags(0)
	, m_fullscreen(0)
	, m_mouse_captured(false)
	, m_mouse_hidden(false)
{
	//FIXME: these should be per_window in config-> or even better a bit set
	m_fullscreen = !video_config.windowed;
	m_prescale = video_config.prescale;

	m_windowed_dim = osd_dim(config->width, config->height);
	m_original_mode = new RETRO_DM_Wrapper;
}

retro_window_info::~retro_window_info()
{
	delete m_original_mode;
}

//============================================================
//  osd_set_aggressive_input_focus
//============================================================

void osd_set_aggressive_input_focus(bool aggressive_focus)
{
	// dummy implementation for now?
}
