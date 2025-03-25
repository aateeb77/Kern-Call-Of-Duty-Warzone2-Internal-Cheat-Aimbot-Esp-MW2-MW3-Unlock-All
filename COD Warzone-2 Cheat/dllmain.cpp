#include "stdafx.h"
#include <cstdint>
#include "tahoma.ttf.h"
#include "imgui/Kiero/kiero.h"
#include "imgui_draw.h"
#include "game.h"
#include "sdk.h"
#include "Menu.h"

typedef long(__fastcall* Present)(IDXGISwapChain*, UINT, UINT);
static Present ori_present = NULL;
void WndProc_hk();

typedef LRESULT(CALLBACK* tWndProc)(HWND hWnd, UINT Msg, WPARAM wp, LPARAM lp);
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace d3d12
{
	
	IDXGISwapChain3* pSwapChain;
	ID3D12Device* pDevice;
	ID3D12CommandQueue* pCommandQueue;
	ID3D12Fence* pFence;
	ID3D12DescriptorHeap* d3d12DescriptorHeapBackBuffers = nullptr;
	ID3D12DescriptorHeap* d3d12DescriptorHeapImGuiRender = nullptr;
	ID3D12DescriptorHeap* pSrvDescHeap = nullptr;;
	ID3D12DescriptorHeap* pRtvDescHeap = nullptr;;
	ID3D12GraphicsCommandList* pCommandList;

	
	FrameContext* FrameContextArray;
	ID3D12Resource** pID3D12ResourceArray;
	D3D12_CPU_DESCRIPTOR_HANDLE* RenderTargetDescriptorArray;

	
	HANDLE hSwapChainWaitableObject;
	HANDLE hFenceEvent;

	
	UINT NUM_FRAMES_IN_FLIGHT;
	UINT NUM_BACK_BUFFERS;

	
	UINT   frame_index = 0;
	UINT64 fenceLastSignaledValue = 0;
}

namespace imgui
{
	bool is_ready;
	bool is_need_reset_imgui;

	bool IsReady()
	{
		return is_ready;
	}

	void reset_imgui_request()
	{
		is_need_reset_imgui = true;
	}

	__forceinline bool get_is_need_reset_imgui()
	{
		return is_need_reset_imgui;
	}

	void init_d3d12(IDXGISwapChain3* pSwapChain, ID3D12CommandQueue* pCommandQueue)
	{

		d3d12::pSwapChain = pSwapChain;
		d3d12::pCommandQueue = pCommandQueue;

		if (!SUCCEEDED(d3d12::pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&d3d12::pDevice)))
			Exit();

		{
			DXGI_SWAP_CHAIN_DESC1 desc;

			if (!SUCCEEDED(d3d12::pSwapChain->GetDesc1(&desc)))
				Exit();

			d3d12::NUM_BACK_BUFFERS = desc.BufferCount;
			d3d12::NUM_FRAMES_IN_FLIGHT = desc.BufferCount;
		}

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc;
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = d3d12::NUM_BACK_BUFFERS;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			desc.NodeMask = 1;

			if (!SUCCEEDED(d3d12::pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&d3d12::pRtvDescHeap))))
				Exit();
		}

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc;
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = 1;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			desc.NodeMask = 0;

			if (!SUCCEEDED(d3d12::pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&d3d12::pSrvDescHeap))))
				Exit();
		}

		if (!SUCCEEDED(d3d12::pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d12::pFence))))
			Exit();

		d3d12::FrameContextArray = new FrameContext[d3d12::NUM_FRAMES_IN_FLIGHT];
		d3d12::pID3D12ResourceArray = new ID3D12Resource * [d3d12::NUM_BACK_BUFFERS];
		d3d12::RenderTargetDescriptorArray = new D3D12_CPU_DESCRIPTOR_HANDLE[d3d12::NUM_BACK_BUFFERS];

		for (UINT i = 0; i < d3d12::NUM_FRAMES_IN_FLIGHT; ++i)
		{
			if (!SUCCEEDED(d3d12::pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d12::FrameContextArray[i].CommandAllocator))))
				Exit();
		}

		SIZE_T nDescriptorSize = d3d12::pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d12::pRtvDescHeap->GetCPUDescriptorHandleForHeapStart();

		for (UINT i = 0; i < d3d12::NUM_BACK_BUFFERS; ++i)
		{
			d3d12::RenderTargetDescriptorArray[i] = rtvHandle;
			rtvHandle.ptr += nDescriptorSize;
		}


		if (!SUCCEEDED(d3d12::pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12::FrameContextArray[0].CommandAllocator, NULL, IID_PPV_ARGS(&d3d12::pCommandList))) ||
			!SUCCEEDED(d3d12::pCommandList->Close()))
		{
			Exit();
		}


		d3d12::hSwapChainWaitableObject = d3d12::pSwapChain->GetFrameLatencyWaitableObject();

		d3d12::hFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		if (d3d12::hFenceEvent == NULL)
			Exit();


		ID3D12Resource* pBackBuffer;
		for (UINT i = 0; i < d3d12::NUM_BACK_BUFFERS; ++i)
		{
			if (!SUCCEEDED(d3d12::pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer))))
				Exit();

			d3d12::pDevice->CreateRenderTargetView(pBackBuffer, NULL, d3d12::RenderTargetDescriptorArray[i]);
			d3d12::pID3D12ResourceArray[i] = pBackBuffer;
		}
	}

	void _clear()
	{
		d3d12::pSwapChain = nullptr;
		d3d12::pDevice = nullptr;
		d3d12::pCommandQueue = nullptr;

		if (d3d12::pFence)
		{
			d3d12::pFence->Release();
			d3d12::pFence = nullptr;
		}

		if (d3d12::pSrvDescHeap)
		{
			d3d12::pSrvDescHeap->Release();
			d3d12::pSrvDescHeap = nullptr;
		}

		if (d3d12::pRtvDescHeap)
		{
			d3d12::pRtvDescHeap->Release();
			d3d12::pRtvDescHeap = nullptr;
		}

		if (d3d12::pCommandList)
		{
			d3d12::pCommandList->Release();
			d3d12::pCommandList = nullptr;
		}

		if (d3d12::FrameContextArray)
		{
			for (UINT i = 0; i < d3d12::NUM_FRAMES_IN_FLIGHT; ++i)
			{
				if (d3d12::FrameContextArray[i].CommandAllocator)
				{
					d3d12::FrameContextArray[i].CommandAllocator->Release();
					d3d12::FrameContextArray[i].CommandAllocator = nullptr;
				}
			}

			delete[] d3d12::FrameContextArray;
			d3d12::FrameContextArray = NULL;
		}

		if (d3d12::pID3D12ResourceArray)
		{
			for (UINT i = 0; i < d3d12::NUM_BACK_BUFFERS; ++i)
			{
				if (d3d12::pID3D12ResourceArray[i])
				{
					d3d12::pID3D12ResourceArray[i]->Release();
					d3d12::pID3D12ResourceArray[i] = nullptr;
				}
			}

			delete[] d3d12::pID3D12ResourceArray;
			d3d12::pID3D12ResourceArray = NULL;
		}

		if (d3d12::RenderTargetDescriptorArray)
		{
			delete[] d3d12::RenderTargetDescriptorArray;
			d3d12::RenderTargetDescriptorArray = NULL;
		}


		if (d3d12::hSwapChainWaitableObject)
		{
			d3d12::hSwapChainWaitableObject = nullptr;
		}

		if (d3d12::hFenceEvent)
		{
			CloseHandle(d3d12::hFenceEvent);
			d3d12::hFenceEvent = nullptr;
		}


		d3d12::NUM_FRAMES_IN_FLIGHT = 0;
		d3d12::NUM_BACK_BUFFERS = 0;


		d3d12::frame_index = 0;
	}

	void clear()
	{
		if (d3d12::FrameContextArray)
		{
			FrameContext* frameCtxt = &d3d12::FrameContextArray[d3d12::frame_index % d3d12::NUM_FRAMES_IN_FLIGHT];

			UINT64 fenceValue = frameCtxt->FenceValue;

			if (fenceValue == 0)
				return; // No fence was signaled

			frameCtxt->FenceValue = 0;

			bool bNotWait = d3d12::pFence->GetCompletedValue() >= fenceValue;

			if (!bNotWait)
			{
				d3d12::pFence->SetEventOnCompletion(fenceValue, d3d12::hFenceEvent);

				WaitForSingleObject(d3d12::hFenceEvent, INFINITE);
			}

			_clear();
		}
	}

	FrameContext* WaitForNextFrameResources()
	{
		UINT nextFrameIndex = d3d12::frame_index + 1;
		d3d12::frame_index = nextFrameIndex;

		HANDLE waitableObjects[] = { d3d12::hSwapChainWaitableObject, NULL };
		constexpr DWORD numWaitableObjects = 1;

		FrameContext* frameCtxt = &d3d12::FrameContextArray[nextFrameIndex % d3d12::NUM_FRAMES_IN_FLIGHT];

		WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

		return frameCtxt;
	}

	void reinit(IDXGISwapChain3* pSwapChain, ID3D12CommandQueue* pCommandQueue)
	{
		init_d3d12(pSwapChain, pCommandQueue);
		ImGui_ImplDX12_CreateDeviceObjects();
	}

	ImFont* start(IDXGISwapChain3* pSwapChain, ID3D12CommandQueue* pCommandQueue, type::tImguiStyle SetStyleFunction)
	{
		static ImFont* s_main_font;

		if (is_ready && get_is_need_reset_imgui())
		{

			reinit(pSwapChain, pCommandQueue);


			is_need_reset_imgui = false;
		}

		if (is_ready)
			return s_main_font;

		init_d3d12(pSwapChain, pCommandQueue);

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		if (SetStyleFunction == nullptr)
			ImGui::StyleColorsDark();
		else
			SetStyleFunction();


		ImGui_ImplWin32_Init(g_data::hWind);
		ImGui_ImplDX12_Init(
			d3d12::pDevice,
			d3d12::NUM_FRAMES_IN_FLIGHT,
			DXGI_FORMAT_R8G8B8A8_UNORM, d3d12::pSrvDescHeap,
			d3d12::pSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			d3d12::pSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

		ImFont* main_font = io.Fonts->AddFontFromMemoryTTF(tahoma_ttf, sizeof(tahoma_ttf), 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());

		if (main_font == nullptr)
			Exit();

		s_main_font = main_font;

		WndProc_hk();
		
		is_ready = true;

		return s_main_font;
	}

	ImFont* add_font(const char* font_path, float font_size)
	{
		if (!is_ready)
			return nullptr;

		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromMemoryTTF(tahoma_ttf, sizeof(tahoma_ttf), 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());

		if (font == nullptr)
			return 0;

		return font;
	}

	void imgui_frame_header()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void imgui_no_border(type::tESP esp_function, ImFont* font)
	{
		/*
		ImGuiStyle& style = ImGui::GetStyle();
		const float bor_size = style.WindowBorderSize;
		style.WindowBorderSize = 0.0f;
		ImGui::GetStyle().WindowPadding = ImVec2(15, 15);
		ImGui::GetStyle().WindowRounding = 5.0f;
		ImGui::GetStyle().FramePadding = ImVec2(5, 5);
		ImGui::GetStyle().FrameRounding = 4.0f;
		ImGui::GetStyle().ItemSpacing = ImVec2(12, 8);
		ImGui::GetStyle().ItemInnerSpacing = ImVec2(8, 6);
		ImGui::GetStyle().IndentSpacing = 25.0f;
		ImGui::GetStyle().ScrollbarSize = 15.0f;
		ImGui::GetStyle().ScrollbarRounding = 9.0f;
		ImGui::GetStyle().GrabMinSize = 5.0f;
		ImGui::GetStyle().GrabRounding = 3.0f;

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		// colors[ImGuiCol_ComboBg] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
//		colors[ImGuiCol_Button] = ImVec4();
//		colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
//		colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		//colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	   // colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	   // colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		//  colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
		 // colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
		 // colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
		colors[ImGuiCol_Tab] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		esp_function(font);
	*/
	}

	void imgui_frame_end()
	{
		FrameContext* frameCtxt = WaitForNextFrameResources();
		UINT backBufferIdx = d3d12::pSwapChain->GetCurrentBackBufferIndex();

		{
			frameCtxt->CommandAllocator->Reset();
			static D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.pResource = d3d12::pID3D12ResourceArray[backBufferIdx];
			d3d12::pCommandList->Reset(frameCtxt->CommandAllocator, NULL);
			d3d12::pCommandList->ResourceBarrier(1, &barrier);
			d3d12::pCommandList->OMSetRenderTargets(1, &d3d12::RenderTargetDescriptorArray[backBufferIdx], FALSE, NULL);
			d3d12::pCommandList->SetDescriptorHeaps(1, &d3d12::pSrvDescHeap);
		}

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12::pCommandList);

		static D3D12_RESOURCE_BARRIER barrier = { };
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.pResource = d3d12::pID3D12ResourceArray[backBufferIdx];

		d3d12::pCommandList->ResourceBarrier(1, &barrier);
		d3d12::pCommandList->Close();

		d3d12::pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&d3d12::pCommandList);

		//HRESULT results = ori_present(pSwapChain, SyncInterval, Flags);

		UINT64 fenceValue = d3d12::fenceLastSignaledValue + 1;
		d3d12::pCommandQueue->Signal(d3d12::pFence, fenceValue);
		d3d12::fenceLastSignaledValue = fenceValue;
		frameCtxt->FenceValue = fenceValue;

	}
}

__declspec(dllexport)HRESULT present_hk(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!pSwapChain)
		return ori_present(pSwapChain, SyncInterval, Flags);

	ImFont* main_font = imgui::start(
		static_cast<IDXGISwapChain3*>(pSwapChain),
		reinterpret_cast<ID3D12CommandQueue*>(*(QWORD*)(g_data::base + direcX::command_queue)), nullptr);

	imgui::imgui_frame_header();

	g_menu::menu();

	imgui::imgui_no_border(g_game::init, main_font);

	imgui::imgui_frame_end();

	return ori_present(pSwapChain, SyncInterval, Flags);;
}

VOID initialize()
{
	if (kiero::init(kiero::RenderType::D3D12) == kiero::Status::Success)
	{
		kiero::bind(140, (void**)&ori_present, present_hk);
	}

	//48 8B 05 ? ? ? ? 41 0F B6 D8 80 E3 01 45 8B F0 48 83 3D ? ? ? ? ? 
	/*auto present_ptr = (QWORD)GetModuleHandle((LPCSTR)"graphics-hook64.dll") + 0x7640;
	MH_Initialize();
	MH_CreateHook(reinterpret_cast<LPVOID>(present_ptr), &present_hk, reinterpret_cast<void**>(&ori_present));
	MH_EnableHook(reinterpret_cast<LPVOID>(present_ptr));*/


	//48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B D9 41 8B F8
	/*auto present_ptr = (QWORD)GetModuleHandle((LPCSTR)"DiscordHook64.dll") + 0x6B0F0;
	MH_Initialize();
	MH_CreateHook(reinterpret_cast<LPVOID>(present_ptr), &present_hk, reinterpret_cast<void**>(&ori_present));
	MH_EnableHook(reinterpret_cast<LPVOID>(present_ptr));*/

}
void unhookPresent()
{
	kiero::unbind(140);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) 
{
	if (reason == DLL_PROCESS_ATTACH) 
	{
		g_data::init();

		MH_Initialize();
		initialize();
	}

	return TRUE;
}

namespace ogr_function
{
	tWndProc WndProc;
}

LRESULT hkWndProc(HWND hWnd, UINT Msg, WPARAM wp, LPARAM lp)
{
	switch (Msg)
	{
		case 0x403:
		case WM_SIZE:
		{
			if (Msg == WM_SIZE && wp == SIZE_MINIMIZED)
				break;

			if (imgui::IsReady())
			{

				imgui::clear();

				ImGui_ImplDX12_InvalidateDeviceObjects();


				imgui::reset_imgui_request();
			}
			break;
		}
	};

	ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wp, lp);

	return ogr_function::WndProc(hWnd, Msg, wp, lp);
}

void WndProc_hk()
{
	ogr_function::WndProc = (WNDPROC)SetWindowLongPtrW(g_data::hWind, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
}