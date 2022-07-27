// as_xml.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2010

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "pugixml/pugixml.hpp"
#include "bakeinflash/bakeinflash_root.h"
#include "as_bitmapdata.h"

namespace bakeinflash
{
  
	void	as_global_rectangle_ctor(const fn_call& fn)
	{
		if (fn.nargs >= 4)
		{
			rect r(fn.arg(0).to_float(), fn.arg(1).to_float(), fn.arg(2).to_float(), fn.arg(3).to_float());
			as_rectangle*	obj = new as_rectangle(r);
			fn.result->set_as_object(obj);
		}
	}

	void	as_global_bitmapdata_ctor(const fn_call& fn)
	{
		if (fn.nargs >= 2)
		{
			as_bitmapdata*	obj = new as_bitmapdata(fn.arg(0).to_int(), fn.arg(1).to_int());
			fn.result->set_as_object(obj);
		}
	}

	//fillRect(rect:Rectangle, color:Number) : Void 
	// Fills a rectangular area of pixels with a specified ARGB color.
	void	as_bitmapdata_fillrect(const fn_call& fn)
	{
		if (fn.nargs >= 2)
		{
			int k=1;

		}
	}

	as_bitmapdata::as_bitmapdata(int w, int h)
	{
		m_image = new image::rgba(w, h);
		builtin_member("fillRect", as_bitmapdata_fillrect);
	}

	as_bitmapdata::~as_bitmapdata()
	{
		delete m_image;
	}

} // end of bakeinflash namespace
