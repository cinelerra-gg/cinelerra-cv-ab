
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "../colors/plugincolors.h"
#include "pluginvclient.h"
#include "fonts.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>




#define FLOAT_MIN -0.1
#define FLOAT_MAX 1.1
#define WAVEFORM_DIVISIONS 12
#define VECTORSCOPE_DIVISIONS 12


class VideoScopeEffect;
class VideoScopeEngine;


class VideoScopeConfig
{
public:
	VideoScopeConfig();
};

class VideoScopeWaveform : public BC_SubWindow
{
public:
	VideoScopeWaveform(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h);
	VideoScopeEffect *plugin;
};


class VideoScopeVectorscope : public BC_SubWindow
{
public:
	VideoScopeVectorscope(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h);
	VideoScopeEffect *plugin;
};

class VideoScopeWindow : public PluginClientWindow
{
public:
	VideoScopeWindow(VideoScopeEffect *plugin);
	~VideoScopeWindow();

	void calculate_sizes(int w, int h);
	void create_objects();
	int resize_event(int w, int h);
	void allocate_bitmaps();
	void draw_overlays();

	VideoScopeEffect *plugin;
	VideoScopeWaveform *waveform;
	VideoScopeVectorscope *vectorscope;
	BC_Bitmap *waveform_bitmap;
	BC_Bitmap *vector_bitmap;

	int vector_x, vector_y, vector_w, vector_h;
	int wave_x, wave_y, wave_w, wave_h;
};





class VideoScopePackage : public LoadPackage
{
public:
	VideoScopePackage();
	int row1, row2;
};


class VideoScopeUnit : public LoadClient
{
public:
	VideoScopeUnit(VideoScopeEffect *plugin, VideoScopeEngine *server);
	void process_package(LoadPackage *package);
	VideoScopeEffect *plugin;
	YUV yuv;
};

class VideoScopeEngine : public LoadServer
{
public:
	VideoScopeEngine(VideoScopeEffect *plugin, int cpus);
	~VideoScopeEngine();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	VideoScopeEffect *plugin;
};

class VideoScopeEffect : public PluginVClient
{
public:
	VideoScopeEffect(PluginServer *server);
	~VideoScopeEffect();


	PLUGIN_CLASS_MEMBERS(VideoScopeConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void render_gui(void *input);

	int w, h;
	VFrame *input;
	VideoScopeEngine *engine;
};













VideoScopeConfig::VideoScopeConfig()
{
}










VideoScopeWaveform::VideoScopeWaveform(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
}


VideoScopeVectorscope::VideoScopeVectorscope(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
}







VideoScopeWindow::VideoScopeWindow(VideoScopeEffect *plugin)
 : PluginClientWindow(plugin, 
	plugin->w, 
	plugin->h, 
	50, 
	50, 
	1)
{
	this->plugin = plugin;
	waveform_bitmap = 0;
	vector_bitmap = 0;
}

VideoScopeWindow::~VideoScopeWindow()
{

	if(waveform_bitmap) delete waveform_bitmap;
	if(vector_bitmap) delete vector_bitmap;
}

void VideoScopeWindow::calculate_sizes(int w, int h)
{
	wave_x = 30;
	wave_y = 10;
	wave_w = w / 2 - 5 - wave_x;
	wave_h = h - 20 - wave_y;
	vector_x = w / 2 + 30;
	vector_y = 10;
	vector_w = w - 10 - vector_x;
	vector_h = h - 10 - vector_y;
}

void VideoScopeWindow::create_objects()
{
	calculate_sizes(get_w(), get_h());

	add_subwindow(waveform = new VideoScopeWaveform(plugin, 
		wave_x, 
		wave_y, 
		wave_w, 
		wave_h));
	add_subwindow(vectorscope = new VideoScopeVectorscope(plugin, 
		vector_x, 
		vector_y, 
		vector_w, 
		vector_h));
	allocate_bitmaps();
	draw_overlays();

	show_window();
	flush();
	
}



int VideoScopeWindow::resize_event(int w, int h)
{

	clear_box(0, 0, w, h);
	plugin->w = w;
	plugin->h = h;
	calculate_sizes(w, h);
	waveform->reposition_window(wave_x, wave_y, wave_w, wave_h);
	vectorscope->reposition_window(vector_x, vector_y, vector_w, vector_h);
	waveform->clear_box(0, 0, wave_w, wave_h);
	vectorscope->clear_box(0, 0, vector_w, vector_h);
	allocate_bitmaps();
	draw_overlays();
	flash();

	return 1;
}

void VideoScopeWindow::allocate_bitmaps()
{
	if(waveform_bitmap) delete waveform_bitmap;
	if(vector_bitmap) delete vector_bitmap;

	waveform_bitmap = new_bitmap(wave_w, wave_h);
	vector_bitmap = new_bitmap(vector_w, vector_h);
}

void VideoScopeWindow::draw_overlays()
{
	set_color(GREEN);
	set_font(SMALLFONT);

// Waveform overlay
	for(int i = 0; i <= WAVEFORM_DIVISIONS; i++)
	{
		int y = wave_h * i / WAVEFORM_DIVISIONS;
		int text_y = y + wave_y + get_text_ascent(SMALLFONT) / 2;
		int x = wave_x - 20;
		char string[BCTEXTLEN];
		sprintf(string, "%d", 
			(int)((FLOAT_MAX - 
			i * (FLOAT_MAX - FLOAT_MIN) / WAVEFORM_DIVISIONS) * 100));
		draw_text(x, text_y, string);

		waveform->draw_line(0, 
			CLAMP(y, 0, waveform->get_h() - 1), 
			wave_w, 
			CLAMP(y, 0, waveform->get_h() - 1));
//waveform->draw_rectangle(0, 0, wave_w, wave_h);
	}



// Vectorscope overlay
	int radius = MIN(vector_w / 2, vector_h / 2);
	for(int i = 1; i <= VECTORSCOPE_DIVISIONS - 1; i += 2)
	{
		int x = vector_w / 2 - radius * i / VECTORSCOPE_DIVISIONS;
		int y = vector_h / 2 - radius * i / VECTORSCOPE_DIVISIONS;
		int text_x = vector_x - 20;
		int text_y = y + vector_y + get_text_ascent(SMALLFONT) / 2;
		int w = radius * i / VECTORSCOPE_DIVISIONS * 2;
		int h = radius * i / VECTORSCOPE_DIVISIONS * 2;
		char string[BCTEXTLEN];
		
		sprintf(string, "%d", 
			(int)((FLOAT_MIN + 
				(FLOAT_MAX - FLOAT_MIN) / VECTORSCOPE_DIVISIONS * i) * 100));
		draw_text(text_x, text_y, string);
		vectorscope->draw_circle(x, y, w, h);
//vectorscope->draw_rectangle(0, 0, vector_w, vector_h);
	}
// 	vectorscope->draw_circle(vector_w / 2 - radius, 
// 		vector_h / 2 - radius, 
// 		radius * 2, 
// 		radius * 2);
	set_font(MEDIUMFONT);

	waveform->flash();
	vectorscope->flash();
	flush();
}














REGISTER_PLUGIN(VideoScopeEffect)






VideoScopeEffect::VideoScopeEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	w = 640;
	h = 260;
	
}

VideoScopeEffect::~VideoScopeEffect()
{
	

	if(engine) delete engine;
}



const char* VideoScopeEffect::plugin_title() { return N_("VideoScope"); }
int VideoScopeEffect::is_realtime() { return 1; }

int VideoScopeEffect::load_configuration()
{
	return 0;
}

NEW_PICON_MACRO(VideoScopeEffect)

NEW_WINDOW_MACRO(VideoScopeEffect, VideoScopeWindow)

int VideoScopeEffect::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%svideoscope.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	w = defaults->get("W", w);
	h = defaults->get("H", h);
	return 0;
}

int VideoScopeEffect::save_defaults()
{
	defaults->update("W", w);
	defaults->update("H", h);
	defaults->save();
	return 0;
}

int VideoScopeEffect::process_realtime(VFrame *input, VFrame *output)
{

	send_render_gui(input);
//printf("VideoScopeEffect::process_realtime 1\n");
	if(input->get_rows()[0] != output->get_rows()[0])
		output->copy_from(input);
	return 1;
}

void VideoScopeEffect::render_gui(void *input)
{
	if(thread)
	{
		VideoScopeWindow *window = ((VideoScopeWindow*)thread->window);
		window->lock_window();

//printf("VideoScopeEffect::process_realtime 1\n");
		this->input = (VFrame*)input;
//printf("VideoScopeEffect::process_realtime 1\n");


		if(!engine)
		{
			engine = new VideoScopeEngine(this, 
				(PluginClient::smp + 1));
		}

//printf("VideoScopeEffect::process_realtime 1 %d\n", PluginClient::smp);
// Clear bitmaps
		bzero(window->waveform_bitmap->get_data(), 
			window->waveform_bitmap->get_h() * 
			window->waveform_bitmap->get_bytes_per_line());
		bzero(window->vector_bitmap->get_data(), 
			window->vector_bitmap->get_h() * 
			window->vector_bitmap->get_bytes_per_line());

		engine->process_packages();
//printf("VideoScopeEffect::process_realtime 2\n");
//printf("VideoScopeEffect::process_realtime 1\n");

		window->waveform->draw_bitmap(window->waveform_bitmap, 
			1,
			0,
			0);

//printf("VideoScopeEffect::process_realtime 1\n");
		window->vectorscope->draw_bitmap(window->vector_bitmap, 
			1,
			0,
			0);


		window->draw_overlays();


		window->unlock_window();
	}
}





VideoScopePackage::VideoScopePackage()
 : LoadPackage()
{
}






VideoScopeUnit::VideoScopeUnit(VideoScopeEffect *plugin, 
	VideoScopeEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}


#define INTENSITY(p) ((unsigned int)(((p)[0]) * 77+ \
									((p)[1] * 150) + \
									((p)[2] * 29)) >> 8)


static void draw_point(unsigned char **rows, 
	int color_model, 
	int x, 
	int y, 
	int r, 
	int g, 
	int b)
{
	switch(color_model)
	{
		case BC_BGR8888:
		{
			unsigned char *pixel = rows[y] + x * 4;
			pixel[0] = b;
			pixel[1] = g;
			pixel[2] = r;
			break;
		}
		case BC_BGR888:
			break;
		case BC_RGB565:
		{
			unsigned char *pixel = rows[y] + x * 2;
			pixel[0] = (r & 0xf8) | (g >> 5);
			pixel[1] = ((g & 0xfc) << 5) | (b >> 3);
			break;
		}
		case BC_BGR565:
			break;
		case BC_RGB8:
			break;
	}
}



#define VIDEOSCOPE(type, temp_type, max, components, use_yuv) \
{ \
	for(int i = pkg->row1; i < pkg->row2; i++) \
	{ \
		type *in_row = (type*)plugin->input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			type *in_pixel = in_row + j * components; \
			float intensity; \
 \
/* Analyze pixel */ \
			if(use_yuv) intensity = (float)*in_pixel / max; \
 \
			float h, s, v; \
			temp_type r, g, b; \
			if(use_yuv) \
			{ \
				if(sizeof(type) == 2) \
				{ \
					yuv.yuv_to_rgb_16(r, \
						g, \
						b, \
						in_pixel[0], \
						in_pixel[1], \
						in_pixel[2]); \
				} \
				else \
				{ \
					yuv.yuv_to_rgb_8(r, \
						g, \
						b, \
						in_pixel[0], \
						in_pixel[1], \
						in_pixel[2]); \
				} \
			} \
			else \
			{ \
				r = in_pixel[0]; \
				g = in_pixel[1]; \
				b = in_pixel[2]; \
			} \
 \
			HSV::rgb_to_hsv((float)r / max, \
					(float)g / max, \
					(float)b / max, \
					h, \
					s, \
					v); \
 \
/* Calculate waveform */ \
			if(!use_yuv) intensity = v; \
			intensity = (intensity - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN) * \
				waveform_h; \
			int y = waveform_h - (int)intensity; \
			int x = j * waveform_w / w; \
			if(x >= 0 && x < waveform_w && y >= 0 && y < waveform_h) \
				draw_point(waveform_rows, \
					waveform_cmodel, \
					x, \
					y, \
					0xff, \
					0xff, \
					0xff); \
 \
/* Calculate vectorscope */ \
			float adjacent = cos(h / 360 * 2 * M_PI); \
			float opposite = sin(h / 360 * 2 * M_PI); \
			x = (int)(vector_w / 2 +  \
				adjacent * (s - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN) * radius); \
 \
			y = (int)(vector_h / 2 -  \
				opposite * (s - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN) * radius); \
 \
 \
			CLAMP(x, 0, vector_w - 1); \
			CLAMP(y, 0, vector_h - 1); \
			if(sizeof(type) == 2) \
			{ \
				r /= 256; \
				g /= 256; \
				b /= 256; \
			} \
			else \
			if(sizeof(type) == 4) \
			{ \
				r = CLIP(r, 0, 1) * 0xff; \
				g = CLIP(g, 0, 1) * 0xff; \
				b = CLIP(b, 0, 1) * 0xff; \
			} \
 \
			draw_point(vector_rows, \
				vector_cmodel, \
				x, \
				y, \
				(int)r, \
				(int)g, \
				(int)b); \
 \
		} \
	} \
}

void VideoScopeUnit::process_package(LoadPackage *package)
{
	VideoScopeWindow *window = (VideoScopeWindow*)plugin->thread->window;
	VideoScopePackage *pkg = (VideoScopePackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	int waveform_h = window->wave_h;
	int waveform_w = window->wave_w;
	int waveform_cmodel = window->waveform_bitmap->get_color_model();
	unsigned char **waveform_rows = window->waveform_bitmap->get_row_pointers();
	int vector_h = window->vector_bitmap->get_h();
	int vector_w = window->vector_bitmap->get_w();
	int vector_cmodel = window->vector_bitmap->get_color_model();
	unsigned char **vector_rows = window->vector_bitmap->get_row_pointers();
	float radius = MIN(vector_w / 2, vector_h / 2);

	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			VIDEOSCOPE(unsigned char, int, 0xff, 3, 0)
			break;

		case BC_RGB_FLOAT:
			VIDEOSCOPE(float, float, 1, 3, 0)
			break;

		case BC_YUV888:
			VIDEOSCOPE(unsigned char, int, 0xff, 3, 1)
			break;

		case BC_RGB161616:
			VIDEOSCOPE(uint16_t, int, 0xffff, 3, 0)
			break;

		case BC_YUV161616:
			VIDEOSCOPE(uint16_t, int, 0xffff, 3, 1)
			break;

		case BC_RGBA8888:
			VIDEOSCOPE(unsigned char, int, 0xff, 4, 0)
			break;

		case BC_RGBA_FLOAT:
			VIDEOSCOPE(float, float, 1, 4, 0)
			break;

		case BC_YUVA8888:
			VIDEOSCOPE(unsigned char, int, 0xff, 4, 1)
			break;

		case BC_RGBA16161616:
			VIDEOSCOPE(uint16_t, int, 0xffff, 4, 0)
			break;

		case BC_YUVA16161616:
			VIDEOSCOPE(uint16_t, int, 0xffff, 4, 1)
			break;
	}
}






VideoScopeEngine::VideoScopeEngine(VideoScopeEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

VideoScopeEngine::~VideoScopeEngine()
{
}

void VideoScopeEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		VideoScopePackage *pkg = (VideoScopePackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}


LoadClient* VideoScopeEngine::new_client()
{
	return new VideoScopeUnit(plugin, this);
}

LoadPackage* VideoScopeEngine::new_package()
{
	return new VideoScopePackage;
}

