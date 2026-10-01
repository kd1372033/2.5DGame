#include "inc_KdStandardShader.hlsli"
#include "../inc_KdCommon.hlsli"

// モデル描画用テクスチャ
Texture2D g_baseTex : register(t0); // ベースカラーテクスチャ
Texture2D g_metalRoughTex : register(t1); // メタリック/ラフネステクスチャ
Texture2D g_emissiveTex : register(t2); // 発光テクスチャ
Texture2D g_normalTex : register(t3); // 法線マップ

// 特殊処理用テクスチャ
Texture2D g_dirShadowMap : register(t10); // 平行光シャドウマップ
Texture2D g_dissolveTex : register(t11); // ディゾルブマップ
Texture2D g_environmentTex : register(t12); // 反射景マップ

// サンプラ
SamplerState g_ss : register(s0); // 通常のテクスチャ描画用
SamplerComparisonState g_ssCmp : register(s1); // 補間用比較機能付き

float BlinnPhong(float3 lightDir, float3 vCam, float3 normal, float specPower)
{
	float3 H = normalize(-lightDir + vCam);
	float NdotH = saturate(dot(normal, H)); // カメラの角度差(0～1)
	float spec = pow(NdotH, specPower);

	// 正規化Blinn-Phong
	return spec * ((specPower + 2) / (2 * 3.1415926535));
}

//================================
// ピクセルシェーダ
//================================
float4 main(VSOutput In) : SV_Target0
{
	// ディゾルブによる描画スキップ
	float discardValue = g_dissolveTex.Sample(g_ss, In.UV).r;
	if (discardValue < g_dissolveValue)
	{
		discard;
	}
	
	//------------------------------------------
	// 材質色
	//------------------------------------------
	float4 baseColor = g_baseTex.Sample(g_ss, In.UV) * g_BaseColor * In.Color;
	
	// Alphaテスト
	if (baseColor.a < 0.05f)
	{
		discard;
	}
	
	// カメラへの方向
	float3 vCam = g_CamPos - In.wPos;
	float camDist = length(vCam); // カメラ - ピクセル距離
	vCam = normalize(vCam);

	// 法線マップから法線ベクトル取得
	float3 wN = g_normalTex.Sample(g_ss, In.UV).rgb;

	// UV座標（0～1）から 射影座標（-1～1）へ変換
	wN = wN * 2.0 - 1.0;
	
	{
		// 3種の法線から法線行列を作成
		row_major float3x3 mTBN =
		{
			normalize(In.wT),
			normalize(In.wB),
			normalize(In.wN),
		};
	
		// 法線ベクトルをこのピクセル空間へ変換
		wN = mul(wN, mTBN);
	}

	// 法線正規化
	wN = normalize(wN);

	float4 mr = g_metalRoughTex.Sample(g_ss, In.UV);
	// 金属性
	float metallic = mr.b * g_Metallic;
	// 粗さ
	float roughness = mr.g * g_Roughness;
	// ラフネスを逆転させ「滑らか」さにする
	float smoothness = 1.0 - roughness;
	float specPower = pow(2, 11 * smoothness); // 1～2048
	
	//------------------------------------------
	// ライティング
	//------------------------------------------
	// 最終的な色
	float3 outColor = 0;
	
	// 材質の拡散色 非金属ほど材質の色になり、金属ほど拡散色は無くなる
	const float3 baseDiffuse = lerp(baseColor.rgb, float3(0, 0, 0), metallic);
	// 材質の反射色 非金属ほど光の色をそのまま反射し、金属ほど材質の色が乗る
	const float3 baseSpecular = lerp(0.04, baseColor.rgb, metallic);

	//-------------------------------
	// シャドウマッピング(影判定)
	//-------------------------------
	float shadow = 1;

	// ピクセルの3D座標から、DepthMapFromLight空間へ変換
	float4 liPos = mul(float4(In.wPos, 1), g_DL_mLightVP);
	liPos.xyz /= liPos.w;

	// 深度マップの範囲内？
	if (abs(liPos.x) <= 1 && abs(liPos.y) <= 1 && liPos.z <= 1)
	{
		// 射影座標 -> UV座標へ変換
		float2 uv = liPos.xy * float2(1, -1) * 0.5 + 0.5;
		// ライトカメラからの距離
		float z = liPos.z - 0.004; // シャドウアクネ対策
		
		// 画像のサイズからテクセルサイズを求める
		float w, h;
		g_dirShadowMap.GetDimensions(w, h);
		float tw = 1.0 / w;
		float th = 1.0 / h;
	
		// uvの周辺3x3も判定し、平均値を求める
		shadow = 0;
		for (int y = -1; y <= 1; y++)
		{
			for (int x = -1; x <= 1; x++)
			{
				shadow += g_dirShadowMap.SampleCmpLevelZero(g_ssCmp, uv + float2(x * tw, y * th), z);
			}
		}
		shadow *= 0.11;
	}
		
	//-------------------------
	// 平行光
	//-------------------------
	// Diffuse(拡散光)
	{
		// 光の方向と法線の方向との角度さが光の強さになる
		float lightDiffuse = dot(-g_DL_Dir, wN);
		lightDiffuse = saturate(lightDiffuse); // マイナス値は0にする 0(暗)～1(明)になる

		// 正規化Lambert
		lightDiffuse /= 3.1415926535;

		// 光の色 * 材質の拡散色 * 透明率
		outColor += (g_DL_Color * lightDiffuse) * baseDiffuse * baseColor.a * shadow;
	}

	// Specular(反射色)
	{
		// 反射した光の強さを求める
		// Blinn-Phong NDF
		float spec = BlinnPhong(g_DL_Dir, vCam, wN, specPower);

		// 光の色 * 反射光の強さ * 材質の反射色 * 透明率 * 適当な調整値
		outColor += (g_DL_Color * spec) * baseSpecular * baseColor.a * 0.5 * shadow;
	}

	// 全体の明度：環境光に1が設定されている場合は影響なし
	// 環境光の不透明度を下げる事により、明度ライトの周り以外は描画されなくなる
	float totalBrightness = g_AmbientLight.a;

	//-------------------------
	// 点光
	//-------------------------
	for (int i = 0; i < g_PointLightNum.x; i++)
	{
		// ピクセルから点光への方向
		float3 dir = g_PointLights[i].Pos - In.wPos;
		
		// 距離を算出
		float dist = length(dir);
		
		// 正規化
		dir /= dist;
		
		// 点光の判定以内
		if (dist < g_PointLights[i].Radius)
		{
			// 半径をもとに、距離の比率を求める
			float atte = 1.0 - saturate(dist / g_PointLights[i].Radius);
			
			// 明度の追加
			totalBrightness += (1 - pow(1 - atte, 2)) * g_PointLights[i].IsBright;
			
			// 逆２乗の法則
			atte *= atte;
			
			// Diffuse(拡散光)
			{
				// 光の方向と法線の方向との角度さが光の強さになる
				float lightDiffuse = dot(dir, wN);
				lightDiffuse = saturate(lightDiffuse); // マイナス値は0にする 0(暗)～1(明)になる

				lightDiffuse *= atte; // 減衰

				// 正規化Lambert
				lightDiffuse /= 3.1415926535;

				// 光の色 * 材質の拡散色 * 透明率
				outColor += (g_PointLights[i].Color * lightDiffuse) * baseDiffuse * baseColor.a;
			}

			// Specular(反射色)
			{
				// 反射した光の強さを求める
				// Blinn-Phong NDF
				float spec = BlinnPhong(-dir, vCam, wN, specPower);

				spec *= atte; // 減衰
				
				// 光の色 * 反射光の強さ * 材質の反射色 * 透明率 * 適当な調整値
				outColor += (g_PointLights[i].Color * spec) * baseSpecular * baseColor.a * 0.5;
			}
		}
	}

	//-------------------------
	// コーンライト（扇状光と壁面楕円プロジェクションの完全分離版）
	//-------------------------
	// 1. コーンライトの機能自体が有効になっているか判定
	if (g_ConeLight.Enable)
	{
		// 2. 現在描画しようとしているピクセル(In.wPos)からライト位置(Pos)への相対ベクトルを計算
		float3 lightToPos = In.wPos - g_ConeLight.Pos;

		// 3. ライト位置から現在ピクセルまでの直線距離を計算
		float dist = length(lightToPos);

		// 4. 距離がライトの有効照射範囲(Range)内で、かつ0除算を防ぐため微小値より大きいか判定
		if (dist < g_ConeLight.Range && dist > 0.0001f)
		{
			// 5. 自機(戦車)への映り込みを防ぐため、ライト起点から0.5単位以上離れているピクセルのみ許可
			bool isNotSelf = (dist > 0.5f);

			// 6. 壁の裏側判定用フラグ（初期値は表側）
			bool isFrontOfWall = true;

			// 7. レイキャスト等で壁にライトが当たっている(IsHit)場合 
			if (g_ConeLight.IsHit)
			{
				// 8. 壁への着弾点(HitPos)から現在ピクセルへの相対ベクトルを計算 
				float3 hitToPos = In.wPos - g_ConeLight.HitPos;

				// 9. 壁の法線(HitNormal)との内積をとり、ピクセルが壁の裏側に落ち込んでいないか判定 
				if (dot(hitToPos, g_ConeLight.HitNormal) < -0.05f) // 9/24修正 
				{
					// 10. 裏側にある場合は描画対象外（フラグをfalseに） 
					isFrontOfWall = false; // 9/24修正 
				}
			}

			// 11. 壁の表側であり、かつ自機モデル自身でない場合のみ光の計算を続行 
			if (isFrontOfWall && isNotSelf) // 9/24修正 
			{
				// 12. ライトから現在ピクセルへの方向ベクトルを正規化（長さ1のベクトルに）
				float3 lightDir = lightToPos / dist;

				// 13. ライトの照射軸(Dir)と、現在ピクセルへの方向(lightDir)のなす角の余弦(cos)を取得
				float cosAngle = dot(lightDir, g_ConeLight.Dir);

				// 14. 設定されたコーンライト照射角度(Angle)の余弦(cos)を取得
				float coneCos = cos(g_ConeLight.Angle);

				// 15. 現在の描画対象ピクセルが「壁の表面」にあるかどうかを判定するフラグ
				bool isWallPixel = false;

				// 16. 壁への着弾が有効な場合
				if (g_ConeLight.IsHit)
				{
					// 17. 現在ピクセルの法線(wN)と壁の法線(HitNormal)が一致している度合いを計算
					float isWallFace = saturate(dot(wN, g_ConeLight.HitNormal));

					// 18. 壁の表面（同じ向きの面）であると判定された場合
					if (isWallFace > 0.01f)
					{
						// 19. 壁面ピクセル判定フラグを有効化
						isWallPixel = true;
					}
				}

				// =========================================================
				// パターンA: 地面など壁以外の描画（従来の角度制限付きコーン光）
				// =========================================================
				// 20. 現在のピクセルが壁面ではない場合のみ実行
				if (!isWallPixel)
				{
					// 21. ピクセルがライトの設定角度(Angle)の内側にある場合のみ処理（ここで角度カットされる）
					if (cosAngle > coneCos)
					{
						// 22. ピクセルの法線(wN)と光の逆向きベクトルの内積で、表面に光が当たる角度を計算
						float NdotL = saturate(dot(wN, -lightDir));

						// 23. 光が表側に当たっている場合
						if (NdotL > 0.0f)
						{
							// 24. ライトの照射軸に沿った距離（平面距離）を計算
							float planeDist = dot(lightToPos, g_ConeLight.Dir);

							// 25. 距離による明るさの減衰（遠いほど0に近づく）
							float atte = saturate(1.0f - (planeDist / g_ConeLight.Range));

							// 26. コーンの外周境界に向かって滑らかにぼかすためのフェード率を計算
							float angleFade = saturate((cosAngle - coneCos) / (1.0f - coneCos));

							// 27. 減衰率にフェード率を乗算
							atte *= angleFade;

							// 28. 拡散光(Diffuse)強度を計算
							float lightDiffuse = NdotL * atte;

							// 29. 拡散光の色を最終出力カラーに加算
							outColor += (g_ConeLight.Color * lightDiffuse) * baseDiffuse * baseColor.a;

							// 30. 鏡面反射光(Specular)をBlinn-Phongモデルで計算
							float spec = BlinnPhong(-lightDir, vCam, wN, specPower);

							// 31. 反射光に減衰率を乗算
							spec *= atte;

							// 32. 反射光の色を最終出力カラーに加算
							outColor += (g_ConeLight.Color * spec) * baseSpecular * baseColor.a;
						}
					}
				}
				// =========================================================
				// パターンB: 壁面の描画（角度制限を完全解除し、綺麗な楕円を描画）
				// =========================================================
				// 33. 現在のピクセルが壁面である場合実行
				else
				{
					// 34. 壁の表面である度合い(0～1)を取得 
					float isWallFace = saturate(dot(wN, g_ConeLight.HitNormal)); // 9/24修正 

					// 35. 着弾点(HitPos)から現在ピクセルまでの相対ベクトルを計算 
					float3 hitToPos = In.wPos - g_ConeLight.HitPos; // 9/24修正 

					// 36. 壁の法線方向の垂直成分を取り除き、壁平面上だけの移動ベクトル(wallProj)を算出 
					float3 wallProj = hitToPos - g_ConeLight.HitNormal * dot(hitToPos, g_ConeLight.HitNormal); // 9/24修正 

					// 37. ライト位置から着弾点までの直線距離を計算 
					float hitDist = length(g_ConeLight.HitPos - g_ConeLight.Pos); // 9/24修正 

					// 38. 着弾点における正円状態の基準半径(距離 * tan(照射角))を計算 
					float baseRadius = hitDist * tan(g_ConeLight.Angle); // 9/24修正 

					// 39. ライトの照射方向と壁法線の角度から、斜め衝突時の「引き伸ばし倍率」を計算 
					float cosHit = saturate(dot(-g_ConeLight.Dir, g_ConeLight.HitNormal)); // 9/24修正 
					float stretch = 1.0f / max(cosHit, 0.55f); // 9/24修正 

					// 40. 着弾点から現在ピクセルまでの「壁平面上での距離」を算出 
					float distOnWall = length(wallProj); // 9/24修正 

					// 41. ライトの照射方向を壁平面上に投影し、光が傾いている「伸びる軸方向」のベクトルを取得 
					float3 rawDirOnWall = g_ConeLight.Dir - g_ConeLight.HitNormal * dot(g_ConeLight.Dir, g_ConeLight.HitNormal); // 9/24修正 
					float dirOnWallLen = length(rawDirOnWall); // 9/24修正 

					// 42. 現在ピクセルの方向が、光の伸長軸方向とどれくらい一致しているか(0～1)を計算
					float effectiveRadius = baseRadius; // 9/24修正 
					if (dirOnWallLen > 0.001f) // 9/24修正 
					{
						float3 lightDirOnWall = rawDirOnWall / dirOnWallLen; // 9/24修正 
						float align = abs(dot(normalize(wallProj + 0.0001f), lightDirOnWall)); // 9/24修正 

						// 43. 一致度に応じて、伸長軸方向のみ半径を拡大させた有効半径(effectiveRadius)を決定 
						effectiveRadius = baseRadius * lerp(1.0f, stretch, align); // 9/24修正 
					}

					// 44. ピクセルが計算された楕円の有効半径内に収まっているか判定 
					if (distOnWall < effectiveRadius) // 9/24修正 
					{
						// 45. 中心(HitPos)からの距離比率(0.0～1.0)を算出 
						float rate = distOnWall / effectiveRadius;

						// 46. 中心から外周に向かって滑らかにフェードアウトする強度を算出 
						float spotIntensity = 1.0f - smoothstep(0.1f, 1.0f, rate); // 9/24修正 

						// 47. ライト光源からの距離減衰を計算 
						float atte = saturate(1.0f - (dist / g_ConeLight.Range)); // 9/24修正 

						// 48. 角度カットの制限を受けない綺麗で滑らかな楕円スポット光を最終カラーに加算 
						outColor += g_ConeLight.Color * spotIntensity * isWallFace * 2.0f * atte; // 9/24修正 
					}
				}
			}
		}
	}

	outColor += g_AmbientLight.rgb * baseColor.rgb * baseColor.a;
	
	// 自己発光色の適応
	if (g_OnlyEmissie)
	{
		outColor = g_emissiveTex.Sample(g_ss, In.UV).rgb * g_Emissive * In.Color.rgb;
	}
	else
	{
		outColor += g_emissiveTex.Sample(g_ss, In.UV).rgb * g_Emissive * In.Color.rgb;
	}
	
	//------------------------------------------
	// 高さフォグ
	//------------------------------------------
	if (g_HeightFogEnable && g_FogEnable)
	{
		if (In.wPos.y < g_HeightFogTopValue)
		{
			float distRate = length(In.wPos - g_CamPos);
			distRate = saturate(distRate / g_HeightFogDistance);
			distRate = pow(distRate, 2.0);
			
			float heightRange = g_HeightFogTopValue - g_HeightFogBottomValue;
			float heightRate = 1 - saturate((In.wPos.y - g_HeightFogBottomValue) / heightRange);
			
			float fogRate = heightRate * distRate;
			outColor = lerp(outColor, g_HeightFogColor, fogRate);
		}
	}
	
	//------------------------------------------
	// 距離フォグ
	//------------------------------------------
	if (g_DistanceFogEnable && g_FogEnable)
	{
		// フォグ 1(近い)～0(遠い)
		float f = saturate(1.0 / exp(camDist * g_DistanceFogDensity));
		
		// 適用
		outColor = lerp(g_DistanceFogColor, outColor, f);
	}
	
	// ディゾルブ輪郭発光
	if (g_dissolveValue > 0)
	{
		// 閾値とマスク値の差分で、縁を検出
		if (abs(discardValue - g_dissolveValue) < g_dissolveEdgeRange)
		{
			// 輪郭に発光色追加
			outColor += g_dissolveEmissive;
		}
	}
	
	totalBrightness = saturate(totalBrightness);
	outColor *= totalBrightness;
	
	//------------------------------------------
	// 出力
	//------------------------------------------
	return float4(outColor, baseColor.a);
}
