#pragma once

class KdPostProcessShader
{
public:
	KdPostProcessShader() {}
	~KdPostProcessShader()
	{
		Release();
	}

	void SetNearClippingDistance(float distance) { m_cb0_DoFInfo.Work().NearClippingDistance = distance; }
	void SetFarClippingDistance(float distance) { m_cb0_DoFInfo.Work().FarClippingDistance = distance; }
	void SetFocusDistance(float distance) { m_cb0_DoFInfo.Work().FocusDistance = distance; }
	void SetFocusRange(float fore, float back) { m_cb0_DoFInfo.Work().FocusForeRange = fore; m_cb0_DoFInfo.Work().FocusBackRange = back; }

	void SetBrightThreshold(float threshold) { m_cb0_BrightInfo.Work().Threshold = threshold; }

	// 8/26追加 ============================================================================

	// ★追加：C++側から行列データをセットする関数
	void SetLightProjectorMatrix(const Math::Matrix& invViewProj, const Math::Matrix& lightViewProj)
	{
		m_cb0_LightProjectorInfo.Work().mInvViewProj = invViewProj;
		m_cb0_LightProjectorInfo.Work().mLightViewProj = lightViewProj;
	}

	// ★追加：ポストプロセス用のテクスチャ・Zバッファ取得用アクセサ
	const std::shared_ptr<KdTexture>& GetPostEffectRTTex() const { return m_postEffectRTPack.m_RTTexture; }
	const std::shared_ptr<KdTexture>& GetPostEffectZBuffer() const { return m_postEffectRTPack.m_ZBuffer; }

	// ★修正：引数に const を追加
	void DrawLightProjector(
		const std::shared_ptr<KdTexture>& spSrcTex,
		const std::shared_ptr<KdTexture>& spDepthTex,
		const std::shared_ptr<KdTexture>& spLightTex,
		const std::shared_ptr<KdTexture>& spDstTex);

	// =====================================================================================

	struct Vertex
	{
		Math::Vector3 Pos;
		Math::Vector2 UV;
	};

	bool Init();

	void Release();

	void Draw();

	void BeginBright();
	void EndBright();

	void PostEffectProcess();

	// 8/26追加 ============================================================================
	// ★追加：ライト投影描画関数の宣言
	void DrawLightProjector(
		std::shared_ptr<KdTexture>& spSrcTex,
		std::shared_ptr<KdTexture>& spDepthTex,
		std::shared_ptr<KdTexture>& spLightTex,
		std::shared_ptr<KdTexture>& spDstTex);
	// =====================================================================================

	void GenerateBlurTexture(std::shared_ptr<KdTexture>& spSrcTex, std::shared_ptr<KdTexture>& spDstTex, D3D11_VIEWPORT& VP, int blurRadius);

private:

	void BlurProcess();
	void LightBloomProcess();
	void DepthOfFieldProcess();

	void CreateBlurOffsetList(std::vector<Math::Vector3>& dstInfo, const std::shared_ptr<KdTexture>& spSrcTex, int samplingSize, const Math::Vector2& dir);

	void DrawTexture(std::shared_ptr<KdTexture>* spSrcTex, int srcTexSize, std::shared_ptr<KdTexture> spDstTex, D3D11_VIEWPORT* pVP);

	void SetBlurInfo(const std::shared_ptr<KdTexture>& spSrcTex, int samplingSize, const Math::Vector2& dir);
	void SetBlurInfo(const std::vector<Math::Vector3>& srcInfo);

	void SetBlurToDevice();
	void SetDoFToDevice();
	void SetBrightToDevice();

	// 8/26追加 ============================================================================
	void SetLightProjectorToDevice();
	// =====================================================================================


	ID3D11VertexShader* m_VS = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;

	ID3D11PixelShader* m_PS_Blur = nullptr;
	ID3D11PixelShader* m_PS_DoF = nullptr;
	ID3D11PixelShader* m_PS_Bright = nullptr;

	// 8/26追加 ============================================================================

	ID3D11PixelShader* m_PS_LightProjector = nullptr; // ★追加：ライト投影用シェーダー

	// =====================================================================================

	static const int kBlurSamplingRadius = 8;
	static const int kLightBloomSamplingRadius = 4;

	static const int kMaxSampling = 31;
	struct cbBlur
	{
		Math::Vector4 Info[kMaxSampling];
	
		int SamplingNum = 0;
		int _blank[3] = { 0, 0 ,0 };
	};
	KdConstantBuffer<cbBlur>	m_cb0_BlurInfo;

	struct cbDepthOfField
	{
		float NearClippingDistance = 0.0f;
		float FarClippingDistance = 1000.0f;

		float FocusDistance = 0.0f;
		float FocusForeRange = 0.0f;
		float FocusBackRange = 1000.0f;
		int   _blank[3] = { 0, 0, 0 };
	};
	KdConstantBuffer<cbDepthOfField>	m_cb0_DoFInfo;

	struct cbBrightFilter
	{
		float Threshold = 0.0f;
		int _blank[3] = { 0, 0, 0 };
	};
	KdConstantBuffer<cbBrightFilter>	m_cb0_BrightInfo;

	// 8/26追加 ===========================================================================

	// ★追加：ライト投影に必要な行列データをシェーダーに送る構造体
	struct cbLightProjector
	{
		Math::Matrix mInvViewProj;  // カメラの逆行列（ワールド座標復元用）
		Math::Matrix mLightViewProj; // ライトのビュー・プロジェクション行列
	};
	KdConstantBuffer<cbLightProjector> m_cb0_LightProjectorInfo; // ★追加

	//=====================================================================================

	KdRenderTargetPack	m_postEffectRTPack;

	KdRenderTargetPack	m_blurRTPack;
	KdRenderTargetPack	m_strongBlurRTPack;

	KdRenderTargetPack	m_depthOfFieldRTPack;

	KdRenderTargetPack	m_brightEffectRTPack;
	static const int	kLightBloomNum = 4;
	KdRenderTargetPack	m_lightBloomRTPack[kLightBloomNum];

	KdRenderTargetChanger m_postEffectRTChanger;
	KdRenderTargetChanger m_brightRTChanger;

	Vertex m_screenVert[4];
};
