// bakeinflash_render_handler_ogles2.cpp	-- Vitaly Alexeev, 2012

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// OpenGL ES2 based video handler for mobile units

#include "base/tu_config.h"
#include "base/tu_file.h"

#if TU_USE_SDL

#ifndef __EMSCRIPTEN__
# include <glew.h>
#endif

#	define GL_GLEXT_PROTOTYPES 1
#	include <SDL2/SDL.h>  // for cursor handling & the scanning for extensions.

#ifdef __EMSCRIPTEN__
#	include <SDL2/SDL_opengles2.h>  // for cursor handling & the scanning for extensions.
#endif


#else
#if ANDROID == 1
#define GL_BGRA                           0x80E1
#include <GLES/gl.h>
#else
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#endif
#endif

#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_types.h"
#include "base/image.h"
#include "base/utility.h"
#include "bakeinflash/bakeinflash_shape.h"

#include <string.h>	// for memset()

extern char s_infile[64];	// hack

#define BUFFER_OFFSET(i) ((char*) NULL + (i))

namespace bakeinflash
{
  const char* get_workdir();
}

namespace render_handler_ogles2
{
  // Pointers to opengl extension functions.
  typedef char GLchar;

#ifndef __EMSCRIPTEN__
  typedef void (APIENTRY* PFNGLBINDVERTEXARRAYOESPROC) (GLuint array);
  PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES = 0;

  typedef void (APIENTRY* PFNGLGENVERTEXARRAYSOESPROC) (GLsizei n, GLuint* arrays);
  PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES = 0;

  typedef void (APIENTRY* PFNGLDELETEVERTEXARRAYSOESPROC) (GLsizei n, GLuint* arrays);
  PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES = 0;
#endif

#if 0 //def WIN32
  typedef void (APIENTRY* PFNGLDELETEPROGRAMPROC) (GLuint program);
  PFNGLDELETEPROGRAMPROC glDeleteProgram = 0;

  typedef void (APIENTRY* PFNGLDELETESHADERPROC) (GLuint shader);
  PFNGLDELETESHADERPROC glDeleteShader = 0;

  typedef GLuint(APIENTRY* PFNGLCREATESHADERPROC) (GLenum type);
  PFNGLCREATESHADERPROC glCreateShader = 0;

  typedef void (APIENTRY* PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
  PFNGLSHADERSOURCEPROC glShaderSource = 0;

  typedef void (APIENTRY* PFNGLCOMPILESHADERPROC) (GLuint shader);
  PFNGLCOMPILESHADERPROC glCompileShader = 0;

  typedef void (APIENTRY* PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint* params);
  PFNGLGETSHADERIVPROC glGetShaderiv = 0;

  typedef void (APIENTRY* PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
  PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = 0;

  typedef GLuint(APIENTRY* PFNGLCREATEPROGRAMPROC) (void);
  PFNGLCREATEPROGRAMPROC glCreateProgram = 0;

  typedef void (APIENTRY* PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
  PFNGLATTACHSHADERPROC glAttachShader = 0;

  typedef void (APIENTRY* PFNGLLINKPROGRAMPROC) (GLuint program);
  PFNGLLINKPROGRAMPROC glLinkProgram = 0;

  typedef void (APIENTRY* PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint* params);
  PFNGLGETPROGRAMIVPROC glGetProgramiv = 0;

  typedef void (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
  PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = 0;

  typedef void (APIENTRY* PFNGLVALIDATEPROGRAMPROC) (GLuint program);
  PFNGLVALIDATEPROGRAMPROC glValidateProgram = 0;

  typedef GLint(APIENTRY* PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar* name);
  PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = 0;

  typedef void (APIENTRY* PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
  PFNGLUNIFORM1FPROC glUniform1f = 0;

  typedef void (APIENTRY* PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
  PFNGLUNIFORM1IPROC glUniform1i = 0;

  typedef void (APIENTRY* PFNGLUNIFORM1IVPROC) (GLint  location, GLsizei  count, const GLint* value);
  PFNGLUNIFORM1IVPROC glUniform1iv = 0;

  typedef void (APIENTRY* PFNGLUNIFORM1FVPROC) (GLint  location, GLsizei  count, const GLfloat* value);
  PFNGLUNIFORM1FVPROC glUniform1fv = 0;

  typedef void (APIENTRY* PFNGLUSEPROGRAMPROC) (GLuint program);
  PFNGLUSEPROGRAMPROC glUseProgram = 0;

  typedef void (APIENTRY* PFNGLUNIFORMMATRIX4FV) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat* value);
  PFNGLUNIFORMMATRIX4FV glUniformMatrix4fv = 0;

  typedef void (APIENTRY* PFNGLDETACHSHADER) (GLuint  program, GLuint  shader);
  PFNGLDETACHSHADER glDetachShader = 0;

  typedef void (APIENTRY* PFNGLBINDATTRIBLOCATION) (GLuint  program, GLuint  index, const GLchar* name);
  PFNGLBINDATTRIBLOCATION glBindAttribLocation = 0;

  typedef void (APIENTRY* PFNGLUNIFORM2FV) (GLint  location, GLsizei  count, const GLfloat* value);
  PFNGLUNIFORM2FV glUniform2fv = 0;
  typedef void (APIENTRY* PFNGLUNIFORM4FV) (GLint  location, GLsizei  count, const GLfloat* value);
  PFNGLUNIFORM4FV glUniform4fv = 0;

  typedef void (APIENTRY* PFNGLUNIFORM4F) (GLint  location, GLfloat  v0, GLfloat  v1, GLfloat  v2, GLfloat  v3);
  PFNGLUNIFORM4F glUniform4f = 0;

  typedef void (APIENTRY* PFNGLACTIVETEXTURE) (GLenum  texture);
  PFNGLACTIVETEXTURE glActiveTexture = 0;

#endif

  void create_texture(int format, int w, int h, void* data, int level)
  {
    int internal_format = format;
    glTexImage2D(GL_TEXTURE_2D, level, internal_format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
  }

  // bitmap_info_ogl declaration
  struct bitmap_info_ogl : public bakeinflash::bitmap_info
  {
    unsigned int	m_texture_id;
    int m_width;
    int m_height;
    image::image_base* m_suspended_image;
    int m_bpp;

    bitmap_info_ogl();
    bitmap_info_ogl(int width, int height, Uint8* data);
    bitmap_info_ogl(image::image_base* im);

    virtual void upload();

    // get byte per pixel
    virtual int get_bpp() const
    {
      return m_bpp;
    }

    virtual unsigned char* get_data() const
    {
      if (m_suspended_image)
      {
        return m_suspended_image->m_data;
      }
      return NULL;
    }

    virtual void activate()
    {
      assert(m_texture_id > 0);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, m_texture_id);
    }

    virtual ~bitmap_info_ogl()
    {
      if (m_texture_id > 0)
      {
        glDeleteTextures(1, (GLuint*)&m_texture_id);
        m_texture_id = 0;	// for debuging
      }
      delete m_suspended_image;
    }

    virtual int get_width() const { return m_width; }
    virtual int get_height() const { return m_height; }

  };

  struct render_handler_ogles2 : public bakeinflash::render_handler
  {
    // Some renderer state.

    // Enable/disable antialiasing.
    bool	m_enable_antialias;

    // Output size.
    float	m_display_width;
    float	m_display_height;
    bakeinflash::matrix	m_current_matrix;
    bakeinflash::cxform	m_current_cxform;
    bakeinflash::rgba* m_current_rgba;

    int m_mask_level;	// nested mask level

    // shaders

    GLuint m_prg;
    GLint m_modelViewProjectionMatrix;		// unform
    GLint m_modelColor;	// uniform
    GLint m_modelTexture;	// uniform
    int m_texture_id;
    GLint m_position;
    GLint m_texcoords;
    GLint m_modelMode;

    GLuint vertexArray;
    GLuint vertexBuffer;
    GLfloat vertexData;

    render_handler_ogles2() :
      m_enable_antialias(false),
      m_display_width(0),
      m_display_height(0),
      m_mask_level(0),
      m_texture_id(0)
    {
    }

    virtual ~render_handler_ogles2()
    {
    }

    void display(Uint8* data, int width, int height,
      const bakeinflash::matrix* m, const bakeinflash::rect* bounds, const bakeinflash::rgba& color)
    {
      assert(0);
    }

    void open()
    {
#if 0
      // Scan for extensions used by bakeinflash
      glDeleteProgram = (PFNGLDELETEPROGRAMPROC)SDL_GL_GetProcAddress("glDeleteProgram");
      glDeleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
      glCreateShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
      glShaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
      glCompileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
      glGetShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
      glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
      glCreateProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
      glAttachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
      glLinkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
      glGetProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
      glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
      glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)SDL_GL_GetProcAddress("glValidateProgram");
      glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
      glUseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
      glUniform1f = (PFNGLUNIFORM1FPROC)SDL_GL_GetProcAddress("glUniform1f");
      glUniform1i = (PFNGLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
      glUniform1iv = (PFNGLUNIFORM1IVPROC)SDL_GL_GetProcAddress("glUniform1iv");
      glUniform1fv = (PFNGLUNIFORM1FVPROC)SDL_GL_GetProcAddress("glUniform1fv");
      glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FV)SDL_GL_GetProcAddress("glUniformMatrix4fv");
      glDetachShader = (PFNGLDETACHSHADER)SDL_GL_GetProcAddress("glDetachShader");
      glBindAttribLocation = (PFNGLBINDATTRIBLOCATION)SDL_GL_GetProcAddress("glBindAttribLocation");
      glUniform2fv = (PFNGLUNIFORM2FV)SDL_GL_GetProcAddress("glUniform2fv");
      glUniform4fv = (PFNGLUNIFORM4FV)SDL_GL_GetProcAddress("glUniform4fv");
      glUniform4f = (PFNGLUNIFORM4F)SDL_GL_GetProcAddress("glUniform4f");
      glActiveTexture = (PFNGLACTIVETEXTURE)SDL_GL_GetProcAddress("glActiveTexture");
#endif

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
      glBindVertexArrayOES = (PFNGLBINDVERTEXARRAYOESPROC)SDL_GL_GetProcAddress("glBindVertexArrayOES");
      glDeleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOESPROC)SDL_GL_GetProcAddress("glDeleteVertexArraysOES");
      glGenVertexArraysOES = (PFNGLGENVERTEXARRAYSOESPROC)SDL_GL_GetProcAddress("glGenVertexArraysOES");
#endif

      // Fragment Shader
      const GLchar* fs_src = R"(
        precision mediump float;

        varying vec4 color;
        varying vec2 texCoords;
        varying vec2 mode;

        uniform sampler2D modelTexture;

        void main()
        {
          if (mode.x == 1.0)    // bitmap
            gl_FragColor = texture2D(modelTexture, texCoords) * color;
          else if (mode.x == 2.0) // alpha
            gl_FragColor = texture2D(modelTexture, texCoords).aaaa * color;
          else
            gl_FragColor = color;   // shape
        }
      )";


      // Vertex Shader
      const GLchar* vs_src = R"(
        attribute vec2 position;
        attribute vec2 texcoords;

        uniform mat4 modelViewProjectionMatrix;
        uniform vec4 modelColor;
        uniform vec2 modelMode;

        varying vec4 color;
        varying vec2 texCoords;
        varying vec2 mode;

        void main()
        {
          color = modelColor;
          mode = modelMode;
          texCoords = texcoords;
          gl_Position = modelViewProjectionMatrix * vec4(position.x, position.y, 0, 1);
        }
      )";


      GLchar msg[256] = {};

      GLint vs = glCreateShader(GL_VERTEX_SHADER);
      assert(vs);

      glShaderSource(vs, 1, &vs_src, NULL);
      glCompileShader(vs);
      GLint rc;
      glGetShaderiv(vs, GL_COMPILE_STATUS, &rc);
      if (!rc)
      {
        glGetShaderInfoLog(vs, sizeof(msg), NULL, msg);
        printf("Vertex Shader error: %s\n", msg);
      }

      GLint fs = glCreateShader(GL_FRAGMENT_SHADER);
      assert(fs);

      glShaderSource(fs, 1, &fs_src, NULL);
      glCompileShader(fs);
      glGetShaderiv(fs, GL_COMPILE_STATUS, &rc);
      if (!rc)
      {
        glGetShaderInfoLog(fs, sizeof(msg), NULL, msg);
        printf("Fragment Shader error: %s\n", msg);
      }

      m_prg = glCreateProgram();
      assert(m_prg);

      glAttachShader(m_prg, vs);
      glAttachShader(m_prg, fs);

      glLinkProgram(m_prg);
      glGetProgramiv(m_prg, GL_LINK_STATUS, &rc);
      if (!rc)
      {
        glGetProgramInfoLog(m_prg, sizeof(msg), NULL, msg);
        printf("glLinkProgram error: %s\n", msg);
      }

      glUseProgram(m_prg);

      // Get uniform locations.
      m_modelViewProjectionMatrix = glGetUniformLocation(m_prg, "modelViewProjectionMatrix");
      //assert(m_modelViewProjectionMatrix >= 0);
      m_modelColor = glGetUniformLocation(m_prg, "modelColor");
      //assert(m_modelColor >= 0);
      m_modelTexture = glGetUniformLocation(m_prg, "modelTexture");
      //assert(m_modelTexture >= 0);
      m_modelMode = glGetUniformLocation(m_prg, "modelMode");
      // assert(m_modelMode >= 0);
      m_position = glGetAttribLocation(m_prg, "position");
      // assert(m_position >= 0);
      m_texcoords = glGetAttribLocation(m_prg, "texcoords");
      //assert(m_texcoords >= 0);

      glActiveTexture(GL_TEXTURE0);
      glUniform1i(m_modelTexture, 0);

      // Release vertex and fragment shaders.
      if (vs)
      {
        //glDetachShader(m_prg, vs);
        //glDeleteShader(vs);
      }
      if (fs)
      {
        //glDetachShader(m_prg, fs);
        //glDeleteShader(fs);
      }

      glGenVertexArraysOES(1, &vertexArray);
      glBindVertexArrayOES(vertexArray);

      glGenBuffers(1, &vertexBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    }

    void set_antialiased(bool enable)
    {
      m_enable_antialias = enable;
    }

    static void make_next_miplevel(int* width, int* height, Uint8* data)
      // Utility.  Mutates *width, *height and *data to create the
      // next mip level.
    {
      assert(width);
      assert(height);
      assert(data);

      int	new_w = *width >> 1;
      int	new_h = *height >> 1;
      if (new_w < 1) new_w = 1;
      if (new_h < 1) new_h = 1;

      if (new_w * 2 != *width || new_h * 2 != *height)
      {
        // Image can't be shrunk along (at least) one
        // of its dimensions, so don't bother
        // resampling.	Technically we should, but
        // it's pretty useless at this point.  Just
        // change the image dimensions and leave the
        // existing pixels.
      }
      else
      {
        // Resample.  Simple average 2x2 --> 1, in-place.
        for (int j = 0; j < new_h; j++) {
          Uint8* out = ((Uint8*)data) + j * new_w;
          Uint8* in = ((Uint8*)data) + (j << 1) * *width;
          for (int i = 0; i < new_w; i++) {
            int	a;
            a = (*(in + 0) + *(in + 1) + *(in + 0 + *width) + *(in + 1 + *width));
            *(out) = a >> 2;
            out++;
            in += 2;
          }
        }
      }

      // Munge parameters to reflect the shrunken image.
      *width = new_w;
      *height = new_h;
    }

    struct fill_style
    {
      enum mode
      {
        INVALID,
        COLOR,
        BITMAP_WRAP,
        BITMAP_CLAMP,
        LINEAR_GRADIENT,
        RADIAL_GRADIENT,
      };
      mode	m_mode;
      bakeinflash::rgba	m_color;
      bakeinflash::bitmap_info* m_bitmap_info;
      bakeinflash::matrix	m_bitmap_matrix;
      bakeinflash::cxform	m_bitmap_color_transform;
      float m_width;	// for line style
      int m_caps_style;

      mutable float	pS[4];
      mutable float	pT[4];

      fill_style() :
        m_mode(INVALID)
      {
      }

      float* gentexcoords(int primitive_type, const void* coords, int vertex_count) const
      {
        GLfloat* tcoord = NULL;
        if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
        {
          tcoord = new GLfloat[2 * vertex_count];
          float* vcoord = (float*)coords;
          for (int i = 0, n = 2 * vertex_count; i < n; i = i + 2)
          {
            tcoord[i] = vcoord[i] * pS[0] + vcoord[i + 1] * pS[1] + pS[3];
            tcoord[i + 1] = vcoord[i] * pT[0] + vcoord[i + 1] * pT[1] + pT[3];
          }
        }
        return tcoord;
      }

      void	apply(int uniformColor, int uniformTex) const
        // Push our style into OpenGL.
      {
        assert(m_mode != INVALID);

        if (m_mode == COLOR)
        {
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glUniform4f(uniformColor, m_color.m_r / 255.0, m_color.m_g / 255.0, m_color.m_b / 255.0, m_color.m_a / 255.0);
          //glDisable(GL_TEXTURE_2D);
        }
        else
          if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
          {
            assert(m_bitmap_info != NULL);
            if (m_bitmap_info == NULL)
            {
              //glDisable(GL_TEXTURE_2D);
            }
            else
            {
              glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

              // The additive color also seems to be modulated by the texture. So,
              // maybe we can fake this in one pass using using the mean value of 
              // the colors: c0*t+c1*t = ((c0+c1)/2) * t*2
              // I don't know what the alpha component of the color is for.

              // Ú‡Í Í‡Í ËÒÔÓÎ¸ÁÛÂÚÒˇ GL_ONE Ì‡‰Ó ‚˚˜ËÒÎËÚ¸ ÍÓÏÔÓÌÂÌÚ˚ ˆ‚ÂÚ‡ Á‡‡ÌÂÂ
              bakeinflash::rgba c;
              c.m_a = iclamp(int(m_color.m_a + m_bitmap_color_transform.m_[3][1]), 0, 255);
              float a = fclamp(c.m_a / 255.0, 0, 1);
              c.m_r = int(fclamp(m_color.m_r + m_bitmap_color_transform.m_[0][1], 0, 255) * a);
              c.m_g = int(fclamp(m_color.m_g + m_bitmap_color_transform.m_[1][1], 0, 255) * a);
              c.m_b = int(fclamp(m_color.m_b + m_bitmap_color_transform.m_[2][1], 0, 255) * a);

              //		apply_color(c);
              glUniform4f(uniformColor, c.m_r / 255.0, c.m_g / 255.0, c.m_b / 255.0, c.m_a / 255.0);

              m_bitmap_info->upload();
              if (m_mode == BITMAP_CLAMP)
              {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
              }
              else
              {
                assert(m_mode == BITMAP_WRAP);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
              }

              // Set up the bitmap matrix for texgen.

              float	inv_width = 1.0f / m_bitmap_info->get_width();
              float	inv_height = 1.0f / m_bitmap_info->get_height();

              const bakeinflash::matrix& m = m_bitmap_matrix;
              //		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
              //	float	p[4] = { 0, 0, 0, 0 };
              pS[0] = m.m_[0][0] * inv_width;
              pS[1] = m.m_[0][1] * inv_width;
              pS[2] = 0;
              pS[3] = m.m_[0][2] * inv_width;
              //	glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
              //	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
              pT[0] = m.m_[1][0] * inv_height;
              pT[1] = m.m_[1][1] * inv_height;
              pT[2] = 0;
              pT[3] = m.m_[1][2] * inv_height;
              //	glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
            }
          }
      }

      void	disable() { m_mode = INVALID; }
      void	set_color(bakeinflash::rgba color) { m_mode = COLOR; m_color = color; }
      void	set_bitmap(bakeinflash::bitmap_info* bi, const bakeinflash::matrix& m, bitmap_wrap_mode wm, const bakeinflash::cxform& color_transform)
      {
        m_mode = (wm == WRAP_REPEAT) ? BITMAP_WRAP : BITMAP_CLAMP;
        m_bitmap_info = bi;
        m_bitmap_matrix = m;
        m_bitmap_color_transform = color_transform;
        m_bitmap_color_transform.clamp();

        m_color = bakeinflash::rgba(
          Uint8(m_bitmap_color_transform.m_[0][0] * 255.0f),
          Uint8(m_bitmap_color_transform.m_[1][0] * 255.0f),
          Uint8(m_bitmap_color_transform.m_[2][0] * 255.0f),
          Uint8(m_bitmap_color_transform.m_[3][0] * 255.0f));
      }
      bool	is_valid() const { return m_mode != INVALID; }
    };


    // Style state.
    enum style_index
    {
      LEFT_STYLE = 0,
      RIGHT_STYLE,
      LINE_STYLE,

      STYLE_COUNT
    };
    fill_style	m_current_styles[STYLE_COUNT];


    bakeinflash::bitmap_info* create_bitmap_info_rgb(image::rgb* im)
      // Given an image, returns a pointer to a bitmap_info struct
      // that can later be passed to fill_styleX_bitmap(), to set a
      // bitmap fill style.
    {
      return new bitmap_info_ogl(im);
    }


    bakeinflash::bitmap_info* create_bitmap_info_rgba(image::rgba* im)
      // Given an image, returns a pointer to a bitmap_info struct
      // that can later be passed to fill_style_bitmap(), to set a
      // bitmap fill style.
      //
      // This version takes an image with an alpha channel.
    {
      return new bitmap_info_ogl(im);
    }


    bakeinflash::bitmap_info* create_bitmap_info_empty()
      // Create a placeholder bitmap_info.  Used when
      // DO_NOT_LOAD_BITMAPS is set; then later on the host program
      // can use movie_definition::get_bitmap_info_count() and
      // movie_definition::get_bitmap_info() to stuff precomputed
      // textures into these bitmap infos.
    {
      return new bitmap_info_ogl;
    }

    bakeinflash::bitmap_info* create_bitmap_info_alpha(int w, int h, Uint8* data)
      // Create a bitmap_info so that it contains an alpha texture
      // with the given data (1 byte per texel).
      //
      // Munges *data (in order to make mipmaps)!!
    {
      return new bitmap_info_ogl(w, h, data);
    }

    bakeinflash::bitmap_info* create_bitmap_info(image::image_base* im)
      // Given an image, returns a pointer to a bitmap_info struct
      // that can later be passed to fill_styleX_bitmap(), to set a
      // bitmap fill style.
    {
      if (im)
      {
        return new bitmap_info_ogl(im);
      }
      return new bitmap_info_ogl();
    }

    bakeinflash::video_handler* create_video_handler()
    {
      return NULL; //new video_handler_ogles2();
    }

    void	begin_display(
      bakeinflash::rgba background_color,
      int viewport_x0, int viewport_y0,
      int viewport_width, int viewport_height,
      float x0, float x1, float y0, float y1)
    {
      m_display_width = fabsf(x1 - x0);
      m_display_height = fabsf(y1 - y0);

      m_display_width = fabsf(x1 - x0);
      m_display_height = fabsf(y1 - y0);

      glViewport(viewport_x0, viewport_y0, viewport_width, viewport_height);

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glClearColor(background_color.m_r / 255.0, background_color.m_g / 255.0, background_color.m_b / 255.0, background_color.m_a / 255.0);
      glClear(GL_COLOR_BUFFER_BIT);
    }

    void	end_display()
      // Clean up after rendering a frame.  Client program is still
      // responsible for calling glSwapBuffers() or whatever.
    {
    }


    void	set_matrix(const bakeinflash::matrix& m)
      // Set the current transform for mesh & line-strip rendering.
    {
      m_current_matrix = m;
    }


    void	set_cxform(const bakeinflash::cxform& cx)
      // Set the current color transform for mesh & line-strip rendering.
    {
      m_current_cxform = cx;
    }

    void	set_rgba(bakeinflash::rgba* color)
    {
      m_current_rgba = color;
    }

    static void	apply_matrix(const bakeinflash::matrix& m)
      // multiply current matrix with opengl matrix
    {
      float	mat[16];
      memset(&mat[0], 0, sizeof(mat));
      mat[0] = m.m_[0][0];
      mat[1] = m.m_[1][0];
      mat[4] = m.m_[0][1];
      mat[5] = m.m_[1][1];
      mat[10] = 1;
      mat[12] = m.m_[0][2];
      mat[13] = m.m_[1][2];
      mat[15] = 1;
      //vv	glMultMatrixf(mat);
    }

    static void	apply_color(const bakeinflash::rgba& c)
      // Set the given color.
    {
      //vv 	glColor4f(c.m_r / 255.0, c.m_g / 255.0, c.m_b / 255.0, c.m_a / 255.0);
    }

    void	fill_style_disable(int fill_side)
      // Don't fill on the {0 == left, 1 == right} side of a path.
    {
      assert(fill_side >= 0 && fill_side < 2);
      m_current_styles[fill_side].disable();
    }


    void	line_style_disable()
      // Don't draw a line on this path.
    {
      m_current_styles[LINE_STYLE].disable();
    }

    void	line_style_caps(int caps_style)
    {
      m_current_styles[LINE_STYLE].m_caps_style = caps_style;
    }

    void	fill_style_color(int fill_side, const bakeinflash::rgba& color)
      // Set fill style for the left interior of the shape.  If
      // enable is false, turn off fill for the left interior.
    {
      assert(fill_side >= 0 && fill_side < 2);
      m_current_styles[fill_side].set_color(m_current_cxform.transform(color));
    }


    void	line_style_color(bakeinflash::rgba color)
      // Set the line style of the shape.  If enable is false, turn
      // off lines for following curve segments.
    {
      m_current_styles[LINE_STYLE].set_color(m_current_cxform.transform(color));
    }


    void	fill_style_bitmap(int fill_side, bakeinflash::bitmap_info* bi, const bakeinflash::matrix& m,
      bitmap_wrap_mode wm, bitmap_blend_mode bm)
    {
      assert(fill_side >= 0 && fill_side < 2);
      m_current_styles[fill_side].set_bitmap(bi, m, wm, m_current_cxform);
    }

    void	line_style_width(float width)
    {
      m_current_styles[LINE_STYLE].m_width = width;
    }

    void testgl()
    {
      switch (glGetError())
      {
      case GL_NO_ERROR:
        return;
      case GL_INVALID_ENUM:
        printf("GL_INVALID_ENUM\n");
        break;
      case GL_INVALID_VALUE:
        printf("GL_INVALID_VALUE\n");
        break;
      case GL_INVALID_OPERATION:
        //printf("GL_INVALID_OPERATION\n");
        return;
        break;
      default:
        printf("GL_INVALID ERROR CODE\n");
        break;
      }
      assert(0);
    }

    void	draw_mesh_primitive(int primitive_type, const void* coords, int vertex_count)
      // Helper for draw_mesh_strip and draw_triangle_list.
    {
      bakeinflash::matrix mat;
      mat.set_identity();
      mat.fliph();
      mat.concatenate_translation(-1, -1);
      mat.concatenate_xyscale(2 / m_display_width, 2 / m_display_height);	// 2 = [-1..1], opengl view size
      mat.concatenate(m_current_matrix);

      glUseProgram(m_prg);
      glUniformMatrix4fv(m_modelViewProjectionMatrix, 1, 0, mat.oglmatrix());

      m_current_styles[LEFT_STYLE].apply(m_modelColor, m_modelTexture);

      glEnableVertexAttribArray(m_position);
      glVertexAttribPointer(m_position, 2, GL_FLOAT, GL_FALSE, 0, 0);

      float* tcoord = m_current_styles[LEFT_STYLE].gentexcoords(primitive_type, coords, vertex_count);
      if (tcoord)
      {
        static float zz[1000 * 4];    // hack
        float* ss = (float*)coords;
        for (int i = 0; i < imin(vertex_count, 1000); i++)
        {
          zz[4 * i] = ss[2 * i];
          zz[4 * i + 1] = ss[2 * i + 1];
          zz[4 * i + 2] = tcoord[2 * i];
          zz[4 * i + 3] = tcoord[2 * i + 1];
        }

        glUniform2f(m_modelMode, 1, 0);
        glEnableVertexAttribArray(m_position);
        glVertexAttribPointer(m_position, 2, GL_FLOAT, GL_FALSE, 16, 0);
        glEnableVertexAttribArray(m_texcoords);
        glVertexAttribPointer(m_texcoords, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(8));
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_count * 4, zz, GL_STATIC_DRAW);
      }
      else
      {
        glUniform2f(m_modelMode, 0, 0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_count * 2, coords, GL_STATIC_DRAW);
      }

      glDrawArrays(primitive_type, 0, vertex_count);

      if (tcoord)
        delete[] tcoord;

      glDisableVertexAttribArray(m_texcoords);
      glDisableVertexAttribArray(m_position);
    }

    void draw_mesh_strip(const void* coords, int vertex_count)
    {
      draw_mesh_primitive(GL_TRIANGLE_STRIP, coords, vertex_count);
    }

    void	draw_triangle_list(const void* coords, int vertex_count)
    {
      draw_mesh_primitive(GL_TRIANGLES, coords, vertex_count);
    }


    void	draw_line_strip(const void* coords, int vertex_count)
      // Draw the line strip formed by the sequence of points.
    {
      return;
      assert(0);
      /*
       // Set up current style.
       m_current_styles[LINE_STYLE].apply();

       // apply line width

       float scale = fabsf(m_current_matrix.get_x_scale()) + fabsf(m_current_matrix.get_y_scale());
       float w = m_current_styles[LINE_STYLE].m_width * scale / 2.0f;
       w = TWIPS_TO_PIXELS(w);

       //		GLfloat width_info[2];
       //		glGetFloatv(GL_LINE_WIDTH_RANGE, width_info);
       //		if (w > width_info[1])
       //		{
       //			printf("Your OpenGL implementation does not support the line width"
       //				" requested. Lines will be drawn with reduced width.");
       //		}

       glLineWidth(w <= 1.0f ? 1.0f : w);

       glMatrixMode(GL_MODELVIEW);
       glPushMatrix();
       apply_matrix(m_current_matrix);

       // Send the line-strip to OpenGL
       glEnableClientState(GL_VERTEX_ARRAY);
       #if TU_USES_FLOAT_AS_COORDINATE_COMPONENT
       glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, coords);
       #else
       glVertexPointer(2, GL_FLOAT, sizeof(Sint16) * 2, coords);
       #endif

       glEnable(GL_LINE_SMOOTH);
       glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
       glDrawArrays(GL_LINE_STRIP, 0, vertex_count);
       glDisable(GL_LINE_SMOOTH);

       // Draw a round dot on the beginning and end coordinates to lines.
       glPointSize(w);
       glEnable(GL_POINT_SMOOTH);
       glDrawArrays(GL_POINTS, 0, vertex_count);
       glDisable(GL_POINT_SMOOTH);
       glPointSize(1);

       glDisableClientState(GL_VERTEX_ARRAY);

       // restore defaults
       glPointSize(1);
       glLineWidth(1);

       glPopMatrix();*/
    }


    void	draw_bitmap(
      const bakeinflash::matrix& m,
      bakeinflash::bitmap_info* bi,
      const bakeinflash::rect& coords,
      const bakeinflash::rect& uv_coords,
      bakeinflash::rgba color)
      // Draw a rectangle textured with the given bitmap, with the
      // given color.	 Apply given transform; ignore any currently
      // set transforms. Intended for textured glyph rendering.
    {
      bi->upload();

      float vcoords[8];
      vcoords[0] = coords.m_x_min;
      vcoords[1] = coords.m_y_min;
      vcoords[2] = coords.m_x_max;
      vcoords[3] = coords.m_y_min;
      vcoords[4] = coords.m_x_min;
      vcoords[5] = coords.m_y_max;
      vcoords[6] = coords.m_x_max;
      vcoords[7] = coords.m_y_max;

      float tcoord[8];
      tcoord[0] = uv_coords.m_x_min;
      tcoord[1] = uv_coords.m_y_min;
      tcoord[2] = uv_coords.m_x_max;
      tcoord[3] = uv_coords.m_y_min;
      tcoord[4] = uv_coords.m_x_min;
      tcoord[5] = uv_coords.m_y_max;
      tcoord[6] = uv_coords.m_x_max;
      tcoord[7] = uv_coords.m_y_max;

      bakeinflash::matrix mat;
      mat.set_identity();
      mat.fliph();
      mat.concatenate_translation(-1, -1);
      mat.concatenate_xyscale(2 / m_display_width, 2 / m_display_height);	// 2 = [-1..1], opengl view size
      mat.concatenate(m);

      const int vertex_count = 4;
      float zz[vertex_count * 4];
      for (int i = 0; i < vertex_count; i++)
      {
        zz[4 * i] = vcoords[2 * i];
        zz[4 * i + 1] = vcoords[2 * i + 1];
        zz[4 * i + 2] = tcoord[2 * i];
        zz[4 * i + 3] = tcoord[2 * i + 1];
      }

      glUseProgram(m_prg);

      // set uniform vars
      glUniformMatrix4fv(m_modelViewProjectionMatrix, 1, 0, mat.oglmatrix());
      glUniform4f(m_modelColor, color.m_r / 255.0f, color.m_g / 255.0f, color.m_b / 255.0f, color.m_a / 255.0f);
      glUniform2f(m_modelMode, bi->get_bpp() == 1 ? 2 : 1, 0);    // alpha or rgba

      // copy coord & tcoord
      glEnableVertexAttribArray(m_position);
      glVertexAttribPointer(m_position, 2, GL_FLOAT, GL_FALSE, 16, 0);
      glEnableVertexAttribArray(m_texcoords);
      glVertexAttribPointer(m_texcoords, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(8));
      glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_count * 4, zz, GL_STATIC_DRAW);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex_count);

      glDisableVertexAttribArray(m_texcoords);
      glDisableVertexAttribArray(m_position);
    }

    bool test_stencil_buffer(const bakeinflash::rect& bound, Uint8 pattern)
    {
      return false;
    }

    void begin_submit_mask()
    {
      glEnable(GL_DEPTH_TEST); // to enable writing to the depth buffer
      glDepthMask(GL_TRUE);    // to allow writes to the depth buffer
      if (m_mask_level == 0)
      {
        //glClearDepth(1);
        glClear(GL_DEPTH_BUFFER_BIT); // for a fresh start
        glDepthFunc(GL_ALWAYS);  // to ensure everything you draw passes	
      }
      else
      {
        glDepthFunc(GL_GEQUAL);
      }
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);   // so that whatever we draw isn't actually visible
      m_mask_level++;
    }

    // called after begin_submit_mask and the drawing of mask polygons
    void end_submit_mask()
    {
      glDepthMask(GL_FALSE);    // to allow writes to the depth buffer
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDepthFunc(GL_EQUAL);
    }

    void disable_mask()
    {
      assert(m_mask_level > 0);

      if (--m_mask_level == 0)
      {
        glDisable(GL_DEPTH_TEST);
        return;
      }

      // nested mask
      end_submit_mask();
    }

    bool is_visible(const bakeinflash::rect& bound)
    {
      bakeinflash::rect viewport;
      viewport.m_x_min = 0;
      viewport.m_y_min = 0;
      viewport.m_x_max = m_display_width;
      viewport.m_y_max = m_display_height;
      return viewport.bound_test(bound);
    }

  };	// end struct render_handler_ogles2


  // bitmap_info_ogl implementation

  void	software_resample(
    int bytes_per_pixel,
    int src_width,
    int src_height,
    int src_pitch,
    uint8* src_data,
    int dst_width,
    int dst_height)
  {
    //	printf("original bitmap %dx%d, resampled bitmap %dx%d\n",
    //		src_width, src_height, dst_width, dst_height);

    assert(bytes_per_pixel == 3 || bytes_per_pixel == 4);

    //	assert(dst_width >= src_width);
    //	assert(dst_height >= src_height);

    //	unsigned int	internal_format = bytes_per_pixel == 3 ? GL_RGB : GL_RGBA;
    unsigned int	input_format = bytes_per_pixel == 3 ? GL_RGB : GL_RGBA;

    // FAST bi-linear filtering
    // the code here is designed to be fast, not readable
    Uint8* rescaled = new Uint8[dst_width * dst_height * bytes_per_pixel];
    float Uf, Vf;		// fractional parts
    float Ui, Vi;		// integral parts
    float w1, w2, w3, w4;	// weighting
    Uint8* psrc;
    Uint8* pdst = rescaled;

    // i1,i2,i3,i4 are the offsets of the surrounding 4 pixels
    const int i1 = 0;
    const int i2 = bytes_per_pixel;
    int i3 = src_pitch;
    int i4 = src_pitch + bytes_per_pixel;

    // change in source u and v
    float dv = (float)(src_height - 2) / dst_height;
    float du = (float)(src_width - 2) / dst_width;

    // source u and source v
    float U;
    float V = 0;

#define BYTE_SAMPLE(offset)	\
(Uint8) (w1 * psrc[i1 + (offset)] + w2 * psrc[i2 + (offset)] + w3 * psrc[i3 + (offset)] + w4 * psrc[i4 + (offset)])

    if (bytes_per_pixel == 3)
    {
      for (int v = 0; v < dst_height; ++v)
      {
        Vf = modff(V, &Vi);
        V += dv;
        U = 0;

        for (int u = 0; u < dst_width; ++u)
        {
          Uf = modff(U, &Ui);
          U += du;

          w1 = (1 - Uf) * (1 - Vf);
          w2 = Uf * (1 - Vf);
          w3 = (1 - Uf) * Vf;
          w4 = Uf * Vf;
          psrc = &src_data[(int)(Vi * src_pitch) + (int)(Ui * bytes_per_pixel)];

          *pdst++ = BYTE_SAMPLE(0);	// red
          *pdst++ = BYTE_SAMPLE(1);	// green
          *pdst++ = BYTE_SAMPLE(2);	// blue

          psrc += 3;
        }
      }
    }
    else
    {
      assert(bytes_per_pixel == 4);

      for (int v = 0; v < dst_height; ++v)
      {
        Vf = modff(V, &Vi);
        V += dv;
        U = 0;

        for (int u = 0; u < dst_width; ++u)
        {
          Uf = modff(U, &Ui);
          U += du;

          w1 = (1 - Uf) * (1 - Vf);
          w2 = Uf * (1 - Vf);
          w3 = (1 - Uf) * Vf;
          w4 = Uf * Vf;
          psrc = &src_data[(int)(Vi * src_pitch) + (int)(Ui * bytes_per_pixel)];

          *pdst++ = BYTE_SAMPLE(0);	// red
          *pdst++ = BYTE_SAMPLE(1);	// green
          *pdst++ = BYTE_SAMPLE(2);	// blue
          *pdst++ = BYTE_SAMPLE(3);	// alpha

          psrc += 4;
        }
      }
    }
    create_texture(input_format, dst_width, dst_height, rescaled, 0);
    delete[] rescaled;
  }

  bitmap_info_ogl::bitmap_info_ogl() :
    m_texture_id(0),
    m_width(0),
    m_height(0),
    m_suspended_image(0),
    m_bpp(0)
  {
  }

  bitmap_info_ogl::bitmap_info_ogl(int width, int height, Uint8* data) :
    m_texture_id(0),
    m_width(width),
    m_height(height),
    m_bpp(1)
  {
    assert(width > 0 && height > 0 && data);
    m_suspended_image = image::create_alpha(width, height);
    memcpy(m_suspended_image->m_data, data, m_suspended_image->m_pitch * m_suspended_image->m_height);
  }

  bitmap_info_ogl::bitmap_info_ogl(image::image_base* im) :
    m_texture_id(0),
    m_width(im->m_width),
    m_height(im->m_height),
    m_bpp(0)
  {
    m_suspended_image = im;
    if (m_suspended_image)
    {
      switch (m_suspended_image->m_type)
      {
      case image::image_base::RGB: m_bpp = 3;
      case image::image_base::RGBA: m_bpp = 4;
      case image::image_base::ALPHA: m_bpp = 1;
      };
    }
  }

  // layout image to opengl texture memory
  void bitmap_info_ogl::upload()
  {
    if (m_texture_id == 0)
    {
      assert(m_suspended_image);

      // Create the texture.
      glEnable(GL_TEXTURE_2D);
      glGenTextures(1, (GLuint*)&m_texture_id);
      glBindTexture(GL_TEXTURE_2D, m_texture_id);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

      m_width = m_suspended_image->m_width;
      m_height = m_suspended_image->m_height;

      int bpp = 4;
      int format = GL_RGBA;

      switch (m_suspended_image->m_type)
      {
      case image::image_base::RGB:
      {
        bpp = 3;
        format = GL_RGB;
      }

      case image::image_base::RGBA:
      {
        int	w = p2(m_suspended_image->m_width);
        int	h = p2(m_suspended_image->m_height);
        if (w != m_suspended_image->m_width || h != m_suspended_image->m_height)
        {
          // Faster/simpler software bilinear rescale.
          software_resample(bpp, m_suspended_image->m_width, m_suspended_image->m_height,
            m_suspended_image->m_pitch, m_suspended_image->m_data, w, h);
        }
        else
        {
          // Use original image directly.
          create_texture(format, w, h, m_suspended_image->m_data, 0);
        }
        break;
      }

      case image::image_base::ALPHA:
      {
        int	w = m_suspended_image->m_width;
        int	h = m_suspended_image->m_height;
        create_texture(GL_ALPHA, w, h, m_suspended_image->m_data, 0);
        break;
      }

      default:
        assert(0);
      }

      delete m_suspended_image;
      m_suspended_image = NULL;
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, m_texture_id);
      glEnable(GL_TEXTURE_2D);
    }
  }

}

namespace bakeinflash
{
  render_handler* create_render_handler_ogles2()
    // Factory.
  {
    return new render_handler_ogles2::render_handler_ogles2();
  }
}

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
