#include "gui.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include <cmath>
#include "../utils/utils.h"
#include <stdio.h>
#include <cmath>
#include <string>
#include "../utils/maths.h"
#include <fstream>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
	);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
		// Whenever we resize the window
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	} return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU)
			return 0;
	}break;
	case WM_DESTROY: {
		PostQuitMessage(0);
	} return 0;
	case WM_LBUTTONDOWN: {
		// Left Click
		gui::position = MAKEPOINTS(longParameter);
	} return 0;
	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{  };
			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.y <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}
	}
	}

	return DefWindowProcW(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(
	const char* windowName,
	const char* className) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	RegisterClassExA(&windowClass);

	window = CreateWindowA(
		className,
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters)); // Calling Windows ZeroMemory to make space

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

void SaveMethod() noexcept
{
	FILE* fileptr;
	//fileptr = fopen("file.txt", "w");

}
float angle = 0.0f;

static ImGuiComboFlags flags = 0;
const char* items[] = { "sin x", "cos x", "tg x", "cotg x", "sec x", "csc x" };
static int item_current_idx = 0;


float DefaultColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Always);
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"Goniometric Functions Calculator",
		&exit,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_MenuBar
	);
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				ImGui::MenuItem("Main menu bar", NULL, false, true);
				if (ImGui::MenuItem("Save", NULL, false, true)) {
					std::ofstream outputFile("export.txt");

					if (outputFile.is_open()) {
						outputFile << "\t --- Export --- \n";
						outputFile << "Angle in degrees: " << angle * (180 / 3.14159265358979323846f) << "° \n" << "Angle in radians: " << angle << " \n";
						outputFile << "sin : \n";
						outputFile << "cos : \n";
						outputFile << "tg : \n";
						outputFile << "cotg : \n";
						outputFile << "sec : \n";
						outputFile << "csc : \n";

						outputFile.close();
						//MessageBox(NULL, "Successfully exported", "Info", MB_OK | MB_ICONINFORMATION);
						MessageBox(NULL, "Successfully exported", "Info", MB_SERVICE_NOTIFICATION | MB_ICONINFORMATION); // This will display a new window message box
					}
					else {
						MessageBox(NULL, "An error occurred", "Error", MB_OK | MB_ICONERROR);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::Text("Calculator");
	ImGui::SliderAngle("Angle", &angle, 0.0f, 360.0f);
	//ImGui::ListBox("Function", &current_item, items, IM_ARRAYSIZE(items), 4);
	//ImGui::Combo("Function", &current_item, items, )

	const char* function_preview_value = items[item_current_idx];

	/*if (ImGui::BeginCombo("Function", function_preview_value, flags))
	{
		for (int n = 0; n < IM_ARRAYSIZE(items); n++)
		{
			const bool is_selected = (item_current_idx == n);
			if (ImGui::Selectable(items[n], is_selected))
				item_current_idx = n;

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}*/


	float sinResult = sin(angle);
	std::string sinStr = "sin x : " + std::to_string(sinResult);

	float cosResult = cos(angle);
	std::string cosStr = "cos x : " + std::to_string(cosResult);

	float tanResult = tan(angle);
	std::string tanStr = "tg x : " + std::to_string(tanResult);
	if (angle == 3.14159265358979323846f / 2 || angle == 1.5f * 3.14159265358979323846f) {
		tanStr = "tg x : Undefined";
	}

	float cotanResult = cos(angle) / sin(angle);
	std::string cotanStr = "cotg x : " + std::to_string(cotanResult);
	if (angle == 0 || angle == 3.14159265358979323846f  || angle == 2 * 3.14159265358979323846f) {
		cotanStr = "cotg x : Undefined";
	}

	float secResult = 1 / cos(angle);
	std::string secStr = "sec x : " + std::to_string(secResult);
	/*if (angle == 1.57079637f) {
		secStr = "sec x : Undefined";
	}
	*/
	if (angle == 3.14159265358979323846f / 2 || angle == 1.5f * 3.14159265358979323846f) {
		secStr = "sec x : Undefined";
	}

	float cscResult = 1 / sin(angle);
	std::string cscStr = "csc x : " + std::to_string(cscResult);
	if (angle == 0 || angle == 3.14159265358979323846f  || angle == 2 * 3.14159265358979323846f) {
		cscStr = "csc x : Undefined";
	}


	ImGui::Text(sinStr.c_str());
	ImGui::Text(cosStr.c_str());
	ImGui::Text(tanStr.c_str());
	ImGui::Text(cotanStr.c_str());
	ImGui::Text(secStr.c_str());
	ImGui::Text(cscStr.c_str());

	std::string RadiansString = "Radians : " + std::to_string(angle);
	ImGui::Text(RadiansString.c_str());
	if (ImGui::CollapsingHeader("Graphs")) {
		if (ImGui::TreeNode("Sinus")) {
			ImGui::SeparatorText("Sinus");
			float sinsamples[100];
			for (int n = 0; n < 100; n++)
				sinsamples[n] = sinf(n * 0.2f + ImGui::GetTime() * 1.5f);
			ImGui::PlotLines("Sine Graph", sinsamples, 100);
		}
		if (ImGui::TreeNode("Cosinus")) {
			ImGui::SeparatorText("Cosinus");
			float cossamples[100];
			for (int n = 0; n < 100; n++)
				cossamples[n] = cos(n * 0.2f + ImGui::GetTime() * 1.5f);
			ImGui::PlotLines("Cosine Graph", cossamples, 100);
		}
		if (ImGui::TreeNode("Tangens")) {
			ImGui::SeparatorText("Tangens");
			float tgsamples[100];
			for (int n = 0; n < 100; n++)
				tgsamples[n] = tan(n + ImGui::GetTime());
			ImGui::PlotLines("Tangens Graph", tgsamples, 100);
		}
		if (ImGui::TreeNode("Cotangens")) {
			ImGui::SeparatorText("Cotangens");
		}
		if (ImGui::TreeNode("Secans")) {
			ImGui::SeparatorText("Secans");
			float secsamples[100];
			for (int n = 0; n < 100; n++)
				secsamples[n] = 1 / cos(n + ImGui::GetTime());
			ImGui::PlotLines("Secans Graph", secsamples, 100);
		}
		if (ImGui::TreeNode("Cosecans")) {
			ImGui::SeparatorText("Cosecans");
		}
	}

	if (ImGui::CollapsingHeader("Configuration")) {
		ImGui::SeparatorText("Configuration");
		ImGui::Text("Coming soon...");
	}
	ImGui::Text("Made by Lukas Polacek");
	ImGui::End();
}