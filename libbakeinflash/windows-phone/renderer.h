#pragma once

#include "Direct3DBase.h"
#include "DirectXMath.h"

struct ModelViewProjectionConstantBuffer
{
	DirectX::XMFLOAT4X4 model;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

struct VertexPositionColor
{
	DirectX::XMFLOAT2 pos;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT2 texcoord;
};

// This class renders a simple spinning cube.
ref class renderer sealed : public Direct3DBase
{
public:
	renderer();

	// Direct3DBase methods.
	virtual void CreateDeviceResources() override;
	virtual void CreateWindowSizeDependentResources() override;
	virtual void Render() override;
	
	// Method for updating time-dependent objects.
	void Update(float timeTotal, float timeDelta);
	
	void onPress(float x, float y);
	void onReleased(float x, float y);
	void onMoved(float x, float y);

	Windows::UI::Core::CoreWindow^ getMainWindow() { return m_mainWindow; };
	void setMainWindow(Windows::UI::Core::CoreWindow^ w) { m_mainWindow = w; };


private:

	bool m_loadingComplete;
	Windows::UI::Core::CoreWindow^ m_mainWindow;

};