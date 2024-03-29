// bakeinflash_impl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

#include "base/tu_file.h"
#include "base/utility.h"
#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_button.h"
#include "bakeinflash/bakeinflash_impl.h"
#include "bakeinflash/bakeinflash_font.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_morph2.h"
#include "bakeinflash/bakeinflash_video_impl.h"
#include "bakeinflash/bakeinflash_render.h"
#include "bakeinflash/bakeinflash_shape.h"
#include "bakeinflash/bakeinflash_stream.h"
#include "bakeinflash/bakeinflash_styles.h"
#include "bakeinflash/bakeinflash_dlist.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_movie_def.h"
#include "bakeinflash/bakeinflash_sprite_def.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_function.h"
#include "bakeinflash/bakeinflash_filters.h"
#include "base/image.h"
#include "base/jpeg.h"
#include "base/zlib_adapter.h"
#include "base/tu_random.h"
#include <string.h>	// for memset
#include <typeinfo>
#include <float.h>


#if TU_CONFIG_LINK_TO_ZLIB
#include <zlib/zlib.h>
#endif // TU_CONFIG_LINK_TO_ZLIB


namespace bakeinflash
{
	bool	s_verbose_action = false;
	bool	s_verbose_parse = false;
	bool	s_verbose_heap = false;

	bool	s_preload_bitmaps = false;
	bool	s_keep_alive_bitmaps = true;
	bool	s_display_invisibles = true;

	// use prebuilt yuv2rgb table
	bool s_yuv2rgb_table = true;

#ifndef NDEBUG
	bool	s_verbose_debug = true;
#else
	bool	s_verbose_debug = false;
#endif

	bool get_verbose_parse()
	{
		return s_verbose_parse;
	}

	bool get_verbose_debug()
	{
		return s_verbose_debug;
	}

	bool get_verbose_action()
	{
		return s_verbose_action;
	}

	void	set_verbose_action(bool verbose)
	// Enable/disable log messages re: actions.
	{
		s_verbose_action = verbose;
	}

	bool	get_yuv2rgb_table()
	{
		return s_yuv2rgb_table;
	}
	void	set_yuv2rgb_table(bool create)
	{
		s_yuv2rgb_table = create;
	}

	void	preload_bitmaps(bool alive)
	{
		s_preload_bitmaps = alive;
	}

	void	keep_alive_bitmaps(bool alive)
	{
		s_keep_alive_bitmaps = alive;
	}

	void	set_display_invisibles(bool show)
	{
		s_display_invisibles = show;
	}

	bool	get_display_invisibles()
	{
		return s_display_invisibles;
	}

	bool is_alive_bitmaps()
	{
		return s_keep_alive_bitmaps;
	}

	void	set_verbose_parse(bool verbose)
	// Enable/disable log messages re: parsing the movie.
	{
		s_verbose_parse = verbose;
	}

	void	set_verbose_heap(bool verbose)
	// Enable/disable log messages re: parsing the movie.
	{
		s_verbose_heap = verbose;
	}

	bool get_verbose_heap()
	{
		return s_verbose_heap;
	}

	//
	// some utility stuff
	//

	void	execute_actions(as_environment* env, const array<action_buffer*>& action_list)
	// Execute the actions in the action list, in the given
	// environment.
	{
		for (int i = 0; i < action_list.size(); i++)
		{
			action_list[i]->execute(env);
		}
	}

	character*	character_def::create_character_instance(character* parent, int id)
	// Default.  Make a generic_character.
	{
		character * ch = new generic_character(this, parent, id);
		// instanciate_registered_class(ch);	//TODO: test it
		return ch;
	}

	void character_def::set_registered_class_constructor(const as_value& value)
	{
		m_registered_class_constructor = value.to_function();
	}

	void character_def::instanciate_registered_class (character* ch)
	{
		assert(ch);

		// AVM2, try SymbolClass tag
		if (m_registered_class_constructor == NULL)
		{
			m_registered_class_constructor = ch->get_class_constructor(ch);
		}

		// set component args
		// movieclip can be a component without component class
		ch->on_event(event_id::CONSTRUCT);

		if (m_registered_class_constructor != NULL)
		{
			// any function must have a prototype
			// action script class definition is a function and its
			// prototype contains set of function/methods of that class
			// any object must can have a proto
			// while creating new object as instance of class
			// all functions from prototype are copied to proto of object
			// so, proto of object contains all functions from all prototypes of
			// all super classes

			// we do not create prototype for func, it is exists already
			// But we must create proto for object and call its constructor
			ch->m_this_ptr = ch;
			as_object* proto = ch->create_proto(m_registered_class_constructor.get());
			UNUSED(proto);

			// exec constructor

			//as_environment env;
			//call_method(m_registered_class_constructor.get(), &env, ch, 0, 0);

			// use caller environment
			as_environment* env = ch->get_environment();
			assert(env);
			call_method(m_registered_class_constructor.get(), env, ch, 0, 0);
		}
	}


	//
	// character
	//

	void	ensure_loaders_registered()
	{
		static bool	s_registered = false;

		if (s_registered == false)
		{
			// Register the standard loaders.
			s_registered = true;
			register_tag_loader(0, end_loader);
			register_tag_loader(2, define_shape_loader);
			register_tag_loader(4, place_object_2_loader);
			register_tag_loader(5, remove_object_2_loader);
			register_tag_loader(6, define_bits_jpeg_loader);
			register_tag_loader(7, button_character_loader);
			register_tag_loader(8, jpeg_tables_loader);
			register_tag_loader(9, set_background_color_loader);
			register_tag_loader(10, define_font_loader);
			register_tag_loader(11, define_text_loader);
			register_tag_loader(12, do_action_loader);
			register_tag_loader(13, define_font_info_loader);
			register_tag_loader(14, define_sound_loader);
			register_tag_loader(15, start_sound_loader);
			register_tag_loader(17, button_sound_loader);
			register_tag_loader(18, sound_stream_head_loader);
			register_tag_loader(19, sound_stream_block_loader);
			register_tag_loader(20, define_bits_lossless_2_loader);
			register_tag_loader(21, define_bits_jpeg2_loader);
			register_tag_loader(22, define_shape_loader);
			register_tag_loader(24, null_loader);	// "protect" tag; we're not an authoring tool so we don't care.
			register_tag_loader(26, place_object_2_loader);
			register_tag_loader(28, remove_object_2_loader);
			register_tag_loader(32, define_shape_loader);
			register_tag_loader(33, define_text_loader);
			register_tag_loader(37, define_edit_text_loader);
			register_tag_loader(34, button_character_loader);
			register_tag_loader(35, define_bits_jpeg3_loader);
			register_tag_loader(36, define_bits_lossless_2_loader);
			register_tag_loader(39, sprite_loader);
			register_tag_loader(43, frame_label_loader);
			register_tag_loader(45, sound_stream_head_loader);
			register_tag_loader(46, define_shape_morph_loader);
			register_tag_loader(48, define_font_loader);
			register_tag_loader(56, export_loader);
			register_tag_loader(57, import_loader);
			register_tag_loader(58, define_enable_debugger_loader);
			register_tag_loader(59, do_init_action_loader);	  
			register_tag_loader(60, define_video_loader);
			register_tag_loader(61, video_loader);
			register_tag_loader(62, define_font_info_loader);

			register_tag_loader(63, define_font_info_loader);
			register_tag_loader(64, define_enable_debugger_loader);
			register_tag_loader(66, define_tabindex_loader);
			register_tag_loader(69, define_file_attribute_loader);	// Flash 8
			register_tag_loader(70, place_object_2_loader);
			register_tag_loader(73, define_font_alignzones);	// DefineFontAlignZones - Flash 8
			register_tag_loader(74, define_csm_textsetting_loader); // CSMTextSetting - Flash 8
			register_tag_loader(75, define_font_loader);	// DefineFont3 - Flash 8
			register_tag_loader(77, define_metadata_loader);	// MetaData - Flash 8
			register_tag_loader(83, define_shape_loader);		// DefineShape4 - Flash 8
			register_tag_loader(88, define_font_name);		// DefineFontName - Flash 9

			register_tag_loader(76, symbol_class_loader);	// SymbolClass - Flash 9
			register_tag_loader(82, define_abc_loader);	// doABC - Flash 9
			register_tag_loader(84, define_shape_morph_loader); //DefineMorphShape2 - Flash 9
			register_tag_loader(86, define_scene_loader);		// DefineSceneAndFrameLabelData - Flash 9

		}
	}

	// FIXME
	/*
	void	get_movie_info(
		const char* filename,
		int* version,
		int* width,
		int* height,
		float* frames_per_second,
		int* frame_count,
		int* tag_count
		)
		// Attempt to read the header of the given .swf movie file.
		// Put extracted info in the given vars.
		// Sets *version to 0 if info can't be extracted.
	{
		if (s_opener_function == NULL)
		{
			myprintf("error: get_movie_info(): no file opener function registered\n");
			if (version) *version = 0;
			return;
		}

		tu_file*	in = s_opener_function(filename);
		if (in == NULL || in->get_error() != TU_FILE_NO_ERROR)
		{
			myprintf("error: get_movie_info(): can't open '%s'\n", filename);
			if (version) *version = 0;
			delete in;
			return;
		}

		Uint32	file_start_pos = in->get_position();
		Uint32	header = in->read_le32();
		Uint32	file_length = in->read_le32();
		Uint32	file_end_pos = file_start_pos + file_length;

		int	local_version = (header >> 24) & 255;
		if ((header & 0x0FFFFFF) != 0x00535746
			&& (header & 0x0FFFFFF) != 0x00535743)
		{
			// ERROR
			myprintf("error: get_movie_info(): file '%s' does not start with a SWF header!\n", filename);
			if (version) *version = 0;
			delete in;
			return;
		}
		bool	compressed = (header & 255) == 'C';

		tu_file*	original_in = NULL;
		if (compressed)
		{
#if TU_CONFIG_LINK_TO_ZLIB == 0
			myprintf("get_movie_info(): can't read zipped SWF data; TU_CONFIG_LINK_TO_ZLIB is 0!\n");
			return;
#endif
			original_in = in;

			// Uncompress the input as we read it.
			in = zlib_adapter::make_inflater(original_in);

			// Subtract the size of the 8-byte header, since
			// it's not included in the compressed
			// stream length.
			file_end_pos = file_length - 8;
		}

		stream	str(in);

		rect	frame_size;
		frame_size.read(&str);

		float	local_frame_rate = str.read_u16() / 256.0f;
		int	local_frame_count = str.read_u16();

		if (version) *version = local_version;
		if (width) *width = int(frame_size.width() / 20.0f + 0.5f);
		if (height) *height = int(frame_size.height() / 20.0f + 0.5f);
		if (frames_per_second) *frames_per_second = local_frame_rate;
		if (frame_count) *frame_count = local_frame_count;

		if (tag_count)
		{
			// Count tags.
			int local_tag_count = 0;
			while ((Uint32) str.get_position() < file_end_pos)
			{
				int tag_type = str.open_tag();
				UNUSED(tag_type);
				str.close_tag();
				local_tag_count++;
			}
			*tag_count = local_tag_count;
		}

		delete in;
		delete original_in;
	}
*/
	static bool	s_no_recurse_while_loading = false;	// @@ TODO get rid of this; make it the normal mode.

	//
	// global bakeinflash management
	//
	void clears_tag_loaders();
	void clear_shared_libs();

	void	precompute_cached_data(movie_definition* movie_def)
	// Fill in cached data in movie_def.
	// @@@@ NEEDS TESTING -- MIGHT BE BROKEN!!!
	{
		assert(movie_def != NULL);

		// Temporarily install null render and sound handlers,
		// so we don't get output during preprocessing.
		//
		// Use automatic struct var to make sure we restore
		// when exiting the function.
		struct save_stuff
		{
			render_handler*	m_original_rh;
			sound_handler*	m_original_sh;

			save_stuff()
			{
				// Save.
				m_original_rh = get_render_handler();
				m_original_sh = get_sound_handler();
				set_render_handler(NULL);
				set_sound_handler(NULL);
			}

			~save_stuff()
			{
				// Restore.
				set_render_handler(m_original_rh);
				set_sound_handler(m_original_sh);
			}
		} save_stuff_instance;

		// Need an instance.
		smart_ptr<bakeinflash::root>	m = movie_def->create_instance();
		if (m == NULL)
		{
			myprintf("error: precompute_cached_data can't create instance of movie\n");
			return;
		}

		// Run through the movie's frames.
		//
		// @@ there might be cleaner ways to do this; e.g. do
		// execute_frame_tags(i, true) on movie and all child
		// sprites.
		int	kick_count = 0;
		for (;;)
		{
			// @@ do we also have to run through all sprite frames
			// as well?
			//
			// @@ also, ActionScript can rescale things
			// dynamically -- we can't really do much about that I
			// guess?
			//
			// @@ Maybe we should allow the user to specify some
			// safety margin on scaled shapes.

			int	last_frame = m->get_current_frame();
			m->advance(0.010f);
			m->display();

			if (m->get_current_frame() == movie_def->get_frame_count() - 1)
			{
				// Done.
				break;
			}

			if (m->get_play_state() == bakeinflash::character::STOP)
			{
				// Kick the movie.
				//myprintf("kicking movie, kick ct = %d\n", kick_count);
				m->goto_frame(last_frame + 1);
				m->set_play_state(bakeinflash::character::PLAY);
				kick_count++;

				if (kick_count > 10)
				{
					//myprintf("movie is stalled; giving up on playing it through.\n");
					break;
				}
			}
			else if (m->get_current_frame() < last_frame)
			{
				// Hm, apparently we looped back.  Skip ahead...
				myprintf("loop back; jumping to frame %d\n", last_frame);
				m->goto_frame(last_frame + 1);
			}
			else
			{
				kick_count = 0;
			}
		}
	}


	//
	// Some tag implementations
	//


	void	null_loader(stream* in, int tag_type, movie_definition_sub* m)
		// Silently ignore the contents of this tag.
	{
	}

	void	frame_label_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Label the current frame of m with the name from the stream.
	{
		tu_string n;
		in->read_string(&n);
		m->add_frame_name(n.c_str());
	}

	struct set_background_color : public execute_tag
	{
		rgba	m_color;

		void	execute(character* m)
		{
			m->set_background_color(m_color);
		}

		void	execute_state(character* m)
		{
			execute(m);
		}

		void	read(stream* in)
		{
			m_color.read_rgb(in);

			IF_VERBOSE_PARSE(myprintf("  set_background_color: (%d %d %d)\n",
				m_color.m_r, m_color.m_g, m_color.m_b));
		}
	};


	void	set_background_color_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 9);
		assert(m);

		set_background_color*	t = new set_background_color;
		t->read(in);

		m->add_execute_tag(t);
	}

	void	jpeg_tables_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load JPEG compression tables that can be used to load
	// images further along in the stream.
	{
		assert(tag_type == 8);

    int curr_pos = in->get_position();
    int end_pos = in->get_tag_end_position();
    int header_size = end_pos - curr_pos;
    assert(header_size >= 0);

#if TU_CONFIG_LINK_TO_JPEGLIB

		jpeg::input*	j_in = NULL;
		if (header_size > 0)
		{
			j_in = jpeg::input::create_swf_jpeg2_header_only(in->get_underlying_stream());
		}
		m->set_jpeg_loader(j_in);

#endif // TU_CONFIG_LINK_TO_JPEGLIB
	}


	void	define_bits_jpeg_loader(stream* in, int tag_type, movie_definition_sub* m)
	// A JPEG image without included tables; those should be in an
	// existing jpeg::input object stored in the movie.
	{
		assert(tag_type == 6);

		Uint16	character_id = in->read_u16();

		bitmap_character_def*	ch = NULL;
		if (m->get_jpeg_loader())
		{
			// hack, for Flash < 9
			// decompress bitmap here..during parsing
			ch = new bitmap_character_def(m, NULL, 0, tag_type, m->get_jpeg_loader());
		}
		else
		{
			int tag_length = in->get_tag_end_position() - in->get_position();
			Uint8* tag = (Uint8*) malloc(tag_length);
			in->get_underlying_stream()->read_bytes(tag, tag_length);
			ch = new bitmap_character_def(m, tag, tag_length, tag_type, m->get_jpeg_loader());
		}

		// add image to movie, under character id.
		m->add_bitmap_character(character_id, ch);
	}

	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 21);

		Uint16	character_id = in->read_u16();

		int tag_length = in->get_tag_end_position() - in->get_position();
		Uint8* tag = (Uint8*) malloc(tag_length);
		in->get_underlying_stream()->read_bytes(tag, tag_length);
		bitmap_character_def*	ch = new bitmap_character_def(m, tag, tag_length, tag_type);

		// add image to movie, under character id.
		m->add_bitmap_character(character_id, ch);
	}


#if TU_CONFIG_LINK_TO_ZLIB
	unsigned long	unzip(Uint8* buffer, int buffer_size, const Uint8* zimage, int zlen)
	{
		assert(zlen > 0);
		unsigned long real_buffer_size = buffer_size;
		int err = uncompress(buffer, &real_buffer_size, zimage, zlen);
		if (err != Z_OK)
		{
			myprintf("uncompress error: %d\n", err);
			return -1;
		}
		return real_buffer_size;
	}
#endif

#if 0	//TU_CONFIG_LINK_TO_ZLIB
	void	inflate_wrapper(tu_file* in, void* buffer, int buffer_bytes)
		// Wrapper function -- uses Zlib to uncompress in_bytes worth
		// of data from the input file into buffer_bytes worth of data
		// into *buffer.
	{
		assert(in);
		assert(buffer);
		assert(buffer_bytes > 0);

		int err;
		z_stream d_stream; /* decompression stream */

		d_stream.zalloc = (alloc_func)0;
		d_stream.zfree = (free_func)0;
		d_stream.opaque = (voidpf)0;

		d_stream.next_in  = 0;
		d_stream.avail_in = 0;

		d_stream.next_out = (Byte*) buffer;
		d_stream.avail_out = (uInt) buffer_bytes;

		err = inflateInit(&d_stream);
		if (err != Z_OK) {
			myprintf("error: inflate_wrapper() inflateInit() returned %d\n", err);
			return;
		}

		Uint8	buf[1];

		for (;;) {
			// Fill a one-byte (!) buffer.
			buf[0] = in->read_byte();
			d_stream.next_in = &buf[0];
			d_stream.avail_in = 1;

			err = inflate(&d_stream, Z_SYNC_FLUSH);
			if (err == Z_STREAM_END) break;
			if (err != Z_OK)
			{
				myprintf("error: inflate_wrapper() inflate() returned %d\n", err);
			}
		}

		err = inflateEnd(&d_stream);
		if (err != Z_OK)
		{
			myprintf("error: inflate_wrapper() inflateEnd() return %d\n", err);
		}
	}
#endif // TU_CONFIG_LINK_TO_ZLIB


	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie_definition_sub* m)
		// loads a define_bits_jpeg3 tag. This is a jpeg file with an alpha
		// channel using zlib compression.
	{
		assert(tag_type == 35);

		Uint16	character_id = in->read_u16();

		int tag_length = in->get_tag_end_position() - in->get_position();
		Uint8* tag = (Uint8*) malloc(tag_length);
		in->get_underlying_stream()->read_bytes(tag, tag_length);
		bitmap_character_def*	ch = new bitmap_character_def(m, tag, tag_length, tag_type);

		// add image to movie, under character id.
		m->add_bitmap_character(character_id, ch);
	}


	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 20 || tag_type == 36);

		Uint16	character_id = in->read_u16();

		int tag_length = in->get_tag_end_position() - in->get_position();
		Uint8* tag = (Uint8*) malloc(tag_length);
		in->get_underlying_stream()->read_bytes(tag, tag_length);
		bitmap_character_def*	ch = new bitmap_character_def(m, tag, tag_length, tag_type);

		// add image to movie, under character id.
		m->add_bitmap_character(character_id, ch);

	}

	void	define_abc_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 82);

		// if flags == 1 then parse only
		Uint32 flags = in->read_u32();
		UNUSED(flags);

		tu_string name;
		in->read_string(&name);

		IF_VERBOSE_PARSE(myprintf("\n doABC tag loader, abc_name = '%s'\n", name.c_str()));

		abc_def* abc = new abc_def();
		abc->read(in, m);
		m->add_abc(name, abc);
	}

	void	define_font_name(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 88);
		IF_VERBOSE_PARSE(myprintf("\n  define_font_name\n"));

		int font_id = in->read_u16();

		tu_string font_name;
		in->read_string(&font_name);

		tu_string font_copyright;	// unused
		in->read_string(&font_copyright);

		font*	f = m->get_font(font_id);
		if (f)
		{
			f->set_name(font_name);

			// font name contains Bold/Italic keyword
			// may be not correct !
			f->set_bold(false);
			f->set_italic(false);
		}

	}

	void	define_shape_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 2 || tag_type == 22 || tag_type == 32 || tag_type == 83);

		Uint16	character_id = in->read_u16();
		IF_VERBOSE_PARSE(myprintf("  shape_loader: id = %d\n", character_id));

		shape_character_def*	ch = new shape_character_def();
		ch->read(in, tag_type, true, m);

		IF_VERBOSE_PARSE(myprintf("  bound rect:"); ch->get_bound_local().print());

		m->add_character(character_id, ch);
	}

	void define_shape_morph_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 46 || tag_type == 84);
		Uint16 character_id = in->read_u16();
		IF_VERBOSE_PARSE(myprintf("  shape_morph_loader: id = %d\n", character_id));
		morph2_character_def* morph = new morph2_character_def();
		morph->read(in, tag_type, true, m);
		m->add_character(character_id, morph);
	}

	//
	// font loaders
	//


	void	define_font_loader(stream* in, int tag_type, movie_definition_sub* m)
		// Load a DefineFont or DefineFont2 tag.
	{
		assert(tag_type == 10 || tag_type == 48 || tag_type == 75);

		Uint16	font_id = in->read_u16();

		font*	f = new font();
		f->read(in, tag_type, m);

		m->add_font(font_id, f);

		// Automatically keeping fonts in fontlib is
		// problematic.	 The app should be responsible for
		// optionally adding fonts to fontlib.
		// //fontlib::add_font(f);
	}


	void	define_font_info_loader(stream* in, int tag_type, movie_definition_sub* m)
		// Load a DefineFontInfo tag.  This adds information to an
		// existing font.
	{
		assert(tag_type == 13 || tag_type == 62);

		Uint16	font_id = in->read_u16();

		font*	f = m->get_font(font_id);
		if (f)
		{
			f->read_font_info(in, tag_type);
		}
		else
		{
			myprintf("define_font_info_loader: can't find font w/ id %d\n", font_id);
		}
	}

	void	define_enable_debugger_loader(stream* in, int tag_type, movie_definition_sub* m)
	// this tag defines characteristics of the SWF file (Flash 8)
	{
		assert(tag_type == 58 || tag_type == 64);

		if (tag_type == 64)	// define_enable_debugger_loader
		{
			in->read_u16();	// reserved
		}

		// now attr is't used
		tu_string md5_password;
		in->read_string(&md5_password);
	}

	void	define_tabindex_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 66);

		// for now this tag is not implemented
		Uint16 depth = in->read_u16();
		Uint16 tabindex = in->read_u16();
		UNUSED(depth);
		UNUSED(tabindex);
	}

	void	define_file_attribute_loader(stream* in, int tag_type, movie_definition_sub* m)
	// this tag defines characteristics of the SWF file (Flash 8)
	{
		assert(tag_type == 69);

		Uint8 attr = in->read_u8();
		in->read_uint(24);	// reserved

		m->m_has_metadata =  attr & 0x10 ? true : false;
		m->m_is_as3 =  attr & 0x08 ? true : false;
		m->m_use_network =  attr & 0x010 ? true : false;
	}

	void	define_font_alignzones(stream* in, int tag_type, movie_definition_sub* m)
	// this tag defines characteristics of the SWF file (Flash 8)
	{
		assert(tag_type == 73);

		Uint16	font_id = in->read_u16();
		font*	f = m->get_font(font_id);
		if (f)
		{
			f->read_font_alignzones(in, tag_type);
		}
		else
		{
			myprintf("define_font_alignzones: can't find font w/ id %d\n", font_id);
		}
	}

	void	define_csm_textsetting_loader(stream* in, int tag_type, movie_definition_sub* m)
	// this tag defines quality & options of text filed
	{
		assert(tag_type == 74);

		Uint16	text_id = in->read_u16();
		character_def*	ch = m->get_character_def(text_id);
		if (ch)
		{
			ch->csm_textsetting(in, tag_type);
		}
		else
		{
			myprintf("define_font_alignzones: can't find font w/ id %d\n", text_id);
		}
	}

	//
	// swf_event
	//
	// For embedding event handlers in place_object_2

	//
	// place_object_2
	//

	struct place_object_2 : public execute_tag
	{
		int	m_tag_type;
		tu_string	m_character_name;
		float	m_ratio;
		cxform	m_color_transform;
		matrix	m_matrix;
		bool	m_has_matrix;
		bool	m_has_cxform;
		int 	m_depth;
		Uint16	m_character_id;
		Uint16	m_clip_depth;
		Uint8   m_blend_mode;
		enum place_type {
			PLACE,
			MOVE,
			REPLACE,
		} m_place_type;
		array<swf_event*>	m_event_handlers;

		place_object_2() :
			m_tag_type(0), m_ratio(0), m_has_matrix(false), m_has_cxform(false), m_depth(0),
			m_character_id(0), m_clip_depth(0), m_blend_mode(0), m_place_type(PLACE)
		{
		}

		virtual ~place_object_2()
		{
			for (int i = 0, n = m_event_handlers.size(); i < n; i++)
			{
				delete m_event_handlers[i];
			}
			m_event_handlers.resize(0);
		}

		void	read(stream* in, int tag_type, int movie_version)
		{
			assert(tag_type == 4 || tag_type == 26 || tag_type == 70);

			m_tag_type = tag_type;

			if (tag_type == 4)
			{
				// Original place_object tag; very simple.
				m_character_id = in->read_u16();
				m_depth = in->read_u16();
				m_matrix.read(in);

				IF_VERBOSE_PARSE(
					myprintf("  char_id = %d\n"
					"  depth = %d\n"
					"  mat = \n",
					m_character_id,
					m_depth);
				m_matrix.print());

				if (in->get_position() < in->get_tag_end_position())
				{
					m_color_transform.read_rgb(in);
					IF_VERBOSE_PARSE(myprintf("  cxform:\n"); m_color_transform.print());
				}
			}
			else
			if (tag_type == 26 || tag_type == 70)
			{
				in->align();

				bool	has_actions = in->read_uint(1) ? true : false;
				bool	has_clip_bracket = in->read_uint(1) ? true : false;
				bool	has_name = in->read_uint(1) ? true : false;
				bool	has_ratio = in->read_uint(1) ? true : false;
				bool	has_cxform = in->read_uint(1) ? true : false;
				bool	has_matrix = in->read_uint(1) ? true : false;
				bool	has_char = in->read_uint(1) ? true : false;
				bool	flag_move = in->read_uint(1) ? true : false;

				bool	has_image = false;
				bool	has_class_name = false;
				bool	has_cache_asbitmap = false;
				bool	has_blend_mode = false;
				bool	has_filter_list = false;

				if (tag_type == 70)
				{
					in->read_uint(3); // unused

					has_image = in->read_uint(1) ? true : false;
					has_class_name = in->read_uint(1) ? true : false;
					has_cache_asbitmap = in->read_uint(1) ? true : false;
					has_blend_mode = in->read_uint(1) ? true : false;
					has_filter_list = in->read_uint(1) ? true : false;
				}

				m_depth = in->read_u16();
				IF_VERBOSE_PARSE(myprintf("  depth = %d\n", m_depth));

				if (has_char)
				{
					m_character_id = in->read_u16();
					IF_VERBOSE_PARSE(myprintf("  char id = %d\n", m_character_id));
				}

				if (has_matrix)
				{
					m_has_matrix = true;
					m_matrix.read(in);
					IF_VERBOSE_PARSE(myprintf("  mat:\n"); m_matrix.print());
				}
				if (has_cxform)
				{
					m_has_cxform = true;
					m_color_transform.read_rgba(in);
					IF_VERBOSE_PARSE(myprintf("  cxform:\n"); m_color_transform.print());
				}

				if (has_ratio)
				{
					m_ratio = (float)in->read_u16() / (float)65535;
					IF_VERBOSE_PARSE(myprintf("  ratio: %f\n", m_ratio));
				}

				if (has_name)
				{
					in->read_string(&m_character_name);
					IF_VERBOSE_PARSE(myprintf("  name = %s\n", m_character_name.c_str()));
				}
				if (has_clip_bracket)
				{
					m_clip_depth = in->read_u16(); 
					IF_VERBOSE_PARSE(myprintf("  clip_depth = %d\n", m_clip_depth));
				}

				if (has_filter_list)
				{
					read_filter_list(in);
				}

				if (has_blend_mode)
				{
					// TODO, implement blend_mode
					m_blend_mode = in->read_u8();
				}

				if (has_actions)
				{
					Uint16	reserved = in->read_u16();
					UNUSED(reserved);

					// The logical 'or' of all the following handlers.
					// I don't think we care about this...
					Uint32	all_flags = 0;
					if (movie_version >= 6)
					{
						all_flags = in->read_u32();
					}
					else
					{
						all_flags = in->read_u16();
					}
					UNUSED(all_flags);

					IF_VERBOSE_PARSE(myprintf("  actions: flags = 0x%X\n", all_flags));

					// Read swf_events.
					for (;;)
					{
						// Read event.
						in->align();

						Uint32 flags = (movie_version >= 6) ? in->read_u32() : in->read_u16();
						if (flags == 0)
						{
							// Done with events.
							break;
						}

						Uint32 event_length = in->read_u32();
						Uint8 ch = key::INVALID;

						if (flags & (1 << 17))	// has keypress event
						{
							ch = in->read_u8();
							event_length--;
						}

						// Read the actions for event(s)
						action_buffer action;
						action.read(in);

						if (action.get_length() != static_cast<int>(event_length))
						{
							myprintf("swf_event::read(), event_length = %d, but read %d\n",
								event_length, action.get_length());
							break;
						}

						// 13 bits reserved, 19 bits used
						static const event_id s_code_bits[19] =
						{
							event_id::LOAD,
							event_id::ENTER_FRAME,
							event_id::UNLOAD,
							event_id::MOUSE_MOVE,
							event_id::MOUSE_DOWN,
							event_id::MOUSE_UP,
							event_id::KEY_DOWN,
							event_id::KEY_UP,
							event_id::DATA,
							event_id::INITIALIZE,
							event_id::PRESS,
							event_id::RELEASE,
							event_id::RELEASE_OUTSIDE,
							event_id::ROLL_OVER,
							event_id::ROLL_OUT,
							event_id::DRAG_OVER,
							event_id::DRAG_OUT,
							event_id(event_id::KEY_PRESS, key::CONTROL),
							event_id::CONSTRUCT
						};

						// Let's see if the event flag we received is for an event that we know of
						if (1 << (TU_ARRAYSIZE(s_code_bits)) < flags)
						{
							myprintf("swf_event::read() -- unknown event type received, flags = 0x%x\n", flags);
						}

						for (int i = 0, mask = 1; i < int(sizeof(s_code_bits)/sizeof(s_code_bits[0])); i++, mask <<= 1)
						{
							if (flags & mask)
							{
								swf_event* ev = new swf_event;
								ev->m_event = s_code_bits[i];

								if (i == 17)	// has keypress event ?
								{
									ev->m_event.m_key_code = ch;
								}

								ev->m_method = action;
								m_event_handlers.push_back(ev);
							}
						}
					}
				}

				if (has_char == true && flag_move == true)
				{
					// Remove whatever's at m_depth, and put m_character there.
					m_place_type = REPLACE;
				}
				else if (has_char == false && flag_move == true)
				{
					// Moves the object at m_depth to the new location.
					m_place_type = MOVE;
				}
				else if (has_char == true && flag_move == false)
				{
					// Put m_character at m_depth.
					m_place_type = PLACE;
				}

				//myprintf("place object at depth %i\n", m_depth);
			}
		}


		void	execute(character* m)
			// Place/move/whatever our object in the given movie.
		{
			switch (m_place_type)
			{
				default:
					break;

				case PLACE:
					if ((m_blend_mode == 0) && (m->m_blend_mode != 0))
					{
						m->add_display_object( m_character_id, m_character_name, m_event_handlers, m_depth, m_tag_type != 4, m_color_transform, m_matrix, m_ratio, m_clip_depth, m->m_blend_mode);
					}
					else
					{
						m->add_display_object( m_character_id, m_character_name, m_event_handlers, m_depth, m_tag_type != 4, m_color_transform, m_matrix, m_ratio, m_clip_depth, m_blend_mode);
					}
					break;

				case MOVE:
					if ((m_blend_mode == 0) && (m->m_blend_mode != 0))
					{
						m->move_display_object( m_depth, m_has_cxform, m_color_transform, m_has_matrix, m_matrix, m_ratio, m_clip_depth, m->m_blend_mode);
					}
					else
					{
						m->move_display_object( m_depth, m_has_cxform, m_color_transform, m_has_matrix, m_matrix, m_ratio, m_clip_depth, m_blend_mode);
					}
					break;

				case REPLACE:
					m->replace_display_object( m_character_id, m_character_name, m_depth, m_has_cxform, m_color_transform, m_has_matrix, m_matrix, m_ratio, m_clip_depth, m_blend_mode);
					break;
				}
		}

		void	execute_state(character* m)
		{
			execute(m);
		}

		void	execute_state_reverse(character* m, int frame)
		{
			switch (m_place_type)
			{
				default:
					break;

				case PLACE:
					// reverse of add is remove
					// NOTE:  this line used to read m->remove_display_object(m_depth, m_tag_type == 4 ? m_character : -1);
					// we were having problems with in certain cases remove_display_object removing the incorrect object,
					// the line was returning -1 frequently and inside remove_display_object it would pick an incorrect object
					// at the same depth
					m->remove_display_object(m_depth, m_character_id);
					break;

				case MOVE:
					// reverse of move is move
					m->move_display_object( m_depth, m_has_cxform, m_color_transform, m_has_matrix, m_matrix, m_ratio, m_clip_depth, m->m_blend_mode);
					break;

				case REPLACE:
					{
						// reverse of replace is to re-add the previous object.
						execute_tag*	last_add = m->find_previous_replace_or_add_tag(frame, m_depth, -1);
						if (last_add)
						{
							last_add->execute_state(m);
						}
						else
						{
							myprintf("reverse REPLACE can't find previous replace or add tag(%d, %d)\n", frame, m_depth);
						}
						break;
					}
			}
		}

		virtual uint32	get_depth_id_of_replace_or_add_tag() const
			// "depth_id" is the 16-bit depth & id packed into one 32-bit int.
		{
			if (m_place_type == PLACE || m_place_type == REPLACE)
			{
				int	id = -1;
				if (m_tag_type == 4)
				{
					// Old-style PlaceObject; the corresponding Remove
					// is specific to the character_id.
					id = m_character_id;
				}
				return ((m_depth & 0x0FFFF) << 16) | (id & 0x0FFFF);
			}
			else
			{
				return (uint32) -1;
			}
		}
	};



	void	place_object_2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 4 || tag_type == 26 || tag_type == 70);

		IF_VERBOSE_PARSE(myprintf("  place_object_2\n"));

		place_object_2*	ch = new place_object_2;
		ch->read(in, tag_type, m->get_version());

		m->add_execute_tag(ch);
	}

	// Create a (mutable) instance of our definition.  The
	// instance is created to live (temporarily) on some level on
	// the parent movie's display list.
	character*	sprite_definition::create_character_instance(character* parent, int id)
	{
		sprite_instance*	si = new sprite_instance(this, get_root(), parent, id);
		return si;
	}

	root*	movie_def_impl::create_instance()
	// Create a playable movie instance from a def.
	{
		root*	root = create_root();
		
		// create dlist
		root->get_root_movie()->execute_frame_tags(0);		

		return root;
	}

	root*	movie_def_impl::create_root()
	{
		root*	m = new root(this);
		assert(m);

		sprite_instance*	root_movie = new sprite_instance(this, m, NULL, -1);
		assert(root_movie);

		// By default _root has no name
		//		root_movie->set_name("_root");
		root_movie->set_member("$version", get_bakeinflash_version());	// Flash 8
		m->set_root_movie(root_movie);
		return m;
	}


	// Create and initialize a sprite, and add it to the movie.
	void	sprite_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 39);

		int	character_id = in->read_u16();

		IF_VERBOSE_PARSE(myprintf("  sprite\n  char id = %d\n", character_id));

		sprite_definition*	ch = new sprite_definition(m);	// @@ combine sprite_definition with movie_def_impl
		ch->read(in);

		m->add_character(character_id, ch);
	}

	//
	// end_tag
	//

	// end_tag doesn't actually need to exist.

	void	end_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 0);
		assert(in->get_position() == in->get_tag_end_position());
	}


	//
	// remove_object_2
	//


	struct remove_object_2 : public execute_tag
	{
		int	m_depth, m_id;

		remove_object_2() : m_depth(-1), m_id(-1) {}

		void read(stream* in, int tag_type)
		{
			assert(tag_type == 5 || tag_type == 28);

			if (tag_type == 5)
			{
				// Older SWF's allow multiple objects at the same depth;
				// this m_id disambiguates.  Later SWF's just use one
				// object per depth.
				m_id = in->read_u16();
			}
			m_depth = in->read_u16();
		}

		virtual void execute(character* m)
		{
			m->remove_display_object(m_depth, m_id);
		}

		virtual void execute_state(character* m)
		{
			execute(m);
		}

		virtual void execute_state_reverse(character* m, int frame)
		{
			// reverse of remove is to re-add the previous object.
			execute_tag* last_add = m->find_previous_replace_or_add_tag(frame, m_depth, m_id);
			if (last_add)
			{
				last_add->execute_state(m);
			}
			else
			{
				myprintf("reverse REMOVE can't find previous replace or add tag(%d, %d)\n", frame, m_depth);
			}
		}

		virtual bool is_remove_tag() const { return true; }
	};


	void	remove_object_2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 5 || tag_type == 28);

		remove_object_2*	t = new remove_object_2;
		t->read(in, tag_type);

		IF_VERBOSE_PARSE(myprintf("  remove_object_2(%d)\n", t->m_depth));

		m->add_execute_tag(t);
	}


	void	button_sound_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 17);

		int	button_character_id = in->read_u16();
		button_character_definition* ch = (button_character_definition*) m->get_character_def(button_character_id);
		assert(ch != NULL);

		ch->read(in, tag_type, m);
	}


	void	button_character_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 7 || tag_type == 34);

		int	character_id = in->read_u16();

		IF_VERBOSE_PARSE(myprintf("  button character loader: char_id = %d\n", character_id));

		button_character_definition*	ch = new button_character_definition();
		ch->read(in, tag_type, m);

		m->add_character(character_id, ch);
	}


	//
	// export
	//


	void	export_loader(stream* in, int tag_type, movie_definition_sub* m) // Load an export tag (for exposing internal resources of m)
	{
		assert(tag_type == 56);

		int	count = in->read_u16();

		IF_VERBOSE_PARSE(myprintf("  export: count = %d\n", count));

		// Read the exports.
		for (int i = 0; i < count; i++)
		{
			Uint16	id = in->read_u16();
			tu_string resource_name;
			in->read_string(&resource_name);
			IF_VERBOSE_PARSE(myprintf("  export: id = %d, name = %s\n", id, resource_name.c_str()));

			if (font* f = m->get_font(id))
			{
				// Expose this font for export.
				m->export_resource(resource_name, f);
			}
			else if (character_def* ch = m->get_character_def(id))
			{
				// Expose this movie/button/whatever for export.
				m->export_resource(resource_name, ch);
			}
			else if (sound_sample* ch = m->get_sound_sample(id))
			{
				m->export_resource(resource_name, ch);
			}
			else
			{
				myprintf("export error: don't know how to export resource '%s'\n", resource_name.c_str());
			}
		}
	}


	//
	// import
	//


	void	import_loader(stream* in, int tag_type, movie_definition_sub* m) // Load an import tag (for pulling in external resources)
	{
		assert(tag_type == 57);

		tu_string source_url;
		in->read_string(&source_url);
		int	count = in->read_u16();

		IF_VERBOSE_PARSE(myprintf("  import: source_url = %s, count = %d\n", source_url.c_str(), count));

		// Try to load the source movie into the movie library.
		movie_definition*	source_movie = NULL;

		if (s_no_recurse_while_loading == false)
		{
			source_movie = create_movie(source_url.c_str());
			if (source_movie == NULL)
			{
				// If workdir is set, try again with
				// the path relative to workdir.
				tu_string relative_url = get_workdir();
				if (relative_url.size())
				{
					relative_url += source_url;
					source_movie = create_movie(relative_url);
				}

				if (source_movie == NULL)
				{
					// Give up on imports.
					myprintf("can't import movie from url %s\n", source_url.c_str());
					return;
				}
			}
		}

		// Get the imports.
		for (int i = 0; i < count; i++)
		{
			Uint16	id = in->read_u16();
			tu_string symbol_name;
			in->read_string(&symbol_name);
			IF_VERBOSE_PARSE(myprintf("  import: id = %d, name = %s\n", id, symbol_name.c_str()));

			if (s_no_recurse_while_loading)
			{
				m->add_import(source_url, id, symbol_name);
			}
			else
			{
				// @@ TODO get rid of this, always use
				// s_no_recurse_while_loading, change
				// create_movie_sub().

				assert(cast_to<movie_def_impl>(source_movie));
				character_def* res = cast_to<movie_def_impl>(source_movie)->get_exported_resource(symbol_name);
				if (res == NULL)
				{
					IF_VERBOSE_ACTION(myprintf("import error: resource '%s' is not exported from movie '%s'\n", symbol_name.c_str(), source_url.c_str()));
				}
				else if (font* f = cast_to<font>(res))
				{
					// Add this shared font to the currently-loading movie.
					m->add_font(id, f);
				}
				else if (character_def* ch = cast_to<character_def>(res))
				{
					// Add this character to the loading movie.
					m->add_character(id, ch);
				}
				else
				{
					IF_VERBOSE_ACTION(myprintf("import error: resource '%s' from movie '%s' has unknown type\n", symbol_name.c_str(), source_url.c_str()));
				}
			}
		}
	}

	void define_video_loader(stream* in, int tag, movie_definition_sub* m)
	{
		assert(tag == 60); // 60
		Uint16 character_id = in->read_u16();

		video_stream_definition* ch = new video_stream_definition();
		ch->read(in, tag, m);

		m->add_character(character_id, ch);
	}

	void video_loader(stream* in, int tag, movie_definition_sub* m)
	{
		assert(tag == 61); // 61

		Uint16 character_id = in->read_u16();
		character_def* chdef = m->get_character_def(character_id);

		video_stream_definition* ch = cast_to<video_stream_definition>(chdef);
		assert(ch != NULL);

		ch->read(in, tag, m);
	}

	void sound_stream_head_loader(stream* in, int tag, movie_definition_sub* m)
	{
		assert(tag == 18 || tag == 45);

		in->read_u8();	// player may ignore this byte
		sound_handler::format_type format = (sound_handler::format_type) in->read_uint(4);
		int	sample_rate = in->read_uint(2);	// multiples of 5512.5
		bool sample_16bit = in->read_uint(1) ? true : false;
		bool stereo = in->read_uint(1) ? true : false;
		int	sample_count = in->read_u16();
		int  latency_seek = 0;
		if (format == sound_handler::FORMAT_MP3)
		{
			latency_seek = in->read_s16();
		}

		IF_VERBOSE_PARSE(myprintf("define stream sound: format=%d, rate=%d, 16=%d, stereo=%d, ct=%d\n", int(format), sample_rate, int(sample_16bit), int(stereo), sample_count));


		sound_handler* sound = get_sound_handler();
//	where will be deleted sound ?
		if (sound && sample_count > 0)
		{
			int	handler_id = sound->create_sound(NULL, 0, sample_count, format, get_sample_rate(sample_rate), stereo);

			m->m_ss_id = handler_id;
			m->m_ss_format = format;
		}
	}

	void sound_stream_block_loader(stream* in, int tag, movie_definition_sub* m)
	{
		assert(tag == 19);

		// no sound
		if (m->m_ss_id < 0)
		{
			return;
		}

		if (m->m_ss_start == -1)
		{
			m->m_ss_start = m->get_loading_frame();
		}

		int sample_count = 0;
		int seek_samples = 0;
		if (m->m_ss_format == sound_handler::FORMAT_MP3)	//MP3
		{
			sample_count = in->read_u16();
			seek_samples = in->read_s16();
		}

		int data_size = in->get_tag_end_position() - in->get_position();
		if (data_size > 0)
		{
//			myprintf("frame=%d, samples=%d, data_size=%d, seek_samples=%d\n",
//				m->get_loading_frame(), sample_count, data_size, seek_samples);

			Uint8* data = new Uint8[data_size];

			for (int i = 0; i < data_size; i++)
			{
				data[i] = in->read_u8();
			}

			sound_handler* sound = get_sound_handler();
			if (sound)
			{
				sound->append_sound(m->m_ss_id, data, data_size);
			}

			delete [] data;
		}
	}

	void define_metadata_loader(stream* in, int tag, movie_definition_sub* m)
	{
		// Flash help says: Flash Player always ignores the Metadata tag.
		assert(tag == 77);
		tu_string str;
		in->read_string(&str);
	}

	// Bitmap character def implementation

	bitmap_character_def::bitmap_character_def(movie_definition* rdef, Uint8* tag, int tag_length, int tag_type) :
		m_tag(tag),
		m_tag_length(tag_length),
		m_tag_type(tag_type),
		m_jpegloader(NULL)
	{
		if (s_preload_bitmaps)
		{
			m_bitmap_info = get_bitmap_info();
			m_bitmap_info_keep = m_bitmap_info;
		}
	}

	bitmap_character_def::bitmap_character_def(movie_definition* rdef, Uint8* tag, int tag_length, int tag_type, jpeg::input* jpegloader) :
		m_tag(tag),
		m_tag_length(tag_length),
		m_tag_type(tag_type),
		m_jpegloader(jpegloader)
	{
		if (s_preload_bitmaps || jpegloader != NULL)
		{
			m_bitmap_info = get_bitmap_info();
			m_bitmap_info_keep = m_bitmap_info;
		}
	}

	bitmap_character_def::~bitmap_character_def()
	{
		free(m_tag);
	}

	bitmap_character_def::bitmap_character_def(movie_definition* rdef, bitmap_info* bi) :
		m_bitmap_info(bi),
		m_tag(NULL),
		m_tag_length(0),
		m_tag_type(0),
		m_jpegloader(NULL)
	{
/*		if (get_log_bitmap_info() == true)
		{
			static int s_file_no = 1;
			static int s_total = 0;

			int w = bi->get_width();
			int h = bi->get_height();

			// log bitmap info
			s_total += (p2(w) * p2(h)) >> 10;		// KB
			myprintf("bitmap #%04d: w=%d, h=%d, p2w=%d, p2h=%d, mem=%dK, total=%dK\n",	
				s_file_no, w, h, p2(w), p2(h), (p2(w) * p2(h)) >> 10, s_total);

		}*/
	}

	bool	bitmap_character_def::point_test_local(float x, float y)
	// Return true if the specified point is on the interior of our shape.
	// Incoming coords are local coords.
	{
		rect coords;
		coords.m_x_min = 0.0f;
		coords.m_x_max = PIXELS_TO_TWIPS(m_bitmap_info->get_width());
		coords.m_y_min = 0.0f;
		coords.m_y_max = PIXELS_TO_TWIPS(m_bitmap_info->get_height());
		if (coords.point_test(x, y))
		{
			return true;
		}
		return false;
	}

	void bitmap_character_def::get_bound(rect* bound)
	{
		bound->m_x_min = 0.0f;
		bound->m_x_max = PIXELS_TO_TWIPS(m_bitmap_info->get_width());
		bound->m_y_min = 0.0f;
		bound->m_y_max = PIXELS_TO_TWIPS(m_bitmap_info->get_height());
	}

	void	bitmap_character_def::display(character* ch)
	{
		rect coords;
		coords.m_x_min = 0.0f;
		coords.m_x_max = PIXELS_TO_TWIPS(m_bitmap_info->get_width());
		coords.m_y_min = 0.0f;
		coords.m_y_max = PIXELS_TO_TWIPS(m_bitmap_info->get_height());

		rect uv_coords;
		uv_coords.m_x_min = 0.0f;
		uv_coords.m_x_max = m_bitmap_info->get_xratio();
		uv_coords.m_y_min = 0.0f;
		uv_coords.m_y_max = m_bitmap_info->get_yratio();

		cxform cx;
		ch->get_world_cxform(&cx);
		rgba color = cx.transform(bakeinflash::rgba());
		matrix m;
		ch->get_world_matrix(&m);
		render::draw_bitmap(m, m_bitmap_info.get(), coords,	uv_coords, color);
	}

	bitmap_info*	bitmap_character_def::get_bitmap_info()
	{
		// bitmap postloader
		if (m_bitmap_info == NULL)
		{
			tu_file* fi = NULL;
			stream* in = NULL;
			if (m_tag)
			{
				// postload ?
				fi = new tu_file(tu_file::memory_buffer, m_tag_length, m_tag);
				in = new stream(fi);
			}

			switch (m_tag_type)
			{
				default:
					assert(0);
					break;

				case 6:
				{
					image::rgb*	im = NULL;
					if (m_jpegloader == NULL)
					{
						// there are no jpeg tables
						im = image::read_jpeg(in->get_underlying_stream());
					}
					else
					{
						// Flash < 9 ??
						m_jpegloader->discard_partial_buffer();
						im = image::read_swf_jpeg2_with_tables(m_jpegloader);
					}
					m_bitmap_info = render::create_bitmap_info(im);
					break;
				}

				case 21:
				{
			//		IF_VERBOSE_PARSE(myprintf("  define_bits_jpeg2_loader: charid = %d pos = 0x%x\n", character_id, in->get_position()));
					image::rgb* im = image::read_swf_jpeg2(in->get_underlying_stream());
					m_bitmap_info = render::create_bitmap_info(im);
					break;
				}

				case 35:
				{
					Uint32	jpeg_size = in->read_u32();
					Uint32	alpha_position = in->get_position() + jpeg_size;
					const Uint8* zimage = m_tag + 4 + jpeg_size;
					int zlen = m_tag_length - 4 - jpeg_size;

					//
					// Read the image data.
					//

					// Read rgb data.
					image::rgba*	im = image::read_swf_jpeg3(in->get_underlying_stream());

					// Read alpha channel.
					//in->set_position(alpha_position);

					int	buffer_bytes = im->m_width * im->m_height;
					Uint8*	buffer = new Uint8[buffer_bytes];
					unzip(buffer, buffer_bytes, zimage, zlen);

					Uint8* dst = im->m_data + 3;	  // alpha channel
					for (int i = 0; i < buffer_bytes; i++)
					{
						*dst = buffer[i];
						dst += 4;
					}

					delete [] buffer;
					m_bitmap_info = render::create_bitmap_info(im);

					break;
				}

				case	20:
				case	36:
				{
					Uint8	bitmap_format = in->read_u8();	// 3 == 8 bit, 4 == 16 bit, 5 == 32 bit
					Uint16	width = in->read_u16();
					Uint16	height = in->read_u16();
					const Uint8* zimage = m_tag + 5;
					int zlen = m_tag_length - 5;

			/*
					IF_VERBOSE_PARSE(myprintf("  defbitslossless2: tag_type = %d, id = %d, fmt = %d, w = %d, h = %d\n",
						tag_type,
						character_id,
						bitmap_format,
						width,
						height));
			*/
					//if (m->get_create_bitmaps() == DO_LOAD_BITMAPS)
					{
						if (m_tag_type == 20)
						{
							// RGB image data.
							image::rgb*	image = image::create_rgb(width, height);

							if (bitmap_format == 3)
							{
								// 8-bit data, preceded by a palette.

								const int	bytes_per_pixel = 1;
								int	color_table_size = in->read_u8();
								color_table_size += 1;	// !! SWF stores one less than the actual size

								zimage++;
								zlen--;

								int	pitch = (width * bytes_per_pixel + 3) & ~3;

								int	buffer_bytes = color_table_size * 3 + pitch * height;
								Uint8*	buffer = new Uint8[buffer_bytes];
								unzip(buffer, buffer_bytes, zimage, zlen);

								Uint8*	color_table = buffer;

								for (int j = 0; j < height; j++)
								{
									Uint8*	image_in_row = buffer + color_table_size * 3 + j * pitch;
									Uint8*	image_out_row = image::scanline(image, j);
									for (int i = 0; i < width; i++)
									{
										Uint8	pixel = image_in_row[i * bytes_per_pixel];
										image_out_row[i * 3 + 0] = color_table[pixel * 3 + 0];
										image_out_row[i * 3 + 1] = color_table[pixel * 3 + 1];
										image_out_row[i * 3 + 2] = color_table[pixel * 3 + 2];
									}
								}

								delete [] buffer;
							}
							else 
							if (bitmap_format == 4)
							{
								// 16 bits / pixel
								const int	bytes_per_pixel = 2;
								int	pitch = (width * bytes_per_pixel + 3) & ~3;

								int	buffer_bytes = pitch * height;
								Uint8*	buffer = new Uint8[buffer_bytes];
								unzip(buffer, buffer_bytes, zimage, zlen);

								for (int j = 0; j < height; j++)
								{
									Uint8*	image_in_row = buffer + j * pitch;
									Uint8*	image_out_row = image::scanline(image, j);
									for (int i = 0; i < width; i++)
									{
										Uint16	pixel = image_in_row[i * 2] | (image_in_row[i * 2 + 1] << 8);

										// @@ How is the data packed???	 I'm just guessing here that it's 565!
										image_out_row[i * 3 + 0] = (pixel >> 8) & 0xF8;	// red
										image_out_row[i * 3 + 1] = (pixel >> 3) & 0xFC;	// green
										image_out_row[i * 3 + 2] = (pixel << 3) & 0xF8;	// blue
									}
								}

								delete [] buffer;
							}
							else 
							if (bitmap_format == 5)
							{
								// 32 bits / pixel, input is ARGB format (???)
								const int	bytes_per_pixel = 4;
								int	pitch = width * bytes_per_pixel;

								int	buffer_bytes = pitch * height;
								Uint8*	buffer = new Uint8[buffer_bytes];
								unzip(buffer, buffer_bytes, zimage, zlen);

								// Need to re-arrange ARGB into RGB.
								for (int j = 0; j < height; j++)
								{
									Uint8*	image_in_row = buffer + j * pitch;
									Uint8*	image_out_row = image::scanline(image, j);
									for (int i = 0; i < width; i++)
									{
										image_out_row[i * 3 + 0] = image_in_row[i * 4 + 1];	// R
										image_out_row[i * 3 + 1] = image_in_row[i * 4 + 2];	// G
										image_out_row[i * 3 + 2] = image_in_row[i * 4 + 3];	// B
									}
								}
								delete [] buffer;
							}
							m_bitmap_info = render::create_bitmap_info(image);
						}
						else
						{
							// RGBA image data.
							image::rgba*	image = image::create_rgba(width, height);

							if (bitmap_format == 3)
							{
								// 8-bit data, preceded by a palette.

								const int	bytes_per_pixel = 1;
								int	color_table_size = in->read_u8();
								color_table_size += 1;	// !! SWF stores one less than the actual size

								zimage++;
								zlen--;

								int	pitch = (width * bytes_per_pixel + 3) & ~3;

								int	buffer_bytes = color_table_size * 4 + pitch * height;
								Uint8*	buffer = new Uint8[buffer_bytes];
								unzip(buffer, buffer_bytes, zimage, zlen);

								Uint8*	color_table = buffer;

								for (int j = 0; j < height; j++)
								{
									Uint8*	image_in_row = buffer + color_table_size * 4 + j * pitch;
									Uint8*	image_out_row = image::scanline(image, j);
									for (int i = 0; i < width; i++)
									{
										Uint8	pixel = image_in_row[i * bytes_per_pixel];
										image_out_row[i * 4 + 0] = color_table[pixel * 4 + 0];
										image_out_row[i * 4 + 1] = color_table[pixel * 4 + 1];
										image_out_row[i * 4 + 2] = color_table[pixel * 4 + 2];
										image_out_row[i * 4 + 3] = color_table[pixel * 4 + 3];
									}
								}

								delete [] buffer;
							}
							else if (bitmap_format == 4)
							{
								// 16 bits / pixel
								const int	bytes_per_pixel = 2;
								int	pitch = (width * bytes_per_pixel + 3) & ~3;

								int	buffer_bytes = pitch * height;
								Uint8*	buffer = new Uint8[buffer_bytes];
								unzip(buffer, buffer_bytes, zimage, zlen);

								for (int j = 0; j < height; j++)
								{
									Uint8*	image_in_row = buffer + j * pitch;
									Uint8*	image_out_row = image::scanline(image, j);
									for (int i = 0; i < width; i++)
									{
										Uint16	pixel = image_in_row[i * 2] | (image_in_row[i * 2 + 1] << 8);

										// @@ How is the data packed???	 I'm just guessing here that it's 565!
										image_out_row[i * 4 + 0] = 255;			// alpha
										image_out_row[i * 4 + 1] = (pixel >> 8) & 0xF8;	// red
										image_out_row[i * 4 + 2] = (pixel >> 3) & 0xFC;	// green
										image_out_row[i * 4 + 3] = (pixel << 3) & 0xF8;	// blue
									}
								}

								delete [] buffer;
							}
							else if (bitmap_format == 5)
							{
								// 32 bits / pixel, input is ARGB format
								unzip(image->m_data, width * height * 4, zimage, zlen);

								// save a bitmap
								//FILE* f=fopen("C:\\Users\\vitaly\\Documents\\cursor.txt","w");
								// Need to re-arrange ARGB into RGBA.
								for (int j = 0; j < height; j++)
								{
									int*	image_row = (int*) image::scanline(image, j);
									for (int i = 0; i < width; i++)
									{
										if (image_row[i])
										{
											image_row[i] = ror(image_row[i], 8);
									//		fprintf(f, "0x%X,", image_row[i]);
										}
									//	else
									//	{
									//		fprintf(f, "0x00000000,");
									//	}
									}
									//fprintf(f, "\n");
								}
								//fclose(f);

							}
							m_bitmap_info = render::create_bitmap_info(image);
						}
					}
					break;
				}
			}
			delete in;
			delete fi;
		}
		return m_bitmap_info.get();
	}

	//
	//	generic character
	//

	generic_character::generic_character(character_def* def, character* parent, int id) :
		character(parent, id),
		m_def(def),
		m_rgba(NULL)
	{
		assert(m_def != NULL);
		//			m_def->unzip_bitmaps();
	}

	generic_character::~generic_character()
	{
		delete m_rgba;
		m_def->clear_bitmaps();
	}

	void	generic_character::display()
	{
		if (is_visible())
		{
			m_def->unzip_bitmaps();
			m_def->display(this);	// pass in transform info
		}
	}

	bool generic_character::get_topmost_mouse_entity( character * &te, float x, float y)
	{
		const matrix&	m = get_matrix();
		point	p;
		m.transform_by_inverse(&p, point(x, y));

		if (m_def->point_test_local(p.m_x, p.m_y))
		{
			// The mouse is inside the shape.
			te = this;
			return true;
		}
		return false;
	}

	character_def* generic_character::get_character_def()
	{
		return m_def.get();	
	}

	rgba* generic_character::get_rgba()
	{
		return m_rgba;
	}

	void generic_character::set_rgba(const rgba& color)
	{
		delete m_rgba;
		m_rgba = new rgba();
		*m_rgba = color;
	}

	void generic_character::load_bitmaps(array < smart_ptr<bitmap_info> >* bitmap_hash)
	{
		if (m_def)
		{
			m_def->load_bitmaps(bitmap_hash);
		}
	}

	// AVM2
	void	symbol_class_loader(stream* in, int tag_type, movie_definition_sub* m)
	// The SymbolClass tag creates associations between symbols in the SWF file
	// and ActionScript 3.0 classes.
	{
		assert(tag_type == 76);

		int num_symbols = in->read_u16();
		for (int i = 0; i < num_symbols; i++)
		{
			Uint16	character_id = in->read_u16();
			tu_string class_name;
			in->read_string(&class_name);
			m->add_symbol_class(character_id, class_name);

			IF_VERBOSE_PARSE(myprintf("  symbol_class_loader: charid = %d class = '%s'\n",
				character_id, class_name.c_str()));
		}
	}

	// AVM2
	void	define_scene_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(tag_type == 86);

		IF_VERBOSE_PARSE(myprintf("\n  define_scene_loader\n"));

		int scene_count = in->read_vs32();
		for (int i = 0; i < scene_count; i++)
		{
			int offset = in->read_vs32();
			tu_string scene_name;
			in->read_string(&scene_name);
			m->add_scene(offset, scene_name);

			IF_VERBOSE_PARSE(myprintf("  scene #%d, name='%s'\n", offset, scene_name.c_str()));
		}
		
		int frame_label_count = in->read_vs32();
		for (int i = 0; i < frame_label_count; i++)
		{
			int frame = in->read_vs32();
			tu_string frame_label;
			in->read_string(&frame_label);
			m->add_frame_label(frame, frame_label);

			IF_VERBOSE_PARSE(myprintf("  frame #%d, label='%s'\n", frame, frame_label.c_str()));
		}

	}



}

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
// /System/Library/Fonts/Cache), 
