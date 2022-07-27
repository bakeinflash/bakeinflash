//#include <vld.h> 

#include "pch.h"
#include "winphone.h"
#include "BasicTimer.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

renderer^ m_renderer;
bool s_has_virtual_keyboard = true;

winphone::winphone() :
	m_windowClosed(false),
	m_windowVisible(true)
{
	DisplayOrientations o = DisplayProperties::CurrentOrientation;
}

void winphone::Initialize(CoreApplicationView^ applicationView)
{
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &winphone::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &winphone::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &winphone::OnResuming);

	m_renderer = ref new renderer();
}

void winphone::OnOrientationChanged(Platform::Object^ sender)
{
  m_renderer->UpdateForWindowSizeChange();
}

void winphone::SetWindow(CoreWindow^ window)
{
	window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &winphone::OnVisibilityChanged);
	window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &winphone::OnWindowClosed);
	window->PointerPressed +=	ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &winphone::OnPointerPressed);
	window->PointerMoved +=	ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &winphone::OnPointerMoved);
	window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &winphone::OnPointerReleased);

  DisplayProperties::AutoRotationPreferences =
		Windows::Graphics::Display::DisplayOrientations::Landscape |
    Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped;
//    Windows::Graphics::Display::DisplayOrientations::Portrait;

	DisplayProperties::OrientationChanged += ref new DisplayPropertiesEventHandler(this, &winphone::OnOrientationChanged);
 
	DisplayProperties::AutoRotationPreferences = Windows::Graphics::Display::DisplayOrientations::Landscape;

	m_renderer->Initialize(CoreWindow::GetForCurrentThread());
	m_renderer->setMainWindow(window);

}

void winphone::Load(Platform::String^ entryPoint)
{
}

void winphone::Run()
{
	BasicTimer^ timer = ref new BasicTimer();

	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			timer->Update();
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			m_renderer->Update(timer->Total, timer->Delta);
			m_renderer->Render();
			m_renderer->Present(); // This call is synchronized to the display frame rate.
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

void winphone::Uninitialize()
{
}

void winphone::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void winphone::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

void winphone::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
	// get the current pointer position
	float x = args->CurrentPoint->Position.X;
	float y = args->CurrentPoint->Position.Y;
	m_renderer->onPress(x, y);
}

void winphone::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
	// get the current pointer position
	float x = args->CurrentPoint->Position.X;
	float y = args->CurrentPoint->Position.Y;
	m_renderer->onMoved(x, y);
}

void winphone::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
	// get the current pointer position
	float x = args->CurrentPoint->Position.X;
	float y = args->CurrentPoint->Position.Y;
	m_renderer->onReleased(x, y);
}

void winphone::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	CoreWindow::GetForCurrentThread()->Activate();
}

void winphone::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();
	m_renderer->ReleaseResourcesForSuspending();

	create_task([this, deferral]()
	{
		// Insert your code here.

		deferral->Complete();
	});
}
 
void winphone::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.
	 m_renderer->CreateWindowSizeDependentResources();
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
	return ref new winphone();
}

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto direct3DApplicationSource = ref new Direct3DApplicationSource();
	CoreApplication::Run(direct3DApplicationSource);
	return 0;
}