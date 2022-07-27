//#include <vld.h>

#include "pch.h"
#include "renderer.h"

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
#include "bakeinflash/bakeinflash_as_classes/as_broadcaster.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace bakeinflash;

Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShaderTex;
Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;

// Cached renderer properties.
extern D3D_FEATURE_LEVEL m_featureLevel;
extern Windows::Foundation::Size m_renderTargetSize;
extern Windows::Foundation::Rect m_windowBounds;
extern Platform::Agile<Windows::UI::Core::CoreWindow> m_window;

// Direct3D Objects.
extern Microsoft::WRL::ComPtr<ID3D11Device1> m_d3dDevice;
extern Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_d3dContext;
extern Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
extern Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
extern Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;

	extern float s_viewport_width;
	extern float s_viewport_height;
	extern float s_x0;
	extern float s_y0;
	extern float s_scale;

// by default it's used the simplest and the fastest edge antialiasing method
// if you have modern video card you can use full screen antialiasing
// full screen antialiasing level may be 2,4,8,16, ...
// static int s_aa_level = 4;	// for windows
static int s_aa_level = 0;

static bool s_background = true;
static smart_ptr<root>	s_root = NULL;


static void	log_callback(bool error, const char* message)
	// Error callback for handling bakeinflash messages.
{
	if (strlen(message) < 2048)
	{
		wchar_t buffer[4096];
		auto count = swprintf_s(buffer, L"%S", message);
		OutputDebugString(buffer);
	}
}

static tu_file*	file_opener(const char* url)
	// Callback function.  This opens files for the bakeinflash library.
{
	tu_string filename;

	// absolute path ?
	if (strlen(url) > 1 && url[1] != ':')
	{
		filename = get_workdir();
	}
	filename += url;
	return new tu_file(filename.c_str(), "rb");
}

static void	fs_callback(character* movie, const char* command, const char* args)
	// For handling notification callbacks from ActionScript.
{
	assert(movie);
}


// Mouse state.
int	mouse_x = 0;
int	mouse_y = 0;
int	mouse_buttons = 0;

Uint32	start_ticks = 0;
Uint32	last_ticks = 0;

renderer::renderer() :
	m_loadingComplete(false)
{
}


void renderer::CreateDeviceResources()
{
	Direct3DBase::CreateDeviceResources();

	auto loadVSTask = DX::ReadDataAsync("SimpleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync("SimplePixelShader.cso");
	auto loadPSTaskTex = DX::ReadDataAsync("SimplePixelShaderTex.cso");

	auto createVSTask = loadVSTask.then([this](Platform::Array<byte>^ fileData)
	{
		DX::ThrowIfFailed(m_d3dDevice->CreateVertexShader(fileData->Data,	fileData->Length,	nullptr, &m_vertexShader));

		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
		};

		DX::ThrowIfFailed(m_d3dDevice->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), fileData->Data, fileData->Length,	&m_inputLayout));
	});

	auto createPSTaskTex = loadPSTaskTex.then([this](Platform::Array<byte>^ fileData)
	{
		DX::ThrowIfFailed(m_d3dDevice->CreatePixelShader(fileData->Data, fileData->Length, nullptr, &m_pixelShaderTex));

	//	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	//	DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr,	&m_constantBuffer));
	});

	auto createPSTask = loadPSTask.then([this](Platform::Array<byte>^ fileData)
	{
		DX::ThrowIfFailed(m_d3dDevice->CreatePixelShader(fileData->Data, fileData->Length, nullptr, &m_pixelShader));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr,	&m_constantBuffer));
	});

	createPSTask.then([this] ()
	{
		m_loadingComplete = true;
	});
}

void renderer::CreateWindowSizeDependentResources()
{
	Direct3DBase::CreateWindowSizeDependentResources();

	// first time &
	if (s_root == NULL)
	{
		tu_string flash_vars;

		bakeinflash::preload_bitmaps(false);
		//bakeinflash::keep_alive_bitmaps(true);
		bakeinflash::keep_alive_bitmaps(false);
		bakeinflash::set_display_invisibles(false);
		bakeinflash::set_yuv2rgb_table(true);

		//		float	exit_timeout = 0;
		bool	do_render = true;
		bool	do_sound = true;
		//		bool	do_loop = true;
		bool	sdl_abort = true;
		float	tex_lod_bias;
		//		bool	force_realtime_framerate = false;

	#ifndef WINPHONE
		WSADATA wsaData;

		int iResult = WSAStartup( MAKEWORD(2,2), &wsaData);
		if ( iResult != NO_ERROR )
			printf("Error at WSAStartup()\n");
	#endif

		// -1.0 tends to look good.
		tex_lod_bias = 0; //-1.2f;

		int	width = 0;
		int	height = 0;

		init_player();

		// use this for multifile games
		// workdir is used when LoadMovie("myfile.swf", _root) is called


		std::wstring wpath = Windows::ApplicationModel::Package::Current->InstalledLocation->Path->Data();
		std::string str(wpath.begin(), wpath.end());
		set_workdir(str.c_str());
		tu_string workdir = get_workdir();
		workdir += "\\Assets";
		workdir += "\\";
		set_workdir(workdir.c_str());

		set_startdir(workdir.c_str());

		tu_string swfile = "casino.swf";
		//tu_string swfile = "test.swf";

		register_file_opener_callback(file_opener);
		register_fscommand_callback(fs_callback);
		register_log_callback(log_callback);

		sound_handler*	sound = create_sound_handler_xaudio2();
		set_sound_handler(sound);

		bakeinflash::render_handler*	s_render = create_render_handler_d3d();
		set_render_handler(s_render);

		set_glyph_provider(create_glyph_provider_freetype());
		//set_glyph_provider(create_glyph_provider_tu());

		// decrease memory usage
		use_cached_movie(false);

		set_flash_vars(flash_vars);

		s_root = load_file(swfile.c_str());
		if (s_root == NULL)
		{
			myprintf("could not load %s\n", swfile.c_str());
			assert(0);
			delete sound;
			delete s_render;
		}

		// move to center
		int w = s_root->get_movie_width();
		int h = s_root->get_movie_height();
		float viewport_scale_x = m_windowBounds.Width / w;
		float viewport_scale_y = m_windowBounds.Height / h;
		s_scale = viewport_scale_x < viewport_scale_y ? viewport_scale_x : viewport_scale_y;
		s_x0 = m_windowBounds.Width - s_scale * w;
		s_x0 /= 2;
		s_y0 = m_windowBounds.Height - s_scale * h;
		s_y0 /= 2;

		// Set the rendering viewport 
		s_viewport_width = w * s_scale;
		s_viewport_height = h * s_scale;


		s_render->open();
		s_render->set_antialiased(false);

		start_ticks = tu_timer::get_ticks();
		last_ticks = start_ticks;
	}

}

void renderer::Render()
{

	// Only draw the cube once it is loaded (loading is asynchronous).
	if (!m_loadingComplete)
	{
		return;
	}

	Uint32	ticks = tu_timer::get_ticks();
	int	delta_ticks = ticks - last_ticks;
	float	delta_t = delta_ticks / 1000.f;

	smart_ptr<root>	m = get_root();
	//	m->set_display_viewport(0, 0, width, height);
	//m->set_background_alpha(s_background ? 1.0f : 0.05f);

	Uint32 t_advance = tu_timer::get_ticks();
	m->advance(delta_t);
	t_advance = tu_timer::get_ticks() - t_advance;

	Uint32 t_display = tu_timer::get_ticks();
	m->display();
	t_display = tu_timer::get_ticks() - t_display;

	// for perfomance testing
	//printf("advance time: %d, display time %d\n",	t_advance, t_display);
	last_ticks = ticks;
}


void renderer::Update(float timeTotal, float timeDelta)
{
}

void renderer::onPress(float x, float y)
{
	x -= s_x0;
	y -= s_y0;
	x /= s_scale;
	y /= s_scale;

	// move to
	get_root()->notify_mouse_state((int) x, (int) y, 0);
	Render();

	// notify press
	get_root()->notify_mouse_state((int) x, (int) y, 1);
	Render();
}

void renderer::onReleased(float x, float y)
{
	x -= s_x0;
	y -= s_y0;
	x /= s_scale;
	y /= s_scale;
//	log("onrelease %d %d\n", x, y);

	// move to
	get_root()->notify_mouse_state((int) x, (int) y, 1);
	Render();

	// notify release
	get_root()->notify_mouse_state((int) x, (int) y, 0);
	Render();
}

void renderer::onMoved(float x, float y)
{
	x -= s_x0;
	y -= s_y0;
	x /= s_scale;
	y /= s_scale;
	//log("onmove %d %d\n", x, y);

	get_root()->notify_mouse_state((int) x, (int) y, 1);
	Render();
}
