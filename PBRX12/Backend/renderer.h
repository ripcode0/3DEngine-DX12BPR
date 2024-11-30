#include "pch.h"

#include <d3dcompiler.h> 
#pragma comment(lib, "d3dcompiler.lib")
#include "d3dx12.h" 

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}


// TODO: Part 1b



// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX12Surface d3d;




	// what we need at a minimum to draw a triangle
	D3D12_VERTEX_BUFFER_VIEW					vertexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>		vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>	rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>	pipeline;

	// TODO: Part 1g
	// TODO: Part 2a
	// TODO: Part 2b
	// TODO: Part 2c
	// TODO: Part 2e
	// TODO: Part 4f


public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX12Surface _d3d)
	{
		win = _win;
		d3d = _d3d;

		InitializeGraphics();
	}

private:
	//constructor helper functions
	void InitializeGraphics()
	{
		ID3D12Device* creator;
		d3d.GetDevice((void**)&creator);

		InitializeVertexBuffer(creator);
		// TODO: Part 1g
		// TODO: part 2a
		// TODO: part 2b
		// TODO: Part 2d
		// TODO: Part 2e
		// TODO: Part 2f
		// TODO: Part 4f
		InitializeGraphicsPipeline(creator);

		// free temporary handle
		creator->Release();
	}

	void InitializeVertexBuffer(ID3D12Device* creator)
	{
		// TODO: Part 1c
		float verts[] = {
			0,   0.5f,
			0.5f, -0.5f,
			-0.5f, -0.5f
		};

		CreateVertexBuffer(creator, sizeof(verts));
		WriteToVertexBuffer(verts, sizeof(verts));
		CreateVertexView(sizeof(float) * 2, sizeof(verts));// TODO: Part 1e, Part 1d
	}

	void CreateVertexBuffer(ID3D12Device* creator, unsigned int sizeInBytes)
	{
		creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));
	}

	void WriteToVertexBuffer(const void* dataToWrite, unsigned int sizeInBytes)
	{
		UINT8* transferMemoryLocation;
		vertexBuffer->Map(0, &CD3DX12_RANGE(0, 0), reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, dataToWrite, sizeInBytes);
		vertexBuffer->Unmap(0, nullptr);
	}

	void CreateVertexView(unsigned int strideInBytes, unsigned int sizeInBytes)
	{
		vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexView.StrideInBytes = strideInBytes;
		vertexView.SizeInBytes = sizeInBytes;
	}

	void InitializeGraphicsPipeline(ID3D12Device* creator)
	{
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(creator, compilerFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(creator, compilerFlags);
		CreateRootSignature(creator);
		CreatePipelineState(creator, vsBlob, psBlob);
	}



	Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D12Device* creator, UINT compilerFlags)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

		std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.hlsl");

		HRESULT compilationResult =
			D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "vs_5_0", compilerFlags, 0,
				vsBlob.GetAddressOf(), errors.GetAddressOf());

		if (FAILED(compilationResult))
		{
			PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return vsBlob;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D12Device* creator, UINT compilerFlags)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

		std::string pixelShaderSource = ReadFileIntoString("../Shaders/PixelShader.hlsl");

		HRESULT compilationResult =
			D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "ps_5_0", compilerFlags, 0,
				psBlob.GetAddressOf(), errors.GetAddressOf());

		if (FAILED(compilationResult))
		{
			PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return psBlob;
	}

	void CreateRootSignature(ID3D12Device* creator)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> signature, errors;
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors);

		creator->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	}

	void CreatePipelineState(
		ID3D12Device* creator, 
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, 
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob)
	{
		// TODO: Part 1e
		// Create Input Layout
		D3D12_INPUT_ELEMENT_DESC formats[1];
		formats[0].SemanticName = "POSITION";
		formats[0].SemanticIndex = 0;
		formats[0].Format = DXGI_FORMAT_R32G32_FLOAT;
		formats[0].InputSlot = 0;
		formats[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		formats[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		formats[0].InstanceDataStepRate = 0;
		// TODO: Part 2g

		// create pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc;
		ZeroMemory(&psDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		psDesc.InputLayout = { formats, ARRAYSIZE(formats) };
		psDesc.pRootSignature = rootSignature.Get();
		psDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
		psDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
		psDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psDesc.SampleMask = UINT_MAX;
		psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psDesc.NumRenderTargets = 1;
		psDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psDesc.SampleDesc.Count = 1;

		creator->CreateGraphicsPipelineState(&psDesc, IID_PPV_ARGS(&pipeline));
	}


public:
	void Render()
	{
		// TODO: Part 2a
		PipelineHandles curHandles = GetCurrentPipelineHandles();
		// setup the pipeline
		SetUpPipeline(curHandles);
		// TODO: Part 1d
		// TODO: Part 1h
		// TODO: Part 3b
		// TODO: Part 3c
		// TODO: Part 4d
		// TODO: Part 4e
		curHandles.commandList->DrawInstanced(3, 1, 0, 0);
		// release temp handles
		curHandles.commandList->Release();
	}

private:

	struct PipelineHandles
	{
		ID3D12GraphicsCommandList* commandList;
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView;
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView;
	};

	PipelineHandles GetCurrentPipelineHandles()
	{
		PipelineHandles retval;
		d3d.GetCommandList((void**)&retval.commandList);
		d3d.GetCurrentRenderTargetView((void**)&retval.renderTargetView);
		d3d.GetDepthStencilView((void**)&retval.depthStencilView);
		return retval;
	}

	void SetUpPipeline(PipelineHandles handles)
	{
		handles.commandList->SetGraphicsRootSignature(rootSignature.Get());
		// TODO: Part 2h
		handles.commandList->OMSetRenderTargets(1, &handles.renderTargetView, FALSE, &handles.depthStencilView);
		handles.commandList->SetPipelineState(pipeline.Get());
		handles.commandList->IASetVertexBuffers(0, 1, &vertexView);
		// TODO: Part 1h
		handles.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

public:
	~Renderer()
	{
		// ComPtr will auto release so nothing to do here yet
	}
};

