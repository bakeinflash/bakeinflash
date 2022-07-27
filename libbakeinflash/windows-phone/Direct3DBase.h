#pragma once

#include "pch.h"
#include "DirectXHelper.h"

using namespace DirectX;

// Helper class that initializes DirectX APIs for 3D rendering.
ref class Direct3DBase abstract
{
internal:
	Direct3DBase();

public:
	virtual void Initialize(Windows::UI::Core::CoreWindow^ window);
	virtual void HandleDeviceLost();
	virtual void CreateDeviceResources();
	virtual void CreateWindowSizeDependentResources();
	virtual void UpdateForWindowSizeChange();
	virtual void ReleaseResourcesForSuspending();
	virtual void Render() = 0;
	virtual void Present();
	virtual float ConvertDipsToPixels(float dips);

private:

	void ComputeOrientationMatrices();

	// Store the current Orientation of the device
	Windows::Graphics::Display::DisplayOrientations m_orientation;

	// Size of the virtual, oriented screen
	Windows::Foundation::Size m_orientedScreenSize;

	// Transforms used for rendering 2D and 3D elements in proper orientation
	DirectX::XMMATRIX m_orientationTransform3D;
	DirectX::XMMATRIX m_orientationTransform2D;

};