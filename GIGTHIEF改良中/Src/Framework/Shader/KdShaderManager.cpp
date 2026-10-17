#include "Framework/KdFramework.h"
#include "KdShaderManager.h"

void KdShaderManager::Init()
{
	m_spriteShader.Init();
	m_StandardShader.Init();
	m_postProcessShader.Init();

	// b7: Camera
	m_cb7_Camera.Create();
	KdDirect3D::Instance().WorkDevContext()->VSSetConstantBuffers(7, 1, m_cb7_Camera.GetAddress());
	KdDirect3D::Instance().WorkDevContext()->PSSetConstantBuffers(7, 1, m_cb7_Camera.GetAddress());

	// b8: Fog
	m_cb8_Fog.Create();
	m_cb8_Fog.Write();
	KdDirect3D::Instance().WorkDevContext()->VSSetConstantBuffers(8, 1, m_cb8_Fog.GetAddress());
	KdDirect3D::Instance().WorkDevContext()->PSSetConstantBuffers(8, 1, m_cb8_Fog.GetAddress());

	// b9: Light
	m_cb9_Light.Create();
	m_cb9_Light.Write();
	KdDirect3D::Instance().WorkDevContext()->VSSetConstantBuffers(9, 1, m_cb9_Light.GetAddress());
	KdDirect3D::Instance().WorkDevContext()->PSSetConstantBuffers(9, 1, m_cb9_Light.GetAddress());

	// b10: Effect (カラースフィアバリア・複数対応)
	m_cb10_Effect.Create();
	m_cb10_Effect.Write();
	KdDirect3D::Instance().WorkDevContext()->VSSetConstantBuffers(10, 1, m_cb10_Effect.GetAddress());
	KdDirect3D::Instance().WorkDevContext()->PSSetConstantBuffers(10, 1, m_cb10_Effect.GetAddress());

	// b11: ConeLight (複数コーンライト対応)
	m_cb11_ConeLight.Create();
	m_cb11_ConeLight.Write();
	KdDirect3D::Instance().WorkDevContext()->VSSetConstantBuffers(11, 1, m_cb11_ConeLight.GetAddress());
	KdDirect3D::Instance().WorkDevContext()->PSSetConstantBuffers(11, 1, m_cb11_ConeLight.GetAddress());

	// ステート初期化
	m_depthStencilStates[(int)KdDepthStencilState::ZEnable] = KdDirect3D::Instance().CreateDepthStencilState(true, true);
	m_depthStencilStates[(int)KdDepthStencilState::ZWriteDisable] = KdDirect3D::Instance().CreateDepthStencilState(true, false);
	m_depthStencilStates[(int)KdDepthStencilState::ZDisable] = KdDirect3D::Instance().CreateDepthStencilState(false, false);
	KdDirect3D::Instance().WorkDevContext()->OMSetDepthStencilState(m_depthStencilStates[(int)KdDepthStencilState::ZEnable], 0);

	m_rasterizerStates[(int)KdRasterizerState::CullNone] = KdDirect3D::Instance().CreateRasterizerState(D3D11_CULL_NONE, D3D11_FILL_SOLID, true, false);
	m_rasterizerStates[(int)KdRasterizerState::CullFront] = KdDirect3D::Instance().CreateRasterizerState(D3D11_CULL_FRONT, D3D11_FILL_SOLID, true, false);
	m_rasterizerStates[(int)KdRasterizerState::CullBack] = KdDirect3D::Instance().CreateRasterizerState(D3D11_CULL_BACK, D3D11_FILL_SOLID, true, false);
	KdDirect3D::Instance().WorkDevContext()->RSSetState(m_rasterizerStates[(int)KdRasterizerState::CullBack]);

	m_blendStates[(int)KdBlendState::Alpha] = KdDirect3D::Instance().CreateBlendState(KdBlendMode::Alpha);
	m_blendStates[(int)KdBlendState::Add] = KdDirect3D::Instance().CreateBlendState(KdBlendMode::Add);
	KdDirect3D::Instance().WorkDevContext()->OMSetBlendState(m_blendStates[(int)KdBlendState::Alpha], Math::Color(0, 0, 0, 0), 0xFFFFFFFF);

	m_samplerStates[(int)KdSamplerState::Anisotropic_Wrap] = KdDirect3D::Instance().CreateSamplerState(KdSamplerFilterMode::Anisotropic, 4, KdSamplerAddressingMode::Wrap, false);
	m_samplerStates[(int)KdSamplerState::Anisotropic_Clamp] = KdDirect3D::Instance().CreateSamplerState(KdSamplerFilterMode::Anisotropic, 4, KdSamplerAddressingMode::Clamp, false);
	m_samplerStates[(int)KdSamplerState::Linear_Clamp] = KdDirect3D::Instance().CreateSamplerState(KdSamplerFilterMode::Linear, 0, KdSamplerAddressingMode::Clamp, false);
	m_samplerStates[(int)KdSamplerState::Linear_Clamp_Cmp] = KdDirect3D::Instance().CreateSamplerState(KdSamplerFilterMode::Linear, 0, KdSamplerAddressingMode::Clamp, true);
	m_samplerStates[(int)KdSamplerState::Point_Wrap] = KdDirect3D::Instance().CreateSamplerState(KdSamplerFilterMode::Point, 0, KdSamplerAddressingMode::Wrap, false);
	m_samplerStates[(int)KdSamplerState::Point_Clamp] = KdDirect3D::Instance().CreateSamplerState(KdSamplerFilterMode::Point, 0, KdSamplerAddressingMode::Clamp, false);

	if (m_pixelArtStyle) {
		KdDirect3D::Instance().WorkDevContext()->PSSetSamplers(0, 1, &m_samplerStates[(int)KdSamplerState::Point_Wrap]);
	}
	else {
		KdDirect3D::Instance().WorkDevContext()->PSSetSamplers(0, 1, &m_samplerStates[(int)KdSamplerState::Anisotropic_Wrap]);
	}

	m_ambientController.Init();
}

// パイプライン設定・ステート変更
bool KdShaderManager::SetVertexShader(ID3D11VertexShader* pSetVS) { if (!pSetVS) return false; ID3D11VertexShader* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->VSGetShader(&pNow, nullptr, nullptr); bool change = pNow != pSetVS; if (change) KdDirect3D::Instance().WorkDevContext()->VSSetShader(pSetVS, nullptr, 0); KdSafeRelease(pNow); return change; }
bool KdShaderManager::SetPixelShader(ID3D11PixelShader* pSetPS) { if (!pSetPS) return false; ID3D11PixelShader* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->PSGetShader(&pNow, nullptr, nullptr); bool change = pNow != pSetPS; if (change) KdDirect3D::Instance().WorkDevContext()->PSSetShader(pSetPS, nullptr, 0); KdSafeRelease(pNow); return change; }
bool KdShaderManager::SetInputLayout(ID3D11InputLayout* pSetLayout) { if (!pSetLayout) return false; ID3D11InputLayout* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->IAGetInputLayout(&pNow); bool change = pNow != pSetLayout; if (change) KdDirect3D::Instance().WorkDevContext()->IASetInputLayout(pSetLayout); KdSafeRelease(pNow); return change; }
bool KdShaderManager::SetVSConstantBuffer(int startSlot, ID3D11Buffer* const* pSetVSBuffer) { if (!pSetVSBuffer) return false; ID3D11Buffer* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->VSGetConstantBuffers(startSlot, 1, &pNow); bool change = pNow != *pSetVSBuffer; if (change) KdDirect3D::Instance().WorkDevContext()->VSSetConstantBuffers(startSlot, 1, pSetVSBuffer); KdSafeRelease(pNow); return change; }
bool KdShaderManager::SetPSConstantBuffer(int startSlot, ID3D11Buffer* const* pSetPSBuffer) { if (!pSetPSBuffer) return false; ID3D11Buffer* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->PSGetConstantBuffers(startSlot, 1, &pNow); bool change = pNow != *pSetPSBuffer; if (change) KdDirect3D::Instance().WorkDevContext()->PSSetConstantBuffers(startSlot, 1, pSetPSBuffer); KdSafeRelease(pNow); return change; }

void KdShaderManager::ChangeDepthStencilState(KdDepthStencilState stateId) { ID3D11DepthStencilState* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->OMGetDepthStencilState(&pNow, 0); if (pNow != m_depthStencilStates[(int)stateId]) KdDirect3D::Instance().WorkDevContext()->OMSetDepthStencilState(m_depthStencilStates[(int)stateId], 0); m_ds_Undo.push(pNow); KdSafeRelease(pNow); }
void KdShaderManager::UndoDepthStencilState() { if (!m_ds_Undo.size()) return; ID3D11DepthStencilState* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->OMGetDepthStencilState(&pNow, 0); if (pNow != m_ds_Undo.top()) KdDirect3D::Instance().WorkDevContext()->OMSetDepthStencilState(m_ds_Undo.top(), 0); m_ds_Undo.pop(); KdSafeRelease(pNow); }
void KdShaderManager::ChangeRasterizerState(KdRasterizerState stateId) { ID3D11RasterizerState* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->RSGetState(&pNow); if (pNow != m_rasterizerStates[(int)stateId]) KdDirect3D::Instance().WorkDevContext()->RSSetState(m_rasterizerStates[(int)stateId]); m_rs_Undo.push(pNow); KdSafeRelease(pNow); }
void KdShaderManager::UndoRasterizerState() { if (!m_rs_Undo.size()) return; ID3D11RasterizerState* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->RSGetState(&pNow); if (pNow != m_rs_Undo.top()) KdDirect3D::Instance().WorkDevContext()->RSSetState(m_rs_Undo.top()); m_rs_Undo.pop(); KdSafeRelease(pNow); }
void KdShaderManager::ChangeBlendState(KdBlendState stateId) { ID3D11BlendState* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->OMGetBlendState(&pNow, nullptr, nullptr); if (pNow != m_blendStates[(int)stateId]) KdDirect3D::Instance().WorkDevContext()->OMSetBlendState(m_blendStates[(int)stateId], Math::Color(0, 0, 0, 0), 0xFFFFFFFF); m_bs_Undo.push(pNow); KdSafeRelease(pNow); }
void KdShaderManager::UndoBlendState() { if (!m_bs_Undo.size()) return; ID3D11BlendState* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->OMGetBlendState(&pNow, nullptr, nullptr); if (pNow != m_bs_Undo.top()) KdDirect3D::Instance().WorkDevContext()->OMSetBlendState(m_bs_Undo.top(), Math::Color(0, 0, 0, 0), 0xFFFFFFFF); m_bs_Undo.pop(); KdSafeRelease(pNow); }
void KdShaderManager::ChangeSamplerState(KdSamplerState stateId, int slot) { ID3D11SamplerState* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->PSGetSamplers(slot, 1, &pNow); if (pNow != m_samplerStates[(int)stateId]) KdDirect3D::Instance().WorkDevContext()->PSSetSamplers(slot, 1, &m_samplerStates[(int)stateId]); m_ss_Undo.push(pNow); KdSafeRelease(pNow); }
void KdShaderManager::UndoSamplerState(int slot) { if (!m_ss_Undo.size()) return; ID3D11SamplerState* pNow = nullptr; KdDirect3D::Instance().WorkDevContext()->PSGetSamplers(0, 1, &pNow); if (pNow != m_ss_Undo.top()) KdDirect3D::Instance().WorkDevContext()->PSSetSamplers(slot, 1, &m_ss_Undo.top()); m_ss_Undo.pop(); KdSafeRelease(pNow); }

// データ転送処理
void KdShaderManager::WriteCBCamera(const Math::Matrix& cam, const Math::Matrix& proj) { cbCamera& c = m_cb7_Camera.Work(); c.mView = cam.Invert(); c.mProj = proj; c.mProjInv = proj.Invert(); c.CamPos = cam.Translation(); m_cb7_Camera.Write(); }
void KdShaderManager::WriteCBFogEnable(bool distance, bool height) { cbFog& f = m_cb8_Fog.Work(); f.DistanceFogEnable = distance; f.HeightFogEnable = height; m_cb8_Fog.Write(); }
void KdShaderManager::WriteCBDistanceFog(const Math::Vector3& col, float density) { cbFog& f = m_cb8_Fog.Work(); f.DistanceFogColor = col; f.DistanceFogDensity = density; m_cb8_Fog.Write(); }
void KdShaderManager::WriteCBHeightFog(const Math::Vector3& col, float top, float bottom, float beginDistance) { cbFog& f = m_cb8_Fog.Work(); f.HeightFogColor = col; f.HeightFogTopValue = top; f.HeightFogBottomValue = bottom; f.HeightFogBeginDistance = beginDistance; m_cb8_Fog.Write(); }
void KdShaderManager::WriteCBAmbientLight(const Math::Vector4& col) { m_cb9_Light.Work().AmbientLight = col; m_cb9_Light.Write(); }
void KdShaderManager::WriteCBDirectionalLight(const Math::Vector3& dir, const Math::Vector3& col) { cbLight& l = m_cb9_Light.Work(); l.DirLight_Dir = dir; l.DirLight_Color = col; m_cb9_Light.Write(); }
void KdShaderManager::WriteCBShadowArea(const Math::Matrix& proj, float dirLightHeight) { Math::Vector3 lightDir = m_cb9_Light.Get().DirLight_Dir; Math::Vector3 lightPos = m_cb7_Camera.Get().CamPos; Math::Vector3 upVec = (lightDir == Math::Vector3::Up) ? Math::Vector3::Right : Math::Vector3::Up; Math::Matrix shadowVP = DirectX::XMMatrixLookAtLH(lightPos - lightDir * dirLightHeight, lightPos, upVec); shadowVP *= proj; m_cb9_Light.Work().DirLight_mVP = shadowVP; m_cb9_Light.Write(); }
void KdShaderManager::WriteCBPointLight(const std::list<PointLight>& pointLights) { cbLight& l = m_cb9_Light.Work(); l.PointLight_Num = (signed)pointLights.size(); UINT idx = 0; for (const PointLight& pl : pointLights) { l.PointLights[idx] = pl; ++idx; } m_cb9_Light.Write(); }

// カラースフィア（赤色バリア）用転送関数 (b10)
void KdShaderManager::WriteCBColorEnable(bool enable)
{
	m_cb10_Effect.Work().colorEnable = enable ? 1 : 0;
	m_cb10_Effect.Write();
}

void KdShaderManager::WriteCBColor(const std::vector<EffectSphere>& sphereList)
{
	cbEffect& cb = m_cb10_Effect.Work();
	cb.colorEnable = 1;
	cb.colorCount = static_cast<int>((std::min)(sphereList.size(), static_cast<size_t>(kMaxEffectSphereCount)));

	for (int i = 0; i < cb.colorCount; ++i)
	{
		cb.list[i] = sphereList[i];
	}

	m_cb10_Effect.Write();
}

// 単体コーンライト用転送関数 (b11経由で転送)
void KdShaderManager::WriteCBConeLightEnable(bool enable)
{
	cbLight& light = m_cb9_Light.Work();
	light.ConeLight_Enable = enable ? 1 : 0;
	m_cb9_Light.Write();
}

void KdShaderManager::WriteCBConeLight2D(
	const Math::Vector3& pos,
	const Math::Vector3& dir,
	float angle,
	float range,
	const Math::Vector3& col,
	float intensity,
	float edgeSmoothness,
	bool isHit,
	const Math::Vector3& hitPos,
	const Math::Vector3& hitNormal)
{
	ConeLight light;
	light.Pos = pos;
	light.Dir = dir;
	light.Angle = angle;
	light.Range = range;
	light.Color = col;
	light.Intensity = intensity;
	light.EdgeSmoothness = edgeSmoothness;
	light.IsHit = isHit ? 1 : 0;
	light.HitPos = hitPos;
	light.HitNormal = hitNormal;

	WriteCBConeLight(std::vector<ConeLight>{ light });
}

void KdShaderManager::WriteCBConeLight(const ConeLight& coneLight)
{
	WriteCBConeLight(std::vector<ConeLight>{ coneLight });
}

// 複数コーンライト用転送関数 (b11)
void KdShaderManager::WriteCBConeLight(const std::vector<ConeLight>& coneLights)
{
	cbConeLight& cb = m_cb11_ConeLight.Work();
	cb.count = static_cast<int>((std::min)(coneLights.size(), static_cast<size_t>(kMaxConeLightCount)));

	for (int i = 0; i < cb.count; ++i)
	{
		cb.list[i] = coneLights[i];
	}

	m_cb11_ConeLight.Write();
}

#pragma warning(disable:4239)
void KdShaderManager::Release()
{
	m_StandardShader.Release();
	m_postProcessShader.Release();
	m_spriteShader.Release();

	m_cb7_Camera.Release();
	m_cb8_Fog.Release();
	m_cb9_Light.Release();
	m_cb10_Effect.Release();
	m_cb11_ConeLight.Release();

	for (auto& state : m_depthStencilStates) KdSafeRelease(state);
	for (auto& state : m_rasterizerStates) KdSafeRelease(state);
	for (auto& state : m_blendStates) KdSafeRelease(state);
	for (auto& state : m_samplerStates) KdSafeRelease(state);
}
#pragma warning(default:4239)