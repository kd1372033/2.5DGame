#pragma once

class DecalShader
{
public:
	// デカール用定数バッファ用構造体（b1スロットに送る）
	struct cbDecal
	{
		Math::Matrix mDecalWorldInv; // 投影箱のワールド逆行列（ビュー空間→ローカル空間への変換用）
		Math::Color  mDecalColor;    // 色・透過度
	};

	DecalShader() = default;
	~DecalShader();

	// 初期化（HLSLのコンパイルと各オブジェクト生成）
	bool Init();

	// 解放処理
	void Release();

	// 描画実行
	void Draw(
		const std::shared_ptr<KdTexture>& spDecalTex,
		ID3D11ShaderResourceView* pDepthSRV,
		const Math::Matrix& mWorld,
		const Math::Color& color = { 1.0f, 1.0f, 1.0f, 1.0f }
	);

private:
	// シェーダーコンパイル用ヘルパー関数
	bool CompileShader(
		const wchar_t* filename,
		const char* entryPoint,
		const char* shaderModel,
		ID3DBlob** ppBlob
	);

	// シェーダー・パイプラインオブジェクト
	ID3D11VertexShader* m_pVS = nullptr;
	ID3D11PixelShader* m_pPS = nullptr;
	ID3D11InputLayout* m_pInputLayout = nullptr;

	// 定数バッファ (cbDecal)
	KdConstantBuffer<cbDecal> m_cbDecal;

	// 投影用単位キューブメッシュ（1x1x1の箱ポリゴン）
	std::shared_ptr<KdMesh> m_spBoxMesh;
};