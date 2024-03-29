// bakeinflash_character.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_CHARACTER_H
#define BAKEINFLASH_CHARACTER_H

#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_types.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_function.h"
#include "bakeinflash/bakeinflash_abc.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include <stdarg.h>

namespace bakeinflash
{
	struct root;
	struct swf_event;
	struct as_mcloader;

	// A character_def is the immutable data representing the template of a
	// movie element.
	//
	// @@ This is not really a public interface.  It's here so it
	// can be mixed into movie_definition, movie_definition_sub,
	// and sprite_definition, without using multiple inheritance.
	struct character_def : public as_object_interface
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_CHARACTER_DEF };
		virtual bool is(int class_id) const
		{
			return m_class_id == class_id;
		}

		character_def()	:
			m_id(-1)
		{
		}

		virtual ~character_def() {}

		virtual void	display(character* instance_info) {}
		virtual bool	point_test_local(float x, float y) { return false; }
		virtual void get_bound(rect* bound) { assert(0); };

		// Should stick the result in a smart_ptr immediately.
		virtual character*	create_character_instance(character* parent, int id);	// default is to make a generic_character

		//
		// Caching.
		//

		virtual void	output_cached_data(tu_file* out, const cache_options& options) {}
		virtual void	input_cached_data(tu_file* in) {}

		// for definetext, definetext2 & defineedittext tags
		virtual void	csm_textsetting(stream* in, int tag_type) { assert(0); };

		void set_registered_class_constructor(const as_value & value);
		void instanciate_registered_class(character * ch);

		// unzip jpeg tags
		virtual void unzip_bitmaps() {} 

		// remove bitmap_info, textures
		virtual void clear_bitmaps() {}

		virtual void	load_bitmaps(array < smart_ptr<bitmap_info> >*	m_bitmap_hash) {}

		private:

		int	m_id;
		weak_ptr<as_function> m_registered_class_constructor;

	};

	// Information about how to display a character.
	struct display_info
	{
		int	m_depth;
		cxform	m_color_transform;
		matrix	m_matrix;
		float	m_ratio;
		Uint16	m_clip_depth;

		display_info() :
			m_depth(0),
			m_ratio(0.0f),
			m_clip_depth(0)
		{
		}

		void	concatenate(const display_info& di)
		// Concatenate the transforms from di into our
		// transforms.
		{
			m_depth = di.m_depth;
			m_color_transform.concatenate(di.m_color_transform);
			m_matrix.concatenate(di.m_matrix);
			m_ratio = di.m_ratio;
			m_clip_depth = di.m_clip_depth;
		}
	};

	// character is a live, stateful instance of a character_def.
	// It represents a single active element in a movie.
	// internal interface
	struct character : public as_object
	{

		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_CHARACTER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		int		m_id;
		weak_ptr<character>		m_parent;
		tu_string	m_name;
		int		m_depth;
		cxform		m_color_transform;
		matrix		m_matrix;
		float		m_ratio;
		Uint16		m_clip_depth;
		Uint8       m_blend_mode;
		bool		m_visible;

		struct drag_state
		{
		private:
			character*	m_character;
			bool		m_lock_center;
			bool		m_bound;
			rect		m_bound_rect;
			float		m_offset_x;
			float		m_offset_y;

		public:
			drag_state() :
				m_character(NULL),
				m_lock_center(false), 
				m_bound(false),
				m_offset_x(0.0f),
				m_offset_y(0.0f)
			{
			}

			void Reset()
			{
				m_bound = false;
				m_character = NULL;
				m_lock_center = false;
			}

			bool IsLockCentered() const
			{
				return m_lock_center;
			}

			void SetLockCentered(bool locked)
			{
				m_lock_center = locked;
			}

			void SetOffset(float x, float y)
			{
				m_offset_x = x;
				m_offset_y = y;
			}

			float OffsetX() const { return m_offset_x; }
			float OffsetY() const { return m_offset_y; }

			bool GetBounds(rect* bounds) const
			{
				if (m_bound)
				{
					*bounds = m_bound_rect;
					return true;
				}
				return false;
			}
			
			void SetBounds(float x0, float y0, float x1, float y1)
			{
				m_bound_rect.m_x_min = x0;
				m_bound_rect.m_x_max = x1;
				m_bound_rect.m_y_min = y0;
				m_bound_rect.m_y_max = y1;
				m_bound = true;
			}

			character* GetCharacter() const
			{
				return m_character;
			}

			void SetCharacter(character* ch)
			{
				m_character = ch;
			}
		};

		enum play_state
		{
			PLAY,
			STOP
		};

		character(character* parent, int id);

		//
		// Mouse/Button interface.
		//
		virtual bool get_topmost_mouse_entity( character * &te, float x, float y) { return false; }

		// The host app uses this to tell the movie where the
		// user's mouse pointer is.
		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		{
			get_parent()->get_mouse_state(x, y, buttons); 
		}

		virtual bool	get_track_as_menu() const { return false; }

		virtual float			get_pixel_scale() const { return 1.0f; }
		virtual character*		get_character(int id) { return NULL; }

		virtual movie_definition*	get_movie_definition()
		{
			character* ch = get_parent();
			if (ch)
			{
				return ch->get_movie_definition();
			}
			return NULL;
		}

		virtual character_def*	find_exported_resource(const tu_string& symbol)
		{
			character* ch = get_parent();
			if (ch)
			{
				return ch->find_exported_resource(symbol);
			}
			return NULL;
		}


		// @@ do we want a version that returns a number?

		// ActionScript method call.  Return value points to a
		// static string buffer with the result; caller should
		// use the value immediately before making more calls
		// to bakeinflash.

		// NOT THREAD SAFE!!!
		//
		// DO NOT USE TO CALL CLASS MEMBER!!!
		//
		// method_name is the name of the method (possibly namespaced).
		//
		// method_arg_fmt is a printf-style declaration of
		// the method call, where the arguments are
		// represented by { %d, %s, %f, %ls }, followed by the
		// vararg list of actual arguments.
		//
		// E.g.
		//
		// m->call_method("path.to.method_name", "%d, %s, %f", i, "hello", 2.7f);
		//
		// The format args are a small subset of printf, namely:
		//
		// %d -- integer arg
		// %s -- 0-terminated char* string arg
		// %ls -- 0-terminated wchar_t* string arg
		// %f -- float/double arg
		//
		// Whitespace and commas in the format string are ignored.
		//
		// This is not an ActionScript language parser, it
		// doesn't recognize expressions or anything tricky.

//#ifdef __GNUC__
		// use the following to catch errors: (only with gcc)
//		virtual const char*	call_method(const char* method_name, const char* method_arg_fmt, ...)
//			__attribute__((format (printf, 3, 4))) = 0;	// "this" is an implied param, so fmt is 3 and ... is 4!
//#else	// not __GNUC__
//		virtual const char*	call_method(const char* method_name, const char* method_arg_fmt, ...) = 0;
//#endif	// not __GNUC__

		virtual void	set_play_state(play_state s) {}
		virtual play_state	get_play_state() const { assert(0); return STOP; }

		// Returns true if labeled frame is found.
		virtual bool	goto_labeled_frame(const tu_string& label) { assert(0); return false; }

		//
		// display-list management.
		//

		virtual execute_tag*	find_previous_replace_or_add_tag(int current_frame, int depth, int id)
		{
			return NULL;
		}

		virtual void	clear_display_objects() { /* assert(0); */ }
		virtual void	remove_display_object(character* ch) { assert(0); }

		// replaces 'this' on md->create_instance()
		virtual void replace_me(character_def*	def) { assert(0); }
		virtual void replace_me(movie_definition*	md) { assert(0); }

		virtual character*	add_display_object( Uint16 character_id, const tu_string& name, const array<swf_event*>& event_handlers, int depth, bool replace_if_depth_is_occupied, const cxform& color_transform, const matrix& mat, float ratio, Uint16 clip_depth, Uint8 blend_mode)
		{
			return NULL;
		}

		virtual void	move_display_object( int depth,	bool use_cxform, const cxform& color_transform, bool use_matrix, const matrix& mat, float ratio, Uint16 clip_depth, Uint8 blend_mode)
		{
		}

		virtual void	replace_display_object( Uint16 character_id, const tu_string& name, int depth, bool use_cxform, const cxform& color_transform, bool use_matrix, const matrix& mat, float ratio, Uint16 clip_depth, Uint8 blend_mode)
		{
		}

		virtual void	replace_display_object( character* ch, const tu_string& name, int depth, bool use_cxform, const cxform& color_transform, bool use_matrix, const matrix& mat, float ratio, Uint16 clip_depth, Uint8 blend_mode)
		{
		}

		virtual void	remove_display_object(int depth, int id)	{}

		virtual void	execute_frame_tags(int frame, bool state_only = false) {}
		virtual void	add_action_buffer(action_buffer* a) { assert(0); }
		virtual void	do_actions(const array<action_buffer*>& action_list) { assert(0); }

		virtual character*	clone_display_object(const tu_string& newname, int depth)
		{
			assert(0);
			return NULL; 
		}

		virtual void	remove_display_object(const tu_string& name) { assert(0); }

		virtual void	stop_drag() { assert(0); }
//		virtual character*	get_root_interface() { return NULL; }
		virtual void	call_frame_actions(const as_value& frame_spec) { assert(0); }

		virtual void	set_background_color(const rgba& color) {}

		//
		// external
		//

		// Set and get userdata, that's useful for the fs_command handler.
		virtual void * get_userdata() { assert(0); return NULL; }
		virtual void set_userdata(void *) { assert(0); }

		// Set an ActionScript variable within this movie.
		// You can use this to set the value of text fields,
		// ordinary variables, or properties of characters
		// within the script.
		//
		// This version accepts UTF-8
		virtual void	set_variable(const char* path_to_var, const char* new_value)
		{
			assert(0);
		}

		// @@ do we want versions that take a number?

		// Get the value of an ActionScript variable.
		//
		// Value is ephemeral & not thread safe!!!  Use it or
		// copy it immediately.
		//
		// Returns UTF-8
		virtual const char*	get_variable(const char* path_to_var) const
		{
			assert(0);
			return "";
		}

		// Accessors for basic display info.

		int	get_id() const { return m_id; }
		character*	get_parent() const { return m_parent.get(); }
		void set_parent(character* parent) { m_parent = parent; }  // for extern movie
		int	get_depth() const { return m_depth; }
		void	set_depth(int d) { m_depth = d; }
		const matrix&	get_matrix() const { return m_matrix; }
		void	set_matrix(const matrix& m)
		{
			m_matrix = m;
		}
		const cxform&	get_cxform() const 
		{ 
			return m_color_transform; 
		}
		void	set_cxform(const cxform& cx)
		{
			m_color_transform = cx;
		}
		void	concatenate_cxform(const cxform& cx) { m_color_transform.concatenate(cx); }
		void	concatenate_matrix(const matrix& m) { m_matrix.concatenate(m); }
		float	get_ratio() const { return m_ratio; }
		void	set_ratio(float f) { m_ratio = f; }
		Uint16	get_clip_depth() const { return m_clip_depth; }
		void	set_clip_depth(Uint16 d) { m_clip_depth = d; }
		Uint8   get_blend_mode() const { return m_blend_mode; }
		void    set_blend_mode(Uint8 d) { m_blend_mode = d; }

		void	set_name(const tu_string& name);
		const tu_string&	get_name() const;

		virtual void get_world_matrix(matrix* m) const;
		virtual void get_world_cxform(cxform* cx) const;

		// Movie interfaces.  By default do nothing.  sprite_instance and some others override these.
		virtual void	display() {}
		virtual float	get_height();
		virtual float	get_width();
		virtual void get_bound(rect* bound);
		virtual character_def* get_character_def() { assert(0); return 0; }

		virtual character*	get_root_movie() const { return m_parent->get_root_movie(); }
		
		// Frame counts in this API are 0-based (unlike ActionScript)
		virtual int	get_current_frame() const { return -1; }
		virtual int	get_target_frame() const { return -1; }		// for goto frame

		virtual int	get_frame_count() const { return -1; }
		virtual int get_loading_frame() const { return -1; }

		virtual bool	has_looped() const { assert(0); return false; }
		virtual void	advance_dlist() {}	// for buttons and sprites
		virtual void	advance_actions() {}	// for buttons and sprites
		virtual void advance_ctor() {}
		virtual bool	goto_frame(int target_frame) { return false; }
		virtual bool	get_accept_anim_moves() const { return true; }

		// Make the movie visible/invisible.  An invisible
		// movie does not advance and does not render.
		virtual void	set_visible(bool visible) { m_visible = visible; }

		// Return visibility status.
		virtual bool	get_visible() const { return m_visible; }

		virtual bool has_keypress_event()
		{
			return false;
		}

		virtual bool can_handle_mouse_event() { return false; }
	
		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual bool	set_member(const tu_string& name, const as_value& val);

		// Movie info
		virtual int	get_movie_version() { return 0; }
		virtual int	get_movie_width() { return 0; }
		virtual int	get_movie_height() { return 0; }

		virtual bool is_visible();
		virtual	bool is_enabled() const { return true; }
		virtual bool is_alive() const
		{
			return m_depth > 0; 
		}

		virtual rgba* get_rgba() { return NULL; }
		virtual void set_rgba(const rgba& color) {}

		virtual void load_bitmaps(array < smart_ptr<bitmap_info> >* bitmap_hash) {};

		// AVM2
		virtual as_function* get_class_constructor(character* ch) { return NULL; }

	};

}

#endif
