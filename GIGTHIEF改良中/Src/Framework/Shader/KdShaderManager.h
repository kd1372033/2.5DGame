#pragma once

#include "StandardShader/KdStandardShader.h"
#include "PostProcessShader/KdPostProcessShader.h"
#include "SpriteShader/KdSpriteShader.h"

// 点光源データ
struct PointLight
{
	PointLight() {}
	PointLight(const Math::Vector3& color, float radius, const Math::Vector3& pos, int isBright)
		:Color(color), Radius(radius), Pos(pos), IsBright(isBright) {
	}

	Math::Vector3 Color;	// 色
	float	Radius = 0.0f;	// 半径
	Math::Vector3 Pos;		// 座標
	int		IsBright = 0;	// 明度用ライトかどうか
};

// コーンライト（単体用データ構造）
struct ConeLight
{
	Math::Vector3 Pos = Math::Vector3::Zero;
	float Angle = 0.0f;

	Math::Vector3 Dir = Math::Vector3::Zero;
	float Range = 0.0f;

	Math::Vector3 Color = Math::Vector3::Zero;
	float Intensity = 1.0f;

	int IsHit = 0;
	Math::Vector3 HitPos = Math::Vector3::Zero;

	Math::Vector3 HitNormal = Math::Vector3::Zero;
	float EdgeSmoothness = 0.5f;
};

// ★ 最大数を 20 個に拡張
static const int kMaxConeLightCount = 20;

struct cbConeLight
{
	int count = 0;
	Math::Vector3 padding;
	ConeLight list[kMaxConeLightCount];
};

// ★ カラースフィア（単体用データ構造）
struct EffectSphere
{
	Math::Vector3 Pos = Math::Vector3::Zero;
	float Radius = 0.0f;
	Math::Vector3 Color = { 1.0f, 0.0f, 0.0f };
	float Padding = 0.0f;
};

static const int kMaxEffectSphereCount = 10;

// ★ カラースフィア定数バッファ構造体 (b10)
struct cbEffect
{
	int colorEnable = 0;
	int colorCount = 0;
	Math::Vector2 padding;
	EffectSphere list[kMaxEffectSphereCount];
};

//==========================
// 各パイプラインステートの呼び出しID
//==========================

enum class KdDepthStencilState
{
	ZEnable,		// 手前の物に隠れる ○、自分の奥の物を隠す ○
	ZWriteDisable,	// 手前の物に隠れる ○、自分の奥の物を隠す ×
	ZDisable,		// 手前の物に隠れる ×、自分の奥の物を隠す ×
	Max
};

enum class KdRasterizerState
{
	CullNone,		// 描画省略なし
	CullFront,		// 表面描画省略
	CullBack,		// 裏面描画省略
	Max
};

enum class KdBlendState
{
	Alpha,			// 透明度による色合成
	Add,			// 加算による色合成
	Max
};

enum class KdSamplerState
{
	Anisotropic_Wrap,
	Anisotropic_Clamp,
	Linear_Clamp,
	Linear_Clamp_Cmp,
	Point_Wrap,
	Point_Clamp,
	Max
};

//==========================================================
// シェーダ関係をまとめたクラス
//==========================================================
class KdShaderManager
{
public:

	// 外部参照用の型別名エイリアス
	using EffectSphere = ::EffectSphere;

	// カメラ定数バッファ用構造体
	struct cbCamera
	{
		Math::Matrix	mView;		// ビュー行列
		Math::Matrix	mProj;		// 射影行列
		Math::Matrix	mProjInv;	// 射影行列：逆行列

		Math::Vector3	CamPos;		// カメラのワールド座標
		int				blank = 0;
	};

	// フォグ定数バッファ用構造体
	struct cbFog
	{
		int				DistanceFogEnable = 0;			// 距離フォグ有効
		Math::Vector3	DistanceFogColor = { 1.0f, 1.0f, 1.0f };// 距離フォグ色

		float			DistanceFogDensity = 0.001f;	// 距離フォグ減衰率
		float			HeightFogTopValue = 5.0f;		// 高さフォグエリアの最高位置
		float			HeightFogBottomValue = -5.0f;	// 高さフォグエリアの最低位置
		float			HeightFogBeginDistance = 0.0f;	// 高さフォグの開始の距離

		Math::Vector3	HeightFogColor = { 1.0f, 1.0f, 1.0f };// 高さフォグ色
		int				HeightFogEnable = 0;			// 高さフォグ有効
	};

	// ライト定数バッファ用構造体 (16バイトアライメント)
	struct cbLight
	{
		static const int MaxPointLightNum = 100;

		Math::Vector4 AmbientLight = { 0.3f, 0.3f, 0.3f, 1.0f };

		Math::Vector3 DirLight_Dir = { 1, -1, 1 };
		float         _blank1 = 0.0f;
		Math::Vector3 DirLight_Color = { 2.25f, 2.25f, 2.25f };
		float         _blank2 = 0.0f;

		Math::Matrix  DirLight_mVP;

		int           PointLight_Num = 0;
		float         _blank3[3] = { 0.0f, 0.0f, 0.0f };
		std::array<PointLight, MaxPointLightNum> PointLights;

		int           ConeLight_Enable = 0;
		Math::Vector3 ConeLight_Pos = { 0, 0, 0 };

		Math::Vector3 ConeLight_Dir = { 0, -1, 0 };
		float         ConeLight_Angle = 45.0f;

		Math::Vector3 ConeLight_Color = { 1, 1, 1 };
		float         ConeLight_Range = 10.0f;

		float         ConeLight_Intensity = 1.0f;
		float         ConeLight_EdgeSmoothness = 0.5f;
		int           ConeLight_IsHit = 0;
		float         _blank_cone1 = 0.0f;

		Math::Vector3 ConeLight_HitPos = { 0, 0, 0 };
		float         _blank_cone2 = 0.0f;

		Math::Vector3 ConeLight_HitNormal = { 0, 1, 0 };
		float         _blank_cone3 = 0.0f;
	};

	static KdShaderManager& Instance()
	{
		static KdShaderManager instance;
		return instance;
	}

	void Init();

	//==========================
	// シェーダ
	//==========================
	KdStandardShader		m_StandardShader;		// 標準描画シェーダ
	KdPostProcessShader		m_postProcessShader;	// ポストプロセスシェーダ
	KdSpriteShader			m_spriteShader;			// 2Dテクスチャ描画シェーダ

	//==========================
	// 描画パイプライン系の設定
	//==========================
	bool SetVertexShader(ID3D11VertexShader* pSetVS);
	bool SetPixelShader(ID3D11PixelShader* pSetPS);

	bool SetInputLayout(ID3D11InputLayout* pSetLayout);

	bool SetVSConstantBuffer(int startSlot, ID3D11Buffer* const* pSetBuffer);
	bool SetPSConstantBuffer(int startSlot, ID3D11Buffer* const* pSetBuffer);

	//==========================
	// パイプラインステートの変更
	//==========================
	void ChangeDepthStencilState(KdDepthStencilState stateId);
	void UndoDepthStencilState();

	void ChangeRasterizerState(KdRasterizerState stateId);
	void UndoRasterizerState();

	void ChangeBlendState(KdBlendState stateId);
	void UndoBlendState();

	void ChangeSamplerState(KdSamplerState stateId, int slot = 0);
	void UndoSamplerState(int slot = 0);

	//==========================
	// 定数バッファ系
	//==========================
	const cbCamera& GetCameraCB() const { return m_cb7_Camera.Get(); }
	const cbFog& GetFogCB() const { return m_cb8_Fog.Get(); }
	const cbLight& GetLightCB() const { return m_cb9_Light.Get(); }

	void WriteCBCamera(const Math::Matrix& cam, const Math::Matrix& proj);

	void WriteCBFogEnable(bool distance, bool height);
	void WriteCBDistanceFog(const Math::Vector3& col, float density = 0.001f);
	void WriteCBHeightFog(const Math::Vector3& col, float top, float bottom, float beginDistance);

	void WriteCBAmbientLight(const Math::Vector4& col);
	void WriteCBDirectionalLight(const Math::Vector3& dir, const Math::Vector3& col);
	void WriteCBShadowArea(const Math::Matrix& proj, float dirLightHeight);
	void WriteCBPointLight(const std::list<PointLight>& pointLights);

	// 単体コーンライト用書き込み関数
	void WriteCBConeLightEnable(bool enable);
	void WriteCBConeLight2D(
		const Math::Vector3& pos,
		const Math::Vector3& dir,
		float angle,
		float range,
		const Math::Vector3& col,
		float intensity = 1.0f,
		float edgeSmoothness = 0.5f,
		bool isHit = false,
		const Math::Vector3& hitPos = Math::Vector3::Zero,
		const Math::Vector3& hitNormal = Math::Vector3::Up
	);
	void WriteCBConeLight(const ConeLight& coneLight);
	void WriteCBConeLight(const std::vector<ConeLight>& coneLights);

	// カラースフィア（複数対応）転送関数 (b10)
	void WriteCBColorEnable(bool enable);
	void WriteCBColor(const std::vector<::EffectSphere>& sphereList);

	//==========================
	// その他
	//==========================

	bool IsPixelArtStyle() const { return m_pixelArtStyle; }

	const KdAmbientController& GetAmbientController() const { return m_ambientController; }
	KdAmbientController& WorkAmbientController() { return m_ambientController; }

	void Release();

private:

	KdShaderManager() {}
	~KdShaderManager() {}

	// カメラ定数バッファ (b7)
	KdConstantBuffer<cbCamera>	m_cb7_Camera;

	// フォグ定数バッファ (b8)
	KdConstantBuffer<cbFog>	m_cb8_Fog;

	// ライト定数バッファ (b9)
	KdConstantBuffer<cbLight>	m_cb9_Light;

	// ★ カラースフィア用定数バッファ (b10)
	KdConstantBuffer<cbEffect>  m_cb10_Effect;

	// ★ 複数コーンライト用定数バッファ (b11)
	KdConstantBuffer<cbConeLight> m_cb11_ConeLight;

	KdAmbientController m_ambientController;

	bool m_pixelArtStyle = true;

	// パイプラインステート
	ID3D11DepthStencilState* m_depthStencilStates[(int)KdDepthStencilState::Max] = {};
	std::stack<ID3D11DepthStencilState*> m_ds_Undo;

	ID3D11RasterizerState* m_rasterizerStates[(int)KdRasterizerState::Max] = {};
	std::stack<ID3D11RasterizerState*> m_rs_Undo;

	ID3D11BlendState* m_blendStates[(int)KdBlendState::Max] = {};
	std::stack<ID3D11BlendState*> m_bs_Undo;

	ID3D11SamplerState* m_samplerStates[(int)KdSamplerState::Max] = {};
	std::stack<ID3D11SamplerState*> m_ss_Undo;
};