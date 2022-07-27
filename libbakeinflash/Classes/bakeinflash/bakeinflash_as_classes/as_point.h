// as_point.h	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_AS_POINT_H
#define BAKEINFLASH_AS_POINT_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_character.h"

namespace bakeinflash
{

	void	as_global_point_ctor(const fn_call& fn);

	struct as_point : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_POINT };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_point(float x, float y);

		 virtual bool	set_member(const tu_string& name, const as_value& val);
		 virtual bool	get_member(const tu_string& name, as_value* val);

		bakeinflash::point m_point;
	};

}	// end namespace bakeinflash


#endif // bakeinflash_AS_POINT_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
