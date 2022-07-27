// as_textformat.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_textformat.h"

namespace bakeinflash
{

	// TextFormat([font:String], [size:Number], [color:Number], [bold:Boolean], 
	// [italic:Boolean], [underline:Boolean], [url:String], [target:String], 
	// [align:String], [leftMargin:Number], [rightMargin:Number], [indent:Number],
	// [leading:Number])

	static tu_string as_global_textformat_ctor_paramnames[] =
	{"font", "size", "color", "bold", "italic", "underline", "url", "target",
	 "align", "leftMargin", "rightMargin", "indent", "leading"};

	void	as_global_textformat_ctor(const fn_call& fn)
	{
		smart_ptr<as_textformat>	obj = new as_textformat();

		assert(unsigned(fn.nargs) <= TU_ARRAYSIZE(as_global_textformat_ctor_paramnames));

		// TODO: check parameters type
		for (int i = 0; i < fn.nargs; i++)
		{
			obj->set_member(as_global_textformat_ctor_paramnames[i], fn.arg(i));
		}

		fn.result->set_as_object(obj.get());
	}

	as_textformat::as_textformat()
	{
	}


};	// bakeinflash
