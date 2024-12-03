// minimalistic code to draw a single triangle, this is not part of the API.
// required for compiling shaders on the fly, consider pre-compiling instead
#include <DDSTextureLoader.h>				//
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
#include "d3dx12.h" // official helper file provided by microsoft
#include "Sprite.h"
#include "Font.h"
#include "libraries/tinyxml2/tinyxml2.h"

// UI Vertex Shader
const char* vertexShaderSource = R"(
// an ultra simple hlsl vertex shader

cbuffer ROOT_CONST : register(b0, space0)
{
	float2 position;
	float2 scale;
	float4 uv_rect;
	float rotation, depth;
};

struct VS_IN
{
	float2 pos : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_OUT
{	
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_OUT main(VS_IN input, uint id : SV_VertexID)
{
	VS_OUT output = (VS_OUT)0;
	float2 uvs[4] = 
	{
		float2(uv_rect.x, uv_rect.y),		// [0,0]
		float2(uv_rect.z, uv_rect.y),		// [1,0]
		float2(uv_rect.x, uv_rect.w),		// [0,1]
		float2(uv_rect.z, uv_rect.w),		// [1,1]
	};
	float2 r = float2(cos(rotation), sin(rotation));
	float2x2 rotate = float2x2(r.x, -r.y, r.y, r.x);
	float2 pos = mul(input.pos.xy * scale, rotate) + position;
	output.pos = float4(pos, depth, 1);
	output.uv = uvs[id];
	return output;
};
)";

// UI Pixel Shader
const char* pixelShaderSource = R"(
// an ultra simple hlsl pixel shader

struct PS_IN
{	
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState filter : register(s0);

float4 main(PS_IN input) : SV_TARGET 
{	
	float4 color = tex.Sample(filter, input.uv);
	return color;
};
)";

//////////////////////////////////////////
// Font Vertex Shader
const char* vertexShaderSource_font = R"(
// an ultra simple hlsl vertex shader

cbuffer ROOT_CONST : register(b0, space0)
{
	float2 position;
	float2 scale;
	float4 uv_rect;
	float rotation, depth;
};

struct VS_IN
{
	float2 pos : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_OUT
{	
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_OUT main(VS_IN input, uint id : SV_VertexID)
{
	VS_OUT output = (VS_OUT)0;
	float2 r = float2(cos(rotation), sin(rotation));
	float2x2 rotate = float2x2(r.x, -r.y, r.y, r.x);
	float2 pos = mul(input.pos.xy * scale, rotate) + position;
	output.pos = float4(pos, depth, 1);
	output.uv = input.uv;
	return output;
};
)";

// Font Pixel Shader
const char* pixelShaderSource_font = R"(
// an ultra simple hlsl pixel shader

struct PS_IN
{	
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState filter : register(s0);

float4 main(PS_IN input) : SV_TARGET 
{	
	float4 color = tex.Sample(filter, input.uv);
	return color;
};
)";

/////////////////////////////////////////////

enum DESC_HEAP_TEXTURE_ID { DRAGON, HUD_BACKPLATE, HUD_HP_LEFT, HUD_HP_RIGHT, HUD_MP_LEFT, HUD_MP_RIGHT, HUD_STAM_BACKPLATE, HUD_STAM, HUD_CENTER, FONT_CONSOLAS, COUNT };

using HUD = std::vector<Sprite>;

// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX12Surface d3d;
	// what we need at a minimum to draw a triangle
	D3D12_VERTEX_BUFFER_VIEW									vertexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>						vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>					rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>					pipeline;


	std::vector<D3D12_VERTEX_BUFFER_VIEW>						vertexView_font_dynamic;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>			vertexBuffer_font_dynamic;

	D3D12_VERTEX_BUFFER_VIEW									vertexView_font_static;
	Microsoft::WRL::ComPtr<ID3D12Resource>						vertexBuffer_font_static;

	Microsoft::WRL::ComPtr<ID3D12PipelineState>					pipeline_font;


	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>				descriptorHeap;
	UINT														cbvDescriptorSize;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>			textureResource;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>			textureUpload;

	GW::INPUT::GInput											inputProxy;

	Sprite														greendragon;
	Text														sample_text_dynamic;
	Text														sample_text_static;
	HUD															hud;
	UINT														width, height;

	Font														consolas_32;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX12Surface _d3d)
	{
		win = _win;
		d3d = _d3d;
		ID3D12Device* creator;
		ID3D12GraphicsCommandList* cmd;
		ID3D12CommandAllocator* allocator;
		ID3D12CommandQueue* queue;
		d3d.GetDevice((void**)&creator);
		d3d.GetCommandList((void**)&cmd);
		d3d.GetCommandAllocator((void**)&allocator);
		d3d.GetCommandQueue((void**)&queue);

		inputProxy.Create(win);
		win.GetClientWidth(width);
		win.GetClientHeight(height);


		////////////////////////////////////////////////////////////////////////////////////
		// Background item creation
		greendragon = Sprite();
		greendragon.SetName("greendragon");
		greendragon.SetScale(0.5f, 0.5f);
		greendragon.SetDepth(0.03f);
		greendragon.SetTextureIndex(DESC_HEAP_TEXTURE_ID::DRAGON);
		greendragon.SetScissorRect({ 0, 0, (float)width, (float)height });
		hud.push_back(greendragon);
		////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////////
		// HUD loading
		std::string filepath = XML_PATH;
		filepath += "hud.xml";
		
		HUD xml_items = LoadHudFromXML(filepath);
		hud.insert(hud.end(), xml_items.begin(), xml_items.end());

		// sorting lambda based on depth of sprites
		auto sortfunc = [=](const Sprite& a, const Sprite& b)
		{
			return a.GetDepth() > b.GetDepth();
		};
		// sort the hud from furthest to closest
		std::sort(hud.begin(), hud.end(), sortfunc);
		////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////////
		// Font loading
		filepath = XML_PATH;
		filepath += "font_consolas_32.xml";
		bool loaded = consolas_32.LoadFromXML(filepath);

		sample_text_dynamic = Text();
		sample_text_dynamic.SetFont(&consolas_32);
		sample_text_dynamic.SetPosition(0.0f, -0.85f);
		sample_text_dynamic.SetScale(0.75f, 0.75f);
		sample_text_dynamic.SetRotation(0.0f);
		sample_text_dynamic.SetDepth(0.01f);

		sample_text_static = Text();
		sample_text_static.SetText("HP:");
		sample_text_static.SetFont(&consolas_32);
		sample_text_static.SetPosition(-0.75f, -0.85f);
		sample_text_static.SetScale(0.75f, 0.75f);
		sample_text_static.SetRotation(0.0f);
		sample_text_static.SetDepth(0.01f);
		sample_text_static.Update(width, height);

		////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////////
		// Vertex & Const Buffer / Pipeline State / Descriptor Heap Creation
		CreateVertexBuffers(creator);
		CreatePipelineStates(creator);
		CreateDescriptorHeap(creator);
		CreateConstBuffersAndViews(creator);
		////////////////////////////////////////////////////////////////////////////////////


		////////////////////////////////////////////////////////////////////////////////////
		// Texture Loading begin
		allocator->Reset();
		cmd->Reset(allocator, nullptr);

		LoadTextureInfoAndCreateShaderResourceViews(creator, cmd, queue);

		cmd->Close();
		Microsoft::WRL::ComPtr<ID3D12CommandList> lists[] = { cmd };
		queue->ExecuteCommandLists(ARRAYSIZE(lists), lists->GetAddressOf());
		// Texture Loading end
		////////////////////////////////////////////////////////////////////////////////////


		////////////////////////////////////////////////////////////////////////////////////
		// free temporary handle
		creator->Release();
		cmd->Release();
		allocator->Release();
		queue->Release();
		////////////////////////////////////////////////////////////////////////////////////


		////////////////////////////////////////////////////////////////////////////////////
		// Console instructions
		std::cout << "Controls" << std::endl;
		std::cout << "========================================" << std::endl;
		std::cout << "Health UP" << "\t\t" << "Left Arrow" << std::endl;
		std::cout << "Health DOWN" << "\t\t" << "Right Arrow" << std::endl;
		////////////////////////////////////////////////////////////////////////////////////
	}

	void Render()
	{
		// grab the context & render target
		ID3D12GraphicsCommandList* cmd;
		D3D12_CPU_DESCRIPTOR_HANDLE rtv;
		D3D12_CPU_DESCRIPTOR_HANDLE dsv;
		d3d.GetCommandList((void**)&cmd);
		d3d.GetCurrentRenderTargetView((void**)&rtv);
		d3d.GetDepthStencilView((void**)&dsv);


		UINT sc_buffer_index = 0;
		d3d.GetSwapChainBufferIndex(sc_buffer_index);


		// setup the pipeline
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> ppHeaps[] = { descriptorHeap.Get() };
		cmd->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps->GetAddressOf());
		cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		////////////////////////////////////////////////////////////////////////////////////
		// Textured Quad drawing here
		cmd->SetGraphicsRootSignature(rootSignature.Get());
		cmd->SetPipelineState(pipeline.Get());
		cmd->IASetVertexBuffers(0, 1, &vertexView);
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


		for (size_t i = 0; i < hud.size(); i++)
		{
			const Sprite& current = hud[i];
			FLOAT toRoot32[] =
			{
				current.GetPosition().x,current.GetPosition().y,
				current.GetScale().x, current.GetScale().y,
				current.GetTexcoordRect().min.x, current.GetTexcoordRect().min.y,
				current.GetTexcoordRect().max.x, current.GetTexcoordRect().max.y,
				current.GetRotation(), current.GetDepth(),
			};
			GW::MATH2D::GRECTANGLE2F sr = current.GetScissorRect();
			CD3DX12_RECT scissor = CD3DX12_RECT((LONG)sr.min.x,(LONG)sr.min.y,(LONG)sr.max.x,(LONG)sr.max.y);
			cmd->RSSetScissorRects(1, &scissor);
			cmd->SetGraphicsRoot32BitConstants(0, ARRAYSIZE(toRoot32), toRoot32, 0);
			CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), current.GetTextureIndex(), cbvDescriptorSize);
			cmd->SetGraphicsRootDescriptorTable(1, srvHandle);
			cmd->DrawInstanced(4, 1, 0, 0);
		}
		////////////////////////////////////////////////////////////////////////////////////


		////////////////////////////////////////////////////////////////////////////////////
		// Dynamic text drawing here
		cmd->SetGraphicsRootSignature(rootSignature.Get());
		cmd->SetPipelineState(pipeline_font.Get());
		cmd->IASetVertexBuffers(0, 1, &vertexView_font_dynamic[sc_buffer_index]);
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		FLOAT toRoot32_font_dynamic[] =
		{
			sample_text_dynamic.GetPosition().x, sample_text_dynamic.GetPosition().y,
			sample_text_dynamic.GetScale().x, sample_text_dynamic.GetScale().y,
			0.0f, 0.0f, 1.0f, 1.0f,
			sample_text_dynamic.GetRotation(), sample_text_dynamic.GetDepth()
		};

		CD3DX12_RECT scissor = CD3DX12_RECT(0, 0, width, height);
		cmd->RSSetScissorRects(1, &scissor);
		cmd->SetGraphicsRoot32BitConstants(0, ARRAYSIZE(toRoot32_font_dynamic), toRoot32_font_dynamic, 0);
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), DESC_HEAP_TEXTURE_ID::FONT_CONSOLAS, cbvDescriptorSize);
		cmd->SetGraphicsRootDescriptorTable(1, srvHandle);
		cmd->DrawInstanced(sample_text_dynamic.GetVertices().size(), 1, 0, 0);
		////////////////////////////////////////////////////////////////////////////////////


		////////////////////////////////////////////////////////////////////////////////////
		// Static text drawing here
		cmd->SetGraphicsRootSignature(rootSignature.Get());
		cmd->SetPipelineState(pipeline_font.Get());
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->IASetVertexBuffers(0, 1, &vertexView_font_static);
		FLOAT toRoot32_font_static[] =
		{
			sample_text_static.GetPosition().x, sample_text_static.GetPosition().y,
			sample_text_static.GetScale().x, sample_text_static.GetScale().y,
			0.0f, 0.0f, 1.0f, 1.0f,
			sample_text_static.GetRotation(), sample_text_static.GetDepth()
		};
		scissor = CD3DX12_RECT(0, 0, width, height);
		cmd->RSSetScissorRects(1, &scissor);
		cmd->SetGraphicsRoot32BitConstants(0, ARRAYSIZE(toRoot32_font_static), toRoot32_font_static, 0);
		srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), DESC_HEAP_TEXTURE_ID::FONT_CONSOLAS, cbvDescriptorSize);
		cmd->SetGraphicsRootDescriptorTable(1, srvHandle);
		cmd->DrawInstanced(sample_text_static.GetVertices().size(), 1, 0, 0);
		////////////////////////////////////////////////////////////////////////////////////



		// release temp handles
		cmd->Release();
	}

	void Update()
	{
		UINT sc_buffer_index = 0;
		d3d.GetSwapChainBufferIndex(sc_buffer_index);

		win.GetClientWidth(width);
		win.GetClientHeight(height);


		static auto start = std::chrono::steady_clock::now();
		double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();


		float states[256] = { 0 };
		for (size_t i = 0; i < ARRAYSIZE(states); i++)
		{
			inputProxy.GetState(G_KEY_UNKNOWN + i, states[i]);
		}

		float speed = 0.5f;
		auto scissor_rect = hud[DESC_HEAP_TEXTURE_ID::HUD_HP_LEFT].GetScissorRect();

		if (states[G_KEY_LEFT]) { if(scissor_rect.min.x > ((width / 2) - (width / 4) - 15)) scissor_rect.min.x -= speed; }
		if (states[G_KEY_RIGHT]) { if(scissor_rect.min.x < (width / 2))	scissor_rect.min.x += speed; }

		hud[DESC_HEAP_TEXTURE_ID::DRAGON].SetRotation(cos(elapsed));
		hud[DESC_HEAP_TEXTURE_ID::HUD_HP_LEFT].SetScissorRect(scissor_rect);

		float right = (width / 2);
		float left = (width / 2) - (width / 4) - 15;
		float current = scissor_rect.min.x;

		float ratio = fabs(right - current) / fabs(right - left);

		sample_text_dynamic.SetText(std::to_string((int)round(ratio * 100)));
		sample_text_dynamic.Update(width, height);

		UINT8* ptr = nullptr;
		vertexBuffer_font_dynamic[sc_buffer_index]->Map(0, &CD3DX12_RANGE(0, 0), reinterpret_cast<void**>(&ptr));
		memcpy(ptr, sample_text_dynamic.GetVertices().data(), sizeof(TextVertex) * sample_text_dynamic.GetVertices().size());
		vertexBuffer_font_dynamic[sc_buffer_index]->Unmap(0, nullptr);
		
	}

	~Renderer()
	{
		// ComPtr will auto release so nothing to do here 
	}

	HRESULT Renderer::LoadTexture(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& cmd,
		const std::wstring filepath, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Microsoft::WRL::ComPtr<ID3D12Resource>& upload, bool* IsCubeMap)
	{
		HRESULT hr = E_NOTIMPL;

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		DirectX::DDS_ALPHA_MODE alphaMode;
		hr = DirectX::LoadDDSTextureFromFile(device.Get(), filepath.c_str(), resource.GetAddressOf(),
			ddsData, subresources, 0Ui64, &alphaMode, IsCubeMap);

		D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();
		CD3DX12_HEAP_PROPERTIES upload_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		UINT64 uploadSize = GetRequiredIntermediateSize(resource.Get(), 0, resourceDesc.MipLevels * resourceDesc.DepthOrArraySize);
		CD3DX12_RESOURCE_DESC upload_resource = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

		// create a heap for uploading
		hr = device->CreateCommittedResource(
			&upload_prop,
			D3D12_HEAP_FLAG_NONE,
			&upload_resource,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(upload.ReleaseAndGetAddressOf()));

		// update the resource using the upload heap
		UINT64 n = UpdateSubresources(cmd.Get(),
			resource.Get(), upload.Get(),
			0, 0, resourceDesc.MipLevels * resourceDesc.DepthOrArraySize,
			subresources.data());

		CD3DX12_RESOURCE_BARRIER resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmd->ResourceBarrier(1, &resource_barrier);

		cmd->DiscardResource(upload.Get(), nullptr);

		return S_OK;
	}

	UINT Renderer::CalculateConstantBufferByteSize(UINT byteSize)
	{
		return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
	};

	void CreateVertexBuffers(Microsoft::WRL::ComPtr<ID3D12Device> creator)
	{
		//////////////////////////////////////////////////////////////////////
		// geometry for a quad w/ uv data
		float verts[] = {
			-1.0f,  1.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f, 1.0f,
			 1.0f, -1.0f, 1.0f, 1.0f
		};
		creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(verts)),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

		UINT8* transferMemoryLocation;
		vertexBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, verts, sizeof(verts));
		vertexBuffer->Unmap(0, nullptr);

		vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexView.StrideInBytes = sizeof(float) * 4;
		vertexView.SizeInBytes = sizeof(verts);
		//////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////
		// dynamic vertex buffer for text
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swap;
		d3d.GetSwapchain4((void**)swap.GetAddressOf());
		DXGI_SWAP_CHAIN_DESC swapDesc = { 0 };
		swap->GetDesc(&swapDesc);

		UINT max_back_buffers = swapDesc.BufferCount;
		vertexBuffer_font_dynamic.resize(max_back_buffers);
		vertexView_font_dynamic.resize(max_back_buffers);

		UINT max_buffer_size = sizeof(TextVertex) * 6 * 5000;		// 5000 letters capacity [6 vertices per quad]
		for (size_t i = 0; i < max_back_buffers; i++)
		{
			creator->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(max_buffer_size),
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vertexBuffer_font_dynamic[i].ReleaseAndGetAddressOf()));

			vertexView_font_dynamic[i].BufferLocation = vertexBuffer_font_dynamic[i]->GetGPUVirtualAddress();
			vertexView_font_dynamic[i].StrideInBytes = sizeof(TextVertex);
			vertexView_font_dynamic[i].SizeInBytes = max_buffer_size;
		}
		//////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////
		// static vertex buffer for text
		max_buffer_size = sizeof(TextVertex) * 6 * sample_text_static.GetText().size();		

		creator->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(max_buffer_size),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vertexBuffer_font_static.ReleaseAndGetAddressOf()));

		//UINT8* transferMemoryLocation;
		vertexBuffer_font_static->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, sample_text_static.GetVertices().data(), sizeof(TextVertex) * sample_text_static.GetVertices().size());
		vertexBuffer_font_static->Unmap(0, nullptr);

		vertexView_font_static.BufferLocation = vertexBuffer_font_static->GetGPUVirtualAddress();
		vertexView_font_static.StrideInBytes = sizeof(TextVertex);
		vertexView_font_static.SizeInBytes = max_buffer_size;		
		//////////////////////////////////////////////////////////////////////
	}

	void CreatePipelineStates(Microsoft::WRL::ComPtr<ID3D12Device> creator)
	{
		// Create Vertex Shader
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> errors;
		std::vector<Microsoft::WRL::ComPtr<ID3DBlob>> vsBlob;
		std::vector<Microsoft::WRL::ComPtr<ID3DBlob>> psBlob;

		const char* vs_shader_files[] =
		{
			vertexShaderSource,
			vertexShaderSource_font

		};
		vsBlob.resize(ARRAYSIZE(vs_shader_files));
		for (size_t i = 0; i < vsBlob.size(); i++)
		{
			errors.Reset();
			if (FAILED(D3DCompile(vs_shader_files[i], strlen(vs_shader_files[i]),
				nullptr, nullptr, nullptr, "main", "vs_5_1", compilerFlags, 0,
				vsBlob[i].GetAddressOf(), errors.GetAddressOf())))
			{
				std::cout << (char*)errors->GetBufferPointer() << std::endl;
				abort();
			}
		}

		const char* ps_shader_files[] =
		{
			pixelShaderSource,
			pixelShaderSource_font
		};
		psBlob.resize(ARRAYSIZE(ps_shader_files));
		for (size_t i = 0; i < psBlob.size(); i++)
		{
			// Create Pixel Shader
			errors.Reset();
			if (FAILED(D3DCompile(ps_shader_files[i], strlen(ps_shader_files[i]),
				nullptr, nullptr, nullptr, "main", "ps_5_1", compilerFlags, 0,
				psBlob[i].GetAddressOf(), errors.GetAddressOf())))
			{
				std::cout << (char*)errors->GetBufferPointer() << std::endl;
				abort();
			}
		}

		// Create Input Layout					
		D3D12_INPUT_ELEMENT_DESC format[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// static samplers
		CD3DX12_STATIC_SAMPLER_DESC sampler(
			0,
			D3D12_FILTER_ANISOTROPIC,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			0.0f, 16U,
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
			0.0f, 3.402823466e+38F,
			D3D12_SHADER_VISIBILITY_PIXEL,
			0);


		// set blending information to the pipeline
		D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
		transparencyBlendDesc.BlendEnable = true;
		transparencyBlendDesc.LogicOpEnable = false;
		transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		CD3DX12_DEPTH_STENCIL_DESC depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		CD3DX12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendState.AlphaToCoverageEnable = false;
		blendState.RenderTarget[0] = transparencyBlendDesc;
		
		CD3DX12_DESCRIPTOR_RANGE rootRanges[1] = { };
		rootRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		CD3DX12_ROOT_PARAMETER rootParams[2] = { };
		rootParams[0].InitAsConstants(10, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParams[1].InitAsDescriptorTable(1, &rootRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

		// create root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(ARRAYSIZE(rootParams), rootParams, 1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		D3D12SerializeRootSignature(&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors);
		creator->CreateRootSignature(0, signature->GetBufferPointer(),
			signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

		// create pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc;
		ZeroMemory(&psDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psDesc.InputLayout = { format, ARRAYSIZE(format) };
		psDesc.pRootSignature = rootSignature.Get();
		psDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob[0].Get());
		psDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob[0].Get());
		psDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psDesc.BlendState = blendState;
		psDesc.DepthStencilState = depthStencilState;
		psDesc.SampleMask = UINT_MAX;
		psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psDesc.NumRenderTargets = 1;
		psDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psDesc.SampleDesc.Count = 1;
		creator->CreateGraphicsPipelineState(&psDesc, IID_PPV_ARGS(&pipeline));



		psDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob[1].Get());
		psDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob[1].Get());
		creator->CreateGraphicsPipelineState(&psDesc, IID_PPV_ARGS(&pipeline_font));
	}

	void CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> creator)
	{
		UINT numberOfConstantBuffers = 0;
		UINT numberOfTextures = DESC_HEAP_TEXTURE_ID::COUNT;
		UINT numberOfDescriptors = numberOfConstantBuffers + numberOfTextures;

		HRESULT hr = E_NOTIMPL;
		// Heap creation		
		D3D12_DESCRIPTOR_HEAP_DESC cbvsrvuavHeapDesc = {};
		cbvsrvuavHeapDesc.NumDescriptors = numberOfDescriptors;		// X number of descriptors to create from cbv_srv_uav heap
		cbvsrvuavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvsrvuavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hr = creator->CreateDescriptorHeap(&cbvsrvuavHeapDesc, IID_PPV_ARGS(descriptorHeap.ReleaseAndGetAddressOf()));
		cbvDescriptorSize = creator->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void CreateConstBuffersAndViews(Microsoft::WRL::ComPtr<ID3D12Device> creator)
	{

	}

	void LoadTextureInfoAndCreateShaderResourceViews(Microsoft::WRL::ComPtr<ID3D12Device> d, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> c, Microsoft::WRL::ComPtr< ID3D12CommandQueue> q)
	{
		UINT textureIndex = 0;
		bool IsCubeMap = false;

		std::wstring texture_names[] =
		{
			L"greendragon.dds",				
			L"HUD_Sharp_backplate.dds",
			L"Health_left.dds",
			L"Health_right.dds",
			L"Mana_left.dds",
			L"Mana_right.dds",
			L"Stamina_backplate.dds",
			L"Stamina.dds",
			L"Center_top.dds",
			L"font_consolas_32.dds"
		};

		UINT texture_names_size = ARRAYSIZE(texture_names);
		textureResource.resize(texture_names_size);
		textureUpload.resize(texture_names_size);
		for (size_t i = 0; i < texture_names_size; i++)
		{
			std::wstring t = LTEXTURES_PATH;
			t += texture_names[i];
			HRESULT hr = LoadTexture(d, c, t, textureResource[i], textureUpload[i], &IsCubeMap);

			D3D12_RESOURCE_DESC resourceDesc = textureResource[i]->GetDesc();
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC();
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = resourceDesc.Format;
			srvDesc.ViewDimension = (IsCubeMap) ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
			CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), textureIndex, cbvDescriptorSize);
			d->CreateShaderResourceView(textureResource[i].Get(), &srvDesc, descHandle);
			textureIndex++;
		}
	}

	std::vector<Sprite>	LoadHudFromXML(std::string filepath)
	{
		std::vector<Sprite> result;

		tinyxml2::XMLDocument document;
		tinyxml2::XMLError error_message = document.LoadFile(filepath.c_str());
		if (error_message != tinyxml2::XML_SUCCESS) 
		{
			std::cout << "XML file [" + filepath + "] did not load properly." << std::endl;
			return std::vector<Sprite>();
		}

		std::string name = document.FirstChildElement("hud")->FindAttribute("name")->Value();
		GW::MATH2D::GVECTOR2F screen_size;
		screen_size.x = atof(document.FirstChildElement("hud")->FindAttribute("width")->Value());
		screen_size.y = atof(document.FirstChildElement("hud")->FindAttribute("height")->Value());

		tinyxml2::XMLElement* current = document.FirstChildElement("hud")->FirstChildElement("element");
		while (current)
		{
			Sprite s = Sprite();
			name = current->FindAttribute("name")->Value();
			FLOAT x = atof(current->FindAttribute("pos_x")->Value());
			FLOAT y = atof(current->FindAttribute("pos_y")->Value());
			FLOAT sx = atof(current->FindAttribute("scale_x")->Value());
			FLOAT sy = atof(current->FindAttribute("scale_y")->Value());
			FLOAT r = atof(current->FindAttribute("rotation")->Value());
			FLOAT d = atof(current->FindAttribute("depth")->Value());
			GW::MATH2D::GVECTOR2F s_min, s_max;
			s_min.x = atof(current->FindAttribute("sr_x")->Value());
			s_min.y = atof(current->FindAttribute("sr_y")->Value());
			s_max.x = atof(current->FindAttribute("sr_w")->Value());
			s_max.y = atof(current->FindAttribute("sr_h")->Value());
			UINT tid = atoi(current->FindAttribute("textureID")->Value());

			s.SetName(name);
			s.SetScale(sx, sy);
			s.SetPosition(x, y);
			s.SetRotation(r);
			s.SetDepth(d);
			s.SetScissorRect({ s_min, s_max });
			s.SetTextureIndex(tid);

			result.push_back(s);

			current = current->NextSiblingElement();
		}
		return result;
	}
};
