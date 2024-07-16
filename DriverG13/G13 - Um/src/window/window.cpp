#include "window.hpp"

ID3D11Device* Overlay::device = nullptr;

// sends rendering commands to the device
ID3D11DeviceContext* Overlay::device_context = nullptr;

// manages the buffers for rendering, also presents rendered frames.
IDXGISwapChain* Overlay::swap_chain = nullptr;

// represents the target surface for rendering
ID3D11RenderTargetView* Overlay::render_targetview = nullptr;

HWND Overlay::overlay = nullptr;
WNDCLASSEX Overlay::wc = { };

// declaration of the ImGui_ImplWin32_WndProcHandler function
// basically integrates ImGui with the Windows message loop so ImGui can process input and events
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// set up ImGui window procedure handler
	if (ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam))
		return true;

	// switch that disables alt application and checks for if the user tries to close the window.
	switch (msg)
	{
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu (imgui uses it in their example :shrug:)
			return 0;
		break;

	case WM_DESTROY:
		Overlay::DestroyDevice();
		Overlay::DestroyOverlay();
		Overlay::DestroyImGui();
		PostQuitMessage(0);
		return 0;

	case WM_CLOSE:
		Overlay::DestroyDevice();
		Overlay::DestroyOverlay();
		Overlay::DestroyImGui();
		return 0;
	}

	// define the window procedure
	return DefWindowProc(window, msg, wParam, lParam);
}

bool Overlay::CreateDevice()
{
	// First we setup our swap chain, this basically just holds a bunch of descriptors for the swap chain.
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));

	// set number of back buffers (this is double buffering)
	sd.BufferCount = 2;

	// width + height of buffer, (0 is automatic sizing)
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;

	// set the pixel format
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// set the fps of the buffer (60 at the moment)
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;

	// allow mode switch (changing display modes)
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// set how the bbuffer will be used
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	sd.OutputWindow = overlay;

	// setup the multi-sampling
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// specify what Direct3D feature levels to use
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

	// create device and swap chain
	HRESULT result = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		featureLevelArray,
		2,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		&featureLevel,
		&device_context);

	// if the hardware isn't supported create with WARP (basically just a different renderer)
	if (result == DXGI_ERROR_UNSUPPORTED) {
		result = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_WARP,
			nullptr,
			0U,
			featureLevelArray,
			2, D3D11_SDK_VERSION,
			&sd,
			&swap_chain,
			&device,
			&featureLevel,
			&device_context);

		printf("[>>] DXGI_ERROR | Created with D3D_DRIVER_TYPE_WARP\n");
	}

	// can't do much more, if the hardware still isn't supported just return false.
	if (result != S_OK) {
		printf("Device Not Okay\n");
		return false;
	}

	// retrieve back_buffer, im defining it here since it isn't being used at any other point in time.
	ID3D11Texture2D* back_buffer{ nullptr };
	swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

	// if back buffer is obtained then we can create render target view and release the back buffer again
	if (back_buffer)
	{
		device->CreateRenderTargetView(back_buffer, nullptr, &render_targetview);
		back_buffer->Release();

		printf("[>>] Created Device\n");
		return true;
	}

	// if we reach this point then it failed to create the back buffer
	printf("[>>] Failed to create Device\n");
	return false;
}

void Overlay::DestroyDevice()
{
	// release everything that has to do with the device.
	if (device)
	{
		device->Release();
		device_context->Release();
		swap_chain->Release();
		render_targetview->Release();

		printf("[>>] Released Device\n");
	}
	else
		printf("[>>] Device Not Found when Exiting.\n");
}

void Overlay::CreateOverlay()
{
	// holds descriptors for the window, called a WindowClass
	// set up window class
	wc.cbSize = sizeof(wc);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = window_procedure;
	wc.hInstance = GetModuleHandleA(0);
	wc.lpszClassName = "G13";

	// register our class
	RegisterClassEx(&wc);

	// create window (the actual one that shows up in your taskbar)
	// WS_EX_TOOLWINDOW hides the new window that shows up in your taskbar and attaches it to any already existing windows instead.
	// (in this case the console)
	overlay = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
		wc.lpszClassName,
		"G13",
		WS_POPUP,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN), // 1920
		GetSystemMetrics(SM_CYSCREEN), // 1080
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);

	if (overlay == NULL)
		printf("Failed to create Overlay\n");

	// set overlay window attributes to make the overlay transparent
	SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

	// set up the DWM frame extension for client area
	{
		// first we define our RECT structures that hold our client and window area
		RECT client_area{};
		RECT window_area{};

		// get the client and window area
		GetClientRect(overlay, &client_area);
		GetWindowRect(overlay, &window_area);

		// calculate the difference between the screen and window coordinates
		POINT diff{};
		ClientToScreen(overlay, &diff);

		// calculate the margins for DWM frame extension
		const MARGINS margins{
			window_area.left + (diff.x - window_area.left),
			window_area.top + (diff.y - window_area.top),
			client_area.right,
			client_area.bottom
		};

		// then we extend the frame into the client area
		DwmExtendFrameIntoClientArea(overlay, &margins);
	}

	// show + update overlay
	ShowWindow(overlay, SW_SHOW);
	UpdateWindow(overlay);

	printf("[>>] Overlay Created\n");
}
//ID3D11ShaderResourceView* myTexture = nullptr;
//ID3D11ShaderResourceView* aimbotpic = nullptr;
//ID3D11ShaderResourceView* esppic = nullptr;
//ID3D11ShaderResourceView* mispic = nullptr;
//ID3D11ShaderResourceView* stpic = nullptr;
//ID3D11ShaderResourceView* colopic = nullptr;
//ID3D11ShaderResourceView* setpic = nullptr;
void Overlay::DestroyOverlay()
{
	DestroyWindow(overlay);
	UnregisterClass(wc.lpszClassName, wc.hInstance);
}
HRESULT LoadTexture(ID3D11Device* device, const wchar_t* filePath, ID3D11ShaderResourceView** outTexture) {
	return DirectX::CreateWICTextureFromFile(device, filePath, nullptr, outTexture);
}
void SetTextureFiltering(ID3D11Device* device, ID3D11ShaderResourceView* texture) {
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* samplerState;
	device->CreateSamplerState(&sampDesc, &samplerState);

	ID3D11DeviceContext* context;
	device->GetImmediateContext(&context);
	context->PSSetSamplers(0, 1, &samplerState);
	samplerState->Release();
}
bool Overlay::CreateImGui()
{
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// Initalize ImGui for the Win32 library
	if (!ImGui_ImplWin32_Init(overlay)) {
		printf("Failed ImGui_ImplWin32_Init\n");
		return false;
	}

	// Initalize ImGui for DirectX 11.
	if (!ImGui_ImplDX11_Init(device, device_context)) {
		printf("Failed ImGui_ImplDX11_Init\n");
		return false;
	}
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());;
	io.Fonts->Build();
	io.IniFilename = nullptr;

	//HRESULT result = LoadTexture(Overlay::device, L"C:\\Users\\yuval\\source\\repos\\DriverG13\\G13 - Um\\ext\\image.png", &myTexture);
	//SetTextureFiltering(Overlay::device, myTexture);
//	HRESULT result1 = LoadTexture(Overlay::device, L"C:\\Users\\yuval\\source\\repos\\DriverG13\\G13 - Um\\ext\\ss.png", &aimbotpic);
	//SetTextureFiltering(Overlay::device, aimbotpic);
	//HRESULT result2 = LoadTexture(Overlay::device, L"C:\\Users\\yuval\\source\\repos\\DriverG13\\G13 - Um\\ext\\ee.png", &esppic);
	//SetTextureFiltering(Overlay::device, esppic);
	//HRESULT result3 = LoadTexture(Overlay::device, L"C:\\Users\\yuval\\source\\repos\\DriverG13\\G13 - Um\\ext\\mm.png", &mispic);
	//SetTextureFiltering(Overlay::device, mispic);
	//HRESULT result4 = LoadTexture(Overlay::device, L"C:\\Users\\yuval\\source\\repos\\DriverG13\\G13 - Um\\ext\\c.png", &colopic);
//	SetTextureFiltering(Overlay::device, colopic);
//	HRESULT result5 = LoadTexture(Overlay::device, L"C:\\Users\\yuval\\source\\repos\\DriverG13\\G13 - Um\\ext\\set.png", &setpic);
//	SetTextureFiltering(Overlay::device, setpic);
	printf("[>>] ImGui Initialized\n");
	return true;
}

void Overlay::DestroyImGui()
{
	// Cleanup ImGui by shutting down DirectX11, the Win32 Platform and Destroying the ImGui context.
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Overlay::StartRender()
{
	// handle windows messages
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// begin a new frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// if the user presses Insert then enable the menu.
	if (GetAsyncKeyState(VK_INSERT) & 1 ||GetAsyncKeyState(VK_OEM_PLUS) & 1) {
		RenderMenu = !RenderMenu;

		// If we are rendering the menu set the window styles to be able to clicked on.
		if (RenderMenu) {
			SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
		}
		else {
			SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED);
		}
	}
}

void Overlay::EndRender()
{
	// Render ImGui
	ImGui::Render();

	// Make a color that's clear / transparent
	float color[4]{ 0, 0, 0, 0 };

	// Set the render target and then clear it
	device_context->OMSetRenderTargets(1, &render_targetview, nullptr);
	device_context->ClearRenderTargetView(render_targetview, color);

	// Render ImGui draw data.
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present rendered frame with V-Sync
	swap_chain->Present(1U, 0U);

	// Present rendered frame without V-Sync
	//swap_chain->Present(0U, 0U);
}



void Overlay::Render()
{

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.30f, 0.95f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.23f, 0.95f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.23f, 0.95f);
	colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 0.67f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.40f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.31f, 0.31f, 0.67f, 0.60f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.40f, 0.40f, 0.80f, 0.80f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.30f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.35f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.30f, 0.75f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.30f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.25f, 0.54f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.64f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.80f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.90f, 0.54f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.74f, 0.54f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.96f, 0.54f);
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.50f, 0.54f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.31f, 0.31f, 0.67f, 0.54f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.40f, 0.80f, 0.54f);
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.50f, 0.54f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.31f, 0.31f, 0.67f, 0.40f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.80f, 0.67f);
	colors[ImGuiCol_Separator] = ImVec4(0.31f, 0.31f, 0.67f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.80f, 0.29f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.90f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.50f, 0.29f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.70f, 0.29f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.80f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.31f, 0.31f, 0.67f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.IndentSpacing = 25;
	style.ScrollbarSize = 15;
	style.GrabMinSize = 10;
	style.WindowBorderSize = 1;
	style.ChildBorderSize = 1;
	style.PopupBorderSize = 1;
	style.WindowRounding = 6;
	style.ChildRounding = 6;
	style.FrameRounding = 4;
	style.PopupRounding = 6;
	style.ScrollbarRounding = 12;
	style.GrabRounding = 4;
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

	static int tabb;
	ImGui::SetNextWindowSize(ImVec2(700, 570), ImGuiCond_Once);
	ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
	auto size = ImVec2(80, 80);

	ImGui::BeginChild("##Sidebar", ImVec2(100, 0), true);
	ImGui::Separator();
	if (ImGui::Button("Aimbot", size)) tabb = 1;
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Toggle aimbot features");
	if (ImGui::Button("ESP", size)) tabb = 2;
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Toggle ESP features");
	if (ImGui::Button("Misc", size)) tabb = 3;
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Miscellaneous features");
	if (ImGui::Button("colors", size)) tabb = 4;
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Customize colors");
	if (ImGui::Button("settings", size)) tabb = 5;
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("General settings");
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("Content", ImVec2(0, 0), false);


	switch (tabb)
	{
	case 1:
		ImGui::Text("Aimbot Settings");
		if (ImGui::CollapsingHeader("Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Enable Aimbot", &var::AimBot);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable aimbot");
			if (var::AimBot) {
				ImGui::SliderFloat("Fov", &var::fov, 0.5, 100);

			}
		}
		if (ImGui::CollapsingHeader("Triggerbot", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Enable Triggerbot", &var::trigger_bot);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable triggerbot");
			ImGui::SliderInt("Trigger Delay (ms)", &var::trigger_delay, 0, 500);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Set the delay for triggerbot in milliseconds");
		}
		break;

	case 2:
		ImGui::Text("ESP Settings");
		if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Draw Players", &var::drawEsp);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable drawing players");
			if (var::drawEsp)
			{
				ImGui::Checkbox("Corner ESP", &var::corneresp);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Enable or disable corner ESP");
				ImGui::Checkbox("Box ESP", &var::enemyboxCheck);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Enable or disable box ESP");
				ImGui::Checkbox("Draw Team", &var::drawEspTeam);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Enable or disable drawing team members");
				ImGui::Checkbox("Visibility Check", &var::visibility);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Enable or disable visibility check");
				ImGui::Checkbox("Name ESP", &var::EspPlayerName);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Enable or disable name ESP");
				ImGui::Checkbox("Health Bar ESP", &var::HealthEsp);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Enable or disable health bar ESP");
				ImGui::Checkbox("Cash ESP", &var::cashEsp);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Enable or disable cash ESP");
				ImGui::Checkbox("Distance ESP", &var::DistanceEsp);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Enable or disable distance ESP");
				ImGui::SliderFloat("Box Thickness", &var::boxthick, 0.5f, 5.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Set the thickness of the box ESP");
			}
		}
		if (ImGui::CollapsingHeader("Skeleton", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Enable Skeleton", &var::boneEsp);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable skeleton ESP");
			if (var::boneEsp)
			{
				ImGui::SliderFloat("Skeleton Thickness", &var::bonethickness, 0.5f, 5.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Set the thickness of the skeleton ESP");
			}
		}
		if (ImGui::CollapsingHeader("Snaplines", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Enable Snaplines", &var::snapline);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable snaplines");
			if (var::snapline)
			{
				ImGui::SliderInt("Snapline Method", &var::snaplineMethod, 1, 3);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Select the method for snaplines");
			}
		}
		break;

	case 3:
		ImGui::Text("Miscellaneous Settings");
		ImGui::Checkbox("Crosshair", &var::crosshair1);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Enable or disable crosshair");
		if (var::crosshair1)
		{
			ImGui::Checkbox("Crosshair Circle", &var::crosshair2);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable crosshair circle");
			if (var::crosshair2)
			{
				var::crosshair3 = false;
				ImGui::SliderFloat("Circle Size", &var::crosshairSize, 0.5f, 8.0f, "%.3f");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Set the size of the crosshair circle");
			}
			ImGui::Checkbox("Crosshair Cross", &var::crosshair3);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable crosshair cross");
			if (var::crosshair3)
			{
				var::crosshair2 = false;
				ImGui::SliderFloat("Cross Size", &var::crosshairSize1, 0.5f, 10.0f, "%.3f");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Set the size of the crosshair cross");
				ImGui::SliderFloat("Cross Thickness", &var::crosshairThick, 0.1f, 5.0f, "%.3f");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Set the thickness of the crosshair cross");
			}
		}
		break;

	case 4:
		ImGui::Text("Color Settings");
		if (ImGui::CollapsingHeader("Player Colors", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::ColorEdit4("Enemy Box Color", var::enemybox, ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Set the color for enemy box ESP");
			ImGui::ColorEdit4("Team Box Color", var::Teambox, ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Set the color for team box ESP");
			ImGui::ColorEdit4("Enemy Skeleton Color", var::boneColorEn, ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Set the color for enemy skeleton ESP");
			ImGui::ColorEdit4("Team Skeleton Color", var::boneColorTeam, ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Set the color for team skeleton ESP");
			ImGui::Checkbox("Enemy Rainbow", &var::BoxRainbow);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable rainbow effect for enemy box ESP");
			ImGui::Checkbox("Team Rainbow", &var::BoxRainbowTeam);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable rainbow effect for team box ESP");
		}
		if (ImGui::CollapsingHeader("Crosshair Colors", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::ColorEdit4("Crosshair Color", var::crosshairColor, ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Set the color for crosshair");
			ImGui::Checkbox("Crosshair Rainbow", &var::crossrainbowcolor);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enable or disable rainbow effect for crosshair");
		}
		break;

	case 5:
		ImGui::Text("General Settings");
		ImGui::Checkbox("Watermark", &var::watermark);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Enable or disable watermark");
		break;
	}

	ImGui::EndChild();

	ImGui::End();
}



void Overlay::SetForeground(HWND window)
{
	if (!IsWindowInForeground(window))
		(window);
}