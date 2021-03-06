
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
#include "bchash.h"
#include "bcsignals.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "preferences.h"
#include "transportque.inc"

#include <string.h>





PluginClientThread::PluginClientThread(PluginClient *client)
 : Thread(1, 0, 0)
{
	this->client = client;
	window = 0;
	init_complete = new Condition(0, "PluginClientThread::init_complete");
}

PluginClientThread::~PluginClientThread()
{
//printf("PluginClientThread::~PluginClientThread %p %d\n", this, __LINE__);
	delete window;
//printf("PluginClientThread::~PluginClientThread %p %d\n", this, __LINE__);
	window = 0;
	delete init_complete;
}

void PluginClientThread::run()
{
	BC_DisplayInfo info;
	int result = 0;
	client->window_x = info.get_abs_cursor_x();
	client->window_y = info.get_abs_cursor_y();
	window = client->new_window();

	if(window)
	{
		window->create_objects();

/* Only set it here so tracking doesn't update it until everything is created. */
 		client->thread = this;
		init_complete->unlock();

//printf("PluginClientThread::run %p %d\n", this, __LINE__);
		result = window->run_window();
//printf("PluginClientThread::run %p %d\n", this, __LINE__);
		window->hide_window(1);
// Can't save defaults in the destructor because it's not called immediately
// after closing.
		if(client->defaults) client->save_defaults();
/* This is needed when the GUI is closed from itself */
		if(result) client->client_side_close();
	}
	else
// No window
	{
 		client->thread = this;
		init_complete->unlock();
	}
}

BC_WindowBase* PluginClientThread::get_window()
{
	return window;
}

PluginClient* PluginClientThread::get_client()
{
	return client;
}




PluginClientWindow::PluginClientWindow(PluginClient *client, 
	int w,
	int h,
	int min_w,
	int min_h,
	int allow_resize)
 : BC_Window(client->gui_string, 
	client->window_x - w / 2, 
	client->window_y - h / 2, 
//	0,
//	0,
	w, 
	h, 
	min_w, 
	min_h,
	allow_resize, 
	0,
	1)
{
	this->client = client;
}

PluginClientWindow::~PluginClientWindow()
{
}


int PluginClientWindow::close_event()
{
/* Set result to 1 to indicate a client side close */
	set_done(1);
	return 1;
}





PluginClient::PluginClient(PluginServer *server)
{
	reset();
	this->server = server;
	defaults = 0;
// Virtual functions don't work here.
}

PluginClient::~PluginClient()
{
// Delete the GUI thread.  The GUI must be hidden with hide_gui first.
	if(thread) 
	{
		thread->join();
		delete thread;
	}

// Virtual functions don't work here.
	if(defaults) delete defaults;
}

int PluginClient::reset()
{
	interactive = 0;
	show_initially = 0;
	wr = rd = 0;
	master_gui_on = 0;
	client_gui_on = 0;
	realtime_priority = 0;
	gui_string[0] = 0;
	total_in_buffers = 0;
	total_out_buffers = 0;
	source_position = 0;
	source_start = 0;
	total_len = 0;
	direction = PLAY_FORWARD;
	thread = 0;
}


void PluginClient::hide_gui()
{
	if(thread && thread->window)
	{
//printf("PluginClient::delete_thread %d\n", __LINE__);
/* This is needed when the GUI is closed from elsewhere than itself */
/* Since we now use autodelete, this is all that has to be done, thread will take care of itself ... */
/* Thread join will wait if this was not called from the thread itself or go on if it was */
		thread->window->lock_window("PluginClient::hide_gui");
		thread->window->set_done(0);
//printf("PluginClient::hide_gui %d thread->window=%p\n", __LINE__, thread->window);
		thread->window->unlock_window();
//printf("PluginClient::delete_thread %d\n", __LINE__);
	}
}

// For realtime plugins initialize buffers
int PluginClient::plugin_init_realtime(int realtime_priority, 
	int total_in_buffers,
	int buffer_size)
{

// Get parameters for all
	master_gui_on = get_gui_status();



// get parameters depending on video or audio
	init_realtime_parameters();

	smp = server->preferences->processors - 1;

	this->realtime_priority = realtime_priority;

	this->total_in_buffers = this->total_out_buffers = total_in_buffers;

	this->out_buffer_size = this->in_buffer_size = buffer_size;

	return 0;
}

int PluginClient::plugin_start_loop(int64_t start, 
	int64_t end, 
	int64_t buffer_size, 
	int total_buffers)
{
	this->source_start = start;
	this->total_len = end - start;
	this->start = start;
	this->end = end;
	this->in_buffer_size = this->out_buffer_size = buffer_size;
	this->total_in_buffers = this->total_out_buffers = total_buffers;
	start_loop();
	return 0;
}

int PluginClient::plugin_process_loop()
{
	return process_loop();
}

int PluginClient::plugin_stop_loop()
{
	return stop_loop();
}

MainProgressBar* PluginClient::start_progress(char *string, int64_t length)
{
	return server->start_progress(string, length);
}



int PluginClient::plugin_get_parameters()
{
	int result = get_parameters();
	return result;
}

// ========================= main loop

int PluginClient::is_multichannel() { return 0; }
int PluginClient::is_synthesis() { return 0; }
int PluginClient::is_realtime() { return 0; }
int PluginClient::is_fileio() { return 0; }
int PluginClient::delete_buffer_ptrs() { return 0; }
const char* PluginClient::plugin_title() { return _("Untitled"); }
VFrame* PluginClient::new_picon() 
{ 
	printf("PluginClient::new_picon not defined in %s\n", plugin_title());
	return 0; 
}
Theme* PluginClient::new_theme() { return 0; }

int PluginClient::load_configuration()
{
	return 0;
}

Theme* PluginClient::get_theme()
{
	return server->get_theme();
}

int PluginClient::show_gui()
{
	load_configuration();
	thread = new PluginClientThread(this);
	thread->start();
	thread->init_complete->lock("PluginClient::show_gui");
// Must wait before sending any hide_gui
	if(thread->window)
	{
		thread->window->init_wait();
	}
	else
	{
		return 1;
	}
	return 0;
}

void PluginClient::raise_window()
{
	if(thread)
	{
		thread->window->lock_window("PluginClient::raise_window");
		thread->window->raise_window();
		thread->window->flush();
		thread->window->unlock_window();
	}
}

int PluginClient::set_string()
{
	if(thread)
	{
		thread->window->lock_window("PluginClient::set_string");
		thread->window->set_title(gui_string);
		thread->window->unlock_window();
	}
	return 0;
}

int PluginClient::is_audio() { return 0; }
int PluginClient::is_video() { return 0; }
int PluginClient::is_theme() { return 0; }
int PluginClient::uses_gui() { return 1; }
int PluginClient::is_transition() { return 0; }
int PluginClient::load_defaults() 
{ 
//	printf("PluginClient::load_defaults undefined in %s.\n", plugin_title());
	return 0; 
}
int PluginClient::save_defaults() 
{ 
//	printf("PluginClient::save_defaults undefined in %s.\n", plugin_title());
	return 0; 
}
BC_Hash* PluginClient::get_defaults()
{
	return defaults;
}
PluginClientThread* PluginClient::get_thread()
{
	return thread;
}

BC_WindowBase* PluginClient::new_window() 
{ 
	printf("PluginClient::new_window undefined in %s.\n", plugin_title());
	return 0; 
}
int PluginClient::get_parameters() { return 0; }
int PluginClient::get_samplerate() { return get_project_samplerate(); }
double PluginClient::get_framerate() { return get_project_framerate(); }
int PluginClient::init_realtime_parameters() { return 0; }
int PluginClient::delete_nonrealtime_parameters() { return 0; }
int PluginClient::start_loop() { return 0; };
int PluginClient::process_loop() { return 0; };
int PluginClient::stop_loop() { return 0; };

void PluginClient::set_interactive()
{
	interactive = 1;
}

int64_t PluginClient::get_in_buffers(int64_t recommended_size)
{
	return recommended_size;
}

int64_t PluginClient::get_out_buffers(int64_t recommended_size)
{
	return recommended_size;
}

int PluginClient::get_gui_status()
{
	return server->get_gui_status();
}

int PluginClient::start_plugin()
{
	printf(_("No processing defined for this plugin.\n"));
	return 0;
}

// close event from client side
void PluginClient::client_side_close()
{
// Last command executed
	server->client_side_close();
}

int PluginClient::stop_gui_client()
{
	if(!client_gui_on) return 0;
	client_gui_on = 0;
	return 0;
}

int PluginClient::get_project_samplerate()
{
	return server->get_project_samplerate();
}

double PluginClient::get_project_framerate()
{
	return server->get_project_framerate();
}


void PluginClient::update_display_title()
{
	server->generate_display_title(gui_string);
	set_string();
}

char* PluginClient::get_gui_string()
{
	return gui_string;
}


char* PluginClient::get_path()
{
	return server->path;
}

char* PluginClient::get_plugin_dir()
{
	return server->preferences->plugin_dir;
}

int PluginClient::set_string_client(char *string)
{
	strcpy(gui_string, string);
	set_string();
	return 0;
}


int PluginClient::get_interpolation_type()
{
	return server->get_interpolation_type();
}


float PluginClient::get_red()
{
	if(server->mwindow)
		return server->mwindow->edl->local_session->red;
	else
	if(server->edl)
		return server->edl->local_session->red;
	else
		return 0;
}

float PluginClient::get_green()
{
	if(server->mwindow)
		return server->mwindow->edl->local_session->green;
	else
	if(server->edl)
		return server->edl->local_session->green;
	else
		return 0;
}

float PluginClient::get_blue()
{
	if(server->mwindow)
		return server->mwindow->edl->local_session->blue;
	else
	if(server->edl)
		return server->edl->local_session->blue;
	else
		return 0;
}



int64_t PluginClient::get_source_position()
{
	return source_position;
}

int64_t PluginClient::get_source_start()
{
	return source_start;
}

int64_t PluginClient::get_total_len()
{
	return total_len;
}

int PluginClient::get_direction()
{
	return direction;
}


int64_t PluginClient::local_to_edl(int64_t position)
{
	return position;
}

int64_t PluginClient::edl_to_local(int64_t position)
{
	return position;
}

int PluginClient::get_use_opengl()
{
	return server->get_use_opengl();
}

int PluginClient::get_total_buffers()
{
	return total_in_buffers;
}

int PluginClient::get_buffer_size()
{
	return in_buffer_size;
}

int PluginClient::get_project_smp()
{
	return smp;
}

const char* PluginClient::get_defaultdir()
{
	return BCASTDIR;
}


int PluginClient::send_hide_gui()
{
// Stop the GUI server and delete GUI messages
	client_gui_on = 0;
	return 0;
}

int PluginClient::send_configure_change()
{
#ifdef USE_KEYFRAME_SPANNING
	KeyFrame keyframe;
	if(server->mwindow)
		server->mwindow->undo->update_undo_before("tweek", this);

	save_data(&keyframe);
	server->apply_keyframe(&keyframe);

#else
	KeyFrame* keyframe = server->get_keyframe();
	if(server->mwindow)
		server->mwindow->undo->update_undo_before("tweek", this);

// Call save routine in plugin
	save_data(keyframe);

#endif


	if(server->mwindow)
		server->mwindow->undo->update_undo_after("tweek", LOAD_AUTOMATION);
	server->sync_parameters();
	return 0;
}


KeyFrame* PluginClient::get_prev_keyframe(int64_t position, int is_local)
{
	if(is_local) position = local_to_edl(position);
	return server->get_prev_keyframe(position);
}

KeyFrame* PluginClient::get_next_keyframe(int64_t position, int is_local)
{
	if(is_local) position = local_to_edl(position);
	return server->get_next_keyframe(position);
}

void PluginClient::get_camera(float *x, float *y, float *z, int64_t position)
{
	server->get_camera(x, y, z, position, direction);
}

void PluginClient::get_projector(float *x, float *y, float *z, int64_t position)
{
	server->get_projector(x, y, z, position, direction);
}


EDLSession* PluginClient::get_edlsession()
{
	if(server->edl) 
		return server->edl->session;
	return 0;
}

int PluginClient::gui_open()
{
	return server->gui_open();
}
