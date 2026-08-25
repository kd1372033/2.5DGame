#include "Framework/KdFramework.h"
#include "DecalShader.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

bool DecalShader::Init()
{
	// 1. デバイスの取得
	const ID3D11Device* pDevice = KdDirect3D::Instance().GetDev();
	if (!pDevice) return false;

	// 2. 頂点シェーダー (VS) のコンパイル＆生成
	ID3DBlob* pVSBlob = nullptr;
	if (!CompileShader(L"Src/Framework/Shader/DecalShader/DecalShader.hlsl", "VS", "vs_5_0", &pVSBlob))
	{
		assert(0 && "DecalShader: Vertex Shader Compile Failed");
		return false;
	}

	HRESULT hr = const_cast<ID3D11Device*>(pDevice)->CreateVertexShader(
		pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(),
		nullptr,
		&m_pVS
	);
	if (FAILED(hr)) { pVSBlob->Release(); return false; }

	// 3. 入力レイアウト (Input Layout) の作成
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	hr = const_cast<ID3D11Device*>(pDevice)->CreateInputLayout(
		layout,
		_countof(layout),
		pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(),
		&m_pInputLayout
	);
	pVSBlob->Release();
	if (FAILED(hr)) return false;

	// 4. ピクセルシェーダー (PS) のコンパイル＆生成
	ID3DBlob* pPSBlob = nullptr;
	if (!CompileShader(L"Src/Framework/Shader/DecalShader/DecalShader.hlsl", "PS", "ps_5_0", &pPSBlob))
	{
		assert(0 && "DecalShader: Pixel Shader Compile Failed");
		return false;
	}

	hr = const_cast<ID3D11Device*>(pDevice)->CreatePixelShader(
		pPSBlob->GetBufferPointer(),
		pPSBlob->GetBufferSize(),
		nullptr,
		&m_pPS
	);
	pPSBlob->Release();
	if (FAILED(hr)) return false;

	// 5. 定数バッファの初期化
	m_cbDecal.Create();

	// 6. 1x1x1の単位キューブメッシュの生成 (中心原点)
	std::vector<KdMeshVertex> vertices(8);
	vertices[0].Pos = Math::Vector3(-0.5f, -0.5f, -0.5f);
	vertices[1].Pos = Math::Vector3(-0.5f, 0.5f, -0.5f);
	vertices[2].Pos = Math::Vector3(0.5f, 0.5f, -0.5f);
	vertices[3].Pos = Math::Vector3(0.5f, -0.5f, -0.5f);
	vertices[4].Pos = Math::Vector3(-0.5f, -0.5f, 0.5f);
	vertices[5].Pos = Math::Vector3(-0.5f, 0.5f, 0.5f);
	vertices[6].Pos = Math::Vector3(0.5f, 0.5f, 0.5f);
	vertices[7].Pos = Math::Vector3(0.5f, -0.5f, 0.5f);

	std::vector<KdMeshFace> faces = {
		{0, 1, 2}, {0, 2, 3}, // 前
		{4, 6, 5}, {4, 7, 6}, // 後
		{4, 5, 1}, {4, 1, 0}, // 左
		{3, 2, 6}, {3, 6, 7}, // 右
		{1, 5, 6}, {1, 6, 2}, // 上
		{4, 0, 3}, {4, 3, 7}  // 下
	};

	KdMeshSubset subset;
	subset.MaterialNo = 0;
	subset.FaceStart = 0;
	subset.FaceCount = static_cast<UINT>(faces.size());
	std::vector<KdMeshSubset> subsets = { subset };

	m_spBoxMesh = std::make_shared<KdMesh>();
	if (!m_spBoxMesh->Create(vertices, faces, subsets, false))
	{
		return false;
	}

	return true;
}

// HLSLコンパイル処理
bool DecalShader::CompileShader(
	const wchar_t* filename,
	const char* entryPoint,
	const char* shaderModel,
	ID3DBlob** ppBlob
)
{
	DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(
		filename,
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint,
		shaderModel,
		shaderFlags,
		0,
		ppBlob,
		&pErrorBlob
	);

	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		return false;
	}

	if (pErrorBlob) pErrorBlob->Release();
	return true;
}

void DecalShader::Draw(
	const std::shared_ptr<KdTexture>& spDecalTex,
	ID3D11ShaderResourceView* pDepthSRV,
	const Math::Matrix& mWorld,
	const Math::Color& color
)
{
	if (!m_pVS || !m_pPS || !spDecalTex || !m_spBoxMesh) return;

	// デバイスコンテキストの取得
	const ID3D11DeviceContext* pContext = KdDirect3D::Instance().GetDevContext();
	if (!pContext) return;

	ID3D11DeviceContext* pWorkContext = const_cast<ID3D11DeviceContext*>(pContext);

	// 1. 定数バッファの更新
	cbDecal& cb = m_cbDecal.Work();
	cb.mDecalWorldInv = mWorld.Invert();
	cb.mDecalColor = color;

	m_cbDecal.Write();

	// 2. パイプラインステートの設定
	pWorkContext->IASetInputLayout(m_pInputLayout);
	pWorkContext->VSSetShader(m_pVS, nullptr, 0);
	pWorkContext->PSSetShader(m_pPS, nullptr, 0);

	// 3. 定数バッファのセット (b1: デカールバッファ)
	pWorkContext->VSSetConstantBuffers(1, 1, m_cbDecal.GetAddress());
	pWorkContext->PSSetConstantBuffers(1, 1, m_cbDecal.GetAddress());

	// 4. テクスチャ(SRV)の設定 (t0: 深度バッファ, t1: デカール画像)
	ID3D11ShaderResourceView* srvs[2] = {
		pDepthSRV,
		const_cast<ID3D11ShaderResourceView*>(spDecalTex->GetSRView())
	};
	pWorkContext->PSSetShaderResources(0, 2, srvs);

	// 5. メッシュの描画
	m_spBoxMesh->SetToDevice();
	for (size_t i = 0; i < m_spBoxMesh->GetSubsets().size(); ++i)
	{
		m_spBoxMesh->DrawSubset(static_cast<int>(i));
	}

	// 6. クリーンアップ
	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	pWorkContext->PSSetShaderResources(0, 2, nullSRVs);
}

void DecalShader::Release()
{
	KdSafeRelease(m_pVS);
	KdSafeRelease(m_pPS);
	KdSafeRelease(m_pInputLayout);
	m_cbDecal.Release();
	m_spBoxMesh = nullptr;
}

DecalShader::~DecalShader()
{
	Release();
}