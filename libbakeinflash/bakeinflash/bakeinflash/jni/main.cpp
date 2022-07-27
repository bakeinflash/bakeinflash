
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>
#include <keycodes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "base/utf8.h"
#include "bakeinflash/bakeinflash.h"
#include <stdlib.h>
#include <stdio.h>
#include "base/utility.h"
#include "base/container.h"
#include "base/tu_file.h"
#include "base/tu_types.h"
#include "base/tu_timer.h"
#include "bakeinflash/bakeinflash_types.h"
#include "bakeinflash/bakeinflash_impl.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_freetype.h"
#include "bakeinflash/bakeinflash_object.h"
#include "bakeinflash/bakeinflash_as_classes/as_netstream.h"
#include "bakeinflash/bakeinflash_sound_handler_openal.h"

#include <android/log.h>
//#include "trial_swf.h"
//#include "FutureDashApp.h"

#if TU_CONFIG_LINK_TO_PLAYREADY == 1
#include <drmtypes.h>
#include "piff/msplayer_piff.h"
#endif

float s_scale = 1;
Uint32	start_ticks = 0;
Uint32	last_ticks = 0;
tu_string flash_vars;
int s_screen_width = 0;
int s_screen_height = 0;
int s_x0 = 0;
int s_y0 = 0;
int s_view_width =  0;
int s_view_height =  0;
int s_real_fps = 0;

tu_string dataDir;

using namespace bakeinflash;

static smart_ptr<as_object> thisPtr;
static smart_ptr<root>	m;
static bool isSwfLoaded = false;
static int oldMouseState = 0;

sound_handler*	sh = NULL;

JNIEnv* s_jenv = NULL;
jclass s_class = NULL;
tu_string s_pkgName;
smart_ptr<bitmap_info> bi;
bool s_has_virtual_keyboard = false;

#if TU_CONFIG_LINK_TO_PLAYREADY == 1
// defines drm home dir with certificates & hds file
extern DRM_CONST_STRING g_dstrDrmPath; 
#endif

static void	message_log(const char* message)
	// Process a log message.
{
	//	alog(message);
	__android_log_print(ANDROID_LOG_INFO, "bakeinflash", "%s", message);
}

static void	log_callback(bool error, const char* message)
	// Error callback for handling bakeinflash messages.
{
	if (error)
	{
		__android_log_print(ANDROID_LOG_ERROR, "bakeinflash", "%s", message);
	}
	else
	{
		__android_log_print(ANDROID_LOG_INFO, "bakeinflash", "%s", message);
	}
}

static string_hash<tu_string> s_fontfile_map;
bool get_fontfile(const tu_string& key, tu_string* fontfile)
{
	if (s_fontfile_map.get(key, fontfile))
	{
		//		printf("get_fontfile: %s=%s\n", key.c_str(), fontfile->c_str());
		//		*fontfile = tu_string([bakeinFlash getWorkdir]) + (*fontfile);
		__android_log_print(ANDROID_LOG_INFO, "bakeinflash", "font: %s, file: %s\n", key.c_str(), fontfile->c_str());

		return true;
	}
	return false;
}

static tu_file*	file_opener(const char* url)
	// Callback function.  This opens files for the bakeinflash library.
{
	// absoluteb path ?
	if (*url == '/')
	{
		return new tu_file(url, "r");
	}
	else
	{
		tu_string fi = get_workdir();
		fi += url;
		return new tu_file(fi.c_str(), "r");
	}
}

static key::code	translate_key(Uint16 key)
// For forwarding SDL key events to bakeinflash.
{
	key::code	c(key::INVALID);

	if (key >= 48 && key <= 57)
	{
		get_root()->m_shift_key_state = false;	// hack
		c = (key::code) ((key - 48) + key::_0);
		return c;
	}
	else if (key >= 65 && key <= (65 + 25))		// A..Z
	{
		get_root()->m_shift_key_state = true;	// hack
		c = (key::code) ((key - 65) + key::A);
		return c;
	}
	else if (key >= 97 && key <= 122)		// a..z
	{
		get_root()->m_shift_key_state = false;	// hack
		c = (key::code) ((key - 97) + key::A);
		return c;
	}
//	else if (key >= SDLK_F1 && key <= SDLK_F15)
//	{
//		c = (key::code) ((key - SDLK_F1) + key::F1);
//		return c;
//	}
	else if (key >= 1040 && key < (1040 + 32))
	{
		get_root()->m_shift_key_state = true;	// hack
		c = (key::code) ((key - 1040) + key::RUS_1);
		return c;
	}
	else if (key >= 1072 && key < (1072 + 32))
	{
		get_root()->m_shift_key_state = false;	// hack
		c = (key::code) ((key - 1072) + key::RUS_a1);
		return c;
	}

	// many keys don't correlate, so just use a look-up table.
	struct
	{
		int code;
		bool shift;
		key::code	gs;
	} table[] =
	{
		{ 8, true, key::BACKSPACE },
		{ 13, true, key::ENTER },
		{ 32, true, key::SPACE },
		{ 33, true, key::_1 },
		{ 34, true, key::QUOTE },
		{ 35, true, key::_3 },
		{ 36, true, key::_4 },
		{ 37, true, key::_5 },
		{ 38, true, key::_7 },
		{ 39, false, key::QUOTE },
		{ 40, true, key::_9 },
		{ 41, true, key::_0 },
		{ 42, true, key::_8 },
		{ 43, true, key::EQUALS },
		{ 44, false, key::COMMA },
		{ 45, false, key::MINUS },
		{ 46, false, key::PERIOD },
		{ 47, false, key::SLASH },
		{ 58, true, key::SEMICOLON },
		{ 59, false, key::SEMICOLON },
		{ 60, true, key::COMMA },
		{ 61, false, key::EQUALS },
		{ 62, true, key::PERIOD },
		{ 63, true, key::SLASH },
		{ 64, true, key::_2 },
		{ 91, false, key::LEFT_BRACKET },
		{ 92, false, key::BACKSLASH },
		{ 93, false, key::RIGHT_BRACKET },
		{ 94, true, key::_6 },
		{ 95, true, key::MINUS },
		{ 96, false, key::BACKTICK },
		{ 123, true, key::LEFT_BRACKET },
		{ 124, true, key::BACKSLASH },
		{ 125, true, key::RIGHT_BRACKET },
		{ 126, true, key::BACKTICK },

		// @@ TODO fill this out some more
		{ 0, false, key::INVALID }	};


	for (int i = 0; table[i].code != 0; i++)
	{
		if (key == table[i].code)
		{
			c = table[i].gs;
			get_root()->m_shift_key_state = table[i].shift;	// hack
			break;
		}
	}

	return c;
}

static void	fs_callback(character* movie, const char* command, const char* args)
	// For handling notification callbacks from ActionScript.
{
//	printf("fscommand: %s %s\n", command, args);
	tu_string cmd = command;
	if (cmd == "setIdleTimer")
	{
	}
	else
	if (cmd == "wakeUp")
	{
	}
	else
	if (cmd == "setInterval")
	{
	}
	else
	if (cmd == "alert")
	{
	}
	else
	if (cmd == "setFontFace")
	{
	}
  else
  if (cmd == "set_workdir")
  {
    bakeinflash::set_workdir(args);
  }
	else
	if (cmd == "fps")
	{
		root* rm = get_root();
		int fps = atoi(args);
		rm->set_frame_rate(fps);
	}
	else
	if (cmd  == "notify_keydown")
	{
		Uint16 ch = utf8::decode_next_unicode_character(&args);
		key::code c = translate_key(ch);
		if (c != key::INVALID)
		{
			notify_key_event(c, c, true);
		}
	}
	else
	if (cmd == "notify_keyup")
	{
		Uint16 ch = utf8::decode_next_unicode_character(&args);
		key::code c = translate_key(ch);
		if (c != key::INVALID)
		{
			notify_key_event(c, c, false);
		}
	}

}

//void attach_NGTServer(const char* sqlite_db);

bool bakeinflash_init(bool hasStencil)
{
	//	s_fontfile_map.add("Myriad Pro", "myriadproregular.ttf");
	//	s_fontfile_map.add("Myriad Pro Bold", "myriadprobold.ttf");
	//	s_fontfile_map.add("Arial", "Arial.ttf");
	//	s_fontfile_map.add("Arial Bold", "ArialBD.ttf");
	//		s_fontfile_map.add("Arial.ttf", "C:\\WINDOWS\\Fonts\\ArialUNI.ttf");
	//	s_fontfile_map.add("Tuffy Regular", "Tuffy_Regular.ttf");
	//	s_fontfile_map.add("Tuffy Bold", "Tuffy_Bold.ttf");
	//	s_fontfile_map.add("Impact", "Impact.ttf");

	bakeinflash::preload_bitmaps(false);
	bakeinflash::keep_alive_bitmaps(true);
	bakeinflash::set_display_invisibles(false);
	bakeinflash::set_yuv2rgb_table(true);

	init_player();

	//pl->add_class("fDash", bakeinflash::as_fdash_ctor);

	set_workdir(dataDir + "/");
	set_startdir(dataDir + "/");
	myprintf("workdir: '%s'\n", get_workdir());

#if TU_CONFIG_LINK_TO_PLAYREADY == 1
	g_dstrDrmPath.cchString = strlen(get_workdir());
	g_dstrDrmPath.pwszString = (DRM_WCHAR*) piff::charToWord(get_workdir());

	tu_string infile = tu_string(get_workdir()) + tu_string("SuperSpeedway_720_230.ismv");
	tu_string outfile = tu_string(get_workdir()) + tu_string("SuperSpeedway_720_230d.ismv");
	tu_string hdsfile = tu_string(get_workdir()) + tu_string("my.hds");
	long rc = decode_piff(infile, outfile, hdsfile);
#endif

	register_file_opener_callback(file_opener);
	register_fscommand_callback(fs_callback);
	register_log_callback(log_callback);

	sh = create_sound_handler_openal();
	set_sound_handler(sh);

	bakeinflash::render_handler*	myrender = create_render_handler_ogles(hasStencil);
	set_render_handler(myrender);

#if TU_CONFIG_LINK_TO_FREETYPE == 1
	set_glyph_provider(create_glyph_provider_freetype());
#else
	set_glyph_provider(create_glyph_provider_tu(pl->get_workdir()));
#endif

	use_cached_movie(false);
	//set_flash_vars(flash_vars);

	tu_string finame = get_workdir();
	finame += "kbd.swf";
	tu_file fi(finame.c_str(), "r");
	if (fi.get_error() == TU_FILE_NO_ERROR)
	{
		s_has_virtual_keyboard = true;
	}

	tu_string inifile = get_workdir();
	inifile += "app.ini";
	tu_file ini(inifile.c_str(), "r");
	if (ini.get_error() == TU_FILE_NO_ERROR)
	{
		int len = ini.size();
		char filename[256];
		int n = ini.read_string(filename, 256);
		if (n > 0)
		{
			m = load_file(filename);

			char flashvars[1024];
			int n = ini.read_string(flashvars, 1024);
			if (n > 0)
			{
				set_flash_vars(flashvars);
			}
		}

	}
	else
	{
		m = load_file("app.swf");
	}

	start_ticks = tu_timer::get_ticks();
	last_ticks = -1000; // start_ticks;

	//printf("screen: %dx%d, movie: %dx%d\n", s_width, s_height, s_view_width, s_view_height);
	//__android_log_print(ANDROID_LOG_INFO, "bakeinflash", "loaded: %d, %d, %d, %d, %d, %d\n",	s_screen_width, s_screen_height, s_view_width, s_view_height, s_x0, s_y0);

//	tu_string sqlitedb = dataDir + "/slot-m.db3";
//	attach_NGTServer(sqlitedb.c_str());

	return true;
}

int frames = 0;
Uint32	prev_frames_ticks = tu_timer::get_ticks();
void advance_bakeinflash()
{
	if (m && isSwfLoaded)
	{
		assert(m);

		Uint32	ticks = tu_timer::get_ticks();
		int	delta_ticks = ticks - last_ticks;
		float	delta_time = delta_ticks / 1000.f;

		frames++;
		if (ticks - prev_frames_ticks >= 1000)
		{
			s_real_fps = (int) (frames * 1000.0f / (ticks - prev_frames_ticks));
			prev_frames_ticks = ticks;
			frames = 0;
			//					printf("fps=%d\n", real_fps);
		}

		last_ticks = ticks;

		float w = m->get_movie_width();
		float h = m->get_movie_height();
		float scale_x = (float) s_screen_width / w;
		float scale_y = (float) s_screen_height / h;
		s_scale = fmin(scale_x, scale_y);

		// move movie to center
		s_view_width =  m->get_movie_width() * s_scale;
		s_view_height =  m->get_movie_height() * s_scale;
		s_x0 =  (s_screen_width - s_view_width) >> 1;
		s_y0 =  (s_screen_height - s_view_height) >> 1;
		m->set_display_viewport(s_x0, s_y0, s_view_width, s_view_height);

//		static int count = 1;
//		if (count > 0)
//		{
//			count--;
//			__android_log_print(ANDROID_LOG_INFO, "bakeinflash", "view: %d, %d, %d, %d, %d, %d\n",	s_screen_width, s_screen_height, s_view_width, s_view_height, s_x0, s_y0);
//		}

		Uint32 t_advance = tu_timer::get_ticks();
		m->advance(delta_time);
		t_advance = tu_timer::get_ticks() - t_advance;

		Uint32 t_display = tu_timer::get_ticks();
		m->display();
		t_display = tu_timer::get_ticks() - t_display;

		// for perfomance testing
		//__android_log_print(ANDROID_LOG_INFO, "bakeinflash", "advance time: %d, display time %d\n",	t_advance, t_display);
	}
}

static key::code	translate_key(int key)
	// For forwarding SDL key events to bakeinflash.
{
	key::code	c(key::INVALID);
	c = (key::code) key;

	if (key >= AKEYCODE_0 && key <= AKEYCODE_9)
	{
		c = (key::code) ((key - AKEYCODE_0) + key::_0);
	}
	else if (key >= AKEYCODE_A && key <= AKEYCODE_Z)
	{
		c = (key::code) ((key - AKEYCODE_A) + key::A);
	}
	else if (key >= AKEYCODE_F1 && key <= AKEYCODE_F12)
	{
		c = (key::code) ((key - AKEYCODE_F1) + key::F1);
	}
	//	else if (key >= SDLK_KP0 && key <= SDLK_KP9)
	//	{
	//		c = (key::code) ((key - SDLK_KP0) + key::KP_0);
	//	}
	else
	{
		// many keys don't correlate, so just use a look-up table.
		struct
		{
			int	sdlk;
			key::code	gs;
		} table[] =
		{
			//   { SDLK_RETURN, key::ENTER },
			{ AKEYCODE_ESCAPE, key::ESCAPE },
			{ AKEYCODE_CTRL_LEFT, key::LEFT },
			//           { AKEYCODE_CTRL_UP, key::UP },
			{ AKEYCODE_CTRL_RIGHT, key::RIGHT },
			//         { AKEYCODE_CTRL_DOWN, key::DOWN },
			{ AKEYCODE_SPACE, key::SPACE },
			{ AKEYCODE_PAGE_DOWN, key::PGDN },
			{ AKEYCODE_PAGE_UP, key::PGUP },
			{ AKEYCODE_HOME, key::HOME },
			{ AKEYCODE_MOVE_END, key::END },
			{ AKEYCODE_INSERT, key::INSERT },
			{ AKEYCODE_DEL, key::DELETEKEY },
			{ AKEYCODE_BACK, key::BACKSPACE },
			{ AKEYCODE_TAB, key::TAB },
			{ AKEYCODE_SHIFT_LEFT, key::SHIFT },
			{ AKEYCODE_SHIFT_RIGHT, key::SHIFT },
			{ AKEYCODE_PERIOD, key::PERIOD },
			{ AKEYCODE_SLASH, key::SLASH },
			{ AKEYCODE_BACKSLASH, key::BACKSLASH },
			{ AKEYCODE_SEMICOLON, key::SEMICOLON },
			//    { AKEYCODE_QUOTE, key::QUOTE },
			{ AKEYCODE_LEFT_BRACKET, key::LEFT_BRACKET },
			{ AKEYCODE_RIGHT_BRACKET, key::RIGHT_BRACKET },
			{ AKEYCODE_COMMA, key::COMMA },

			// @@ TODO fill this out some more
			{ AKEYCODE_UNKNOWN, key::INVALID }
		};

		for (int i = 0; table[i].sdlk != AKEYCODE_UNKNOWN; i++)
		{
			if (key == table[i].sdlk)
			{
				c = table[i].gs;
				break;
			}
		}
	}
	return c;
}

extern "C"
{

	void Java_com_microslots_bakeinflash_sendMessage(JNIEnv* env, jclass envClass, jstring jname, jstring jarg)
	{
		const char* name = env->GetStringUTFChars(jname, NULL);
		const char* arg = env->GetStringUTFChars(jarg, NULL);

		bakeinflash::as_value function;
		bakeinflash::as_object* obj = bakeinflash::get_global();
		if (m && obj && obj->get_member("sendMessage", &function))
		{
			bakeinflash::as_environment env;
			env.push(arg);
			env.push(name);
			bakeinflash::as_value val = bakeinflash::call_method(function, &env, obj, 2, env.get_top_index());
		}	
	}

	void Java_com_microslots_bakeinflash_loadSWF(JNIEnv* env, jclass envClass, jstring pkgName, jstring appPath, bool hasStencil)
	{
		s_jenv = env;
		s_class = envClass;
		s_pkgName = env->GetStringUTFChars(pkgName, NULL);
		dataDir = env->GetStringUTFChars(appPath, NULL);
		isSwfLoaded = bakeinflash_init(hasStencil);
	}

	void Java_com_microslots_bakeinflash_onKeyEvent(JNIEnv*  env, jclass envClass, int keycode, bool state)
	{
		key::code c = translate_key(keycode);
		if (m && c != key::INVALID)
		{
			notify_key_event(c, c, state);
		}
	}

	void Java_com_microslots_bakeinflash_notifyMouseState(JNIEnv*  env, jclass envClass, int xx, int yy, int state)
	{
		// first mouse move
		int x = (xx - s_x0) / s_scale;
		int y = (yy - s_y0) / s_scale;

		//myprintf("onTouch x=%d y=%d state=%d originalx=%d originaly=%d", x, y, state, xx, yy);
		if (m)
		{
			m->notify_mouse_state(x, y, state);
			advance_bakeinflash();	// force mouse move
		}
	}

	// Call to render the next GL frame
	void Java_com_microslots_bakeinflash_render(JNIEnv*  env, jobject obj)
	{
		s_jenv = env;
		if (m)
		{
			advance_bakeinflash();
		}
	}

	// invoke _root.onPause()
	void Java_com_microslots_bakeinflash_pauseSWF(JNIEnv*  env, jobject obj)
	{
		s_jenv = env;
		if (m)
		{
			character* rm = m->get_root_movie();
			if (rm)
			{
				as_value func;
				if (rm->get_member("onPause", &func))
				{
					as_environment env;
					call_method(func, &env, rm, 0, env.get_top_index());
				}
			}
		}
	}

	// invoke _root.onResume()
	void Java_com_microslots_bakeinflash_resumeSWF(JNIEnv*  env, jobject obj)
	{
		s_jenv = env;
		if (m)
		{
			character* rm = m->get_root_movie();
			if (rm)
			{
				as_value func;
				if (rm->get_member("onResume", &func))
				{
					as_environment env;
					call_method(func, &env, rm, 0, env.get_top_index());
				}
			}
		}
	}

	void Java_com_microslots_bakeinflash_setView(JNIEnv*  env, jobject obj, int w, int h)
	{
		s_screen_width = w;
		s_screen_height = h;
	}

}
