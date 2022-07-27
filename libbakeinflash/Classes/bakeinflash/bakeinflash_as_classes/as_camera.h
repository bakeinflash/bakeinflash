// as_netstream.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_CAMERA_H
#define BAKEINFLASH_CAMERA_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/tu_queue.h"
#include "bakeinflash/bakeinflash_object.h"
#include "as_video_stream.h"

// USB video
#if TU_CONFIG_LINK_TO_LIBUVC == 1
	#include <libuvc/libuvc.h>
#endif

namespace bakeinflash
{
	
	struct as_camera : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_CAMERA };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
	
		as_camera();
		virtual ~as_camera();
		void notify_image(bool rc);
		as_object* get();
		void take_image(character* target, float quality);
	};

	struct as_camera_object : public as_video_stream
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_CAMERA_OBJECT };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}
	
		as_camera_object();
		virtual ~as_camera_object();

		virtual int get_width() const;
		virtual int get_height() const;

		virtual Uint8* get_video_data();
		virtual void free_video_data(Uint8* data);
		virtual void set_video_data(Uint8* data);

		virtual bool open_stream(const char* url);
		virtual void close_stream();
		virtual void	advance(float delta_time) {};

#ifdef iOS
    virtual video_pixel_format::code get_pixel_format() { return video_pixel_format::RGBA; };
#else
    virtual video_pixel_format::code get_pixel_format();
#endif
    
		void run();


		// RGBA video frame 
		Uint8* m_video_data;
		tu_mutex m_lock_video;

		smart_ptr<tu_thread> m_thread;
		bool m_go;

		// libuvc
#if TU_CONFIG_LINK_TO_LIBUVC == 1
		uvc_device_t* m_uvc_dev;
		uvc_device_handle_t* m_uvc_devh;
		uvc_context_t* m_uvc_ctx;
#endif

	};

	as_object* camera_init();

} // end of bakeinflash namespace

// bakeinflash_NETSTREAM_H
#endif

