/* 
* Project : iZ3D Stereo Driver
* Copyright (C) iZ3D Inc. 2002 - 2010
*/

#pragma once

template <typename T, typename Q>
ShaderPipelineData<T, Q>::ShaderPipelineData()
{
	m_CurrentShader = NULL;
	m_CurrentStereoShader = NULL;
	m_MaxTextures = 0;
	m_StereoTexturesMask = 0;
	m_pShaderFunctionOpCode = NULL;
	m_ShaderFunctionSize = 0;
	m_bShaderDataAvailable = FALSE;
	m_ShaderVersion = 0;
	m_pProfile = NULL;
}

template <typename T, typename Q>
ShaderPipelineData<T, Q>::~ShaderPipelineData()
{
	if (m_pShaderFunctionOpCode)
		delete [] m_pShaderFunctionOpCode;
	m_pShaderFunctionOpCode = NULL;
	m_ShaderFunctionSize = 0;
	m_ShadersList.clear();
}

template <typename T, typename Q>
void ShaderPipelineData<T, Q>::Init(DWORD textures)
{
	m_pShaderFunctionOpCode = NULL;
	m_ShaderFunctionSize = 0;
	ZeroMemory(&m_CurrentShaderData, sizeof(T));
	m_CurrentShaderData.textures = textures;
	m_bShaderDataAvailable = false;
	m_ShaderVersion = 0;
	m_pProfile = NULL;
}

template <typename T, typename Q>
void ShaderPipelineData<T, Q>::Clear()
{
	m_StereoTexturesMask = 0;
	for(DWORD i=0; i< m_MaxTextures; i++)
	{
		m_Texture[i].m_pLeft = NULL;
		m_Texture[i].m_pRight.Release();
		m_Texture[i].m_pWideTexture = NULL;
	}
	if (m_pShaderFunctionOpCode)
		delete [] m_pShaderFunctionOpCode;
	m_pShaderFunctionOpCode = NULL;
	m_ShaderFunctionSize = 0;
	m_CurrentShader = NULL;
	m_CurrentStereoShader = NULL;
	m_pProfile = NULL;
}

template <typename T, typename Q>
bool ShaderPipelineData<T, Q>::IsTextureUsed( int index )
{
	if ((m_CurrentShaderData.textures >> index) & 1)
		return true;
	return false;
}

template <typename T, typename Q>
void ShaderPipelineData<T, Q>::SetMaxTextures( DWORD maxTextures )
{
	_ASSERT(maxTextures <= sizeof(DWORD) * 8);
	m_MaxTextures = maxTextures;
	m_Texture.resize(maxTextures, TexData());
	m_TextureCRCMultiplier.assign(maxTextures, NULL);
}

template <typename T, typename Q>
BOOL ShaderPipelineData<T, Q>::GetShaderPrivateData(Q* shader)
{
	_ASSERT(shader);

	DEBUG_TRACE3(_T("GetShaderPrivateData\n"));
	UINT sizeOfData = 0;
	if(SUCCEEDED(shader->GetFunction(NULL, &sizeOfData)))
	{
		if (m_ShaderFunctionSize < sizeOfData)
		{
			if (m_pShaderFunctionOpCode)
				delete [] m_pShaderFunctionOpCode;
			m_ShaderFunctionSize = sizeOfData;
			m_pShaderFunctionOpCode = DNew BYTE[m_ShaderFunctionSize];
		}
		DWORD* p = (DWORD*)&m_pShaderFunctionOpCode[0];
		if(SUCCEEDED(shader->GetFunction(p, &sizeOfData)))
		{
			DWORD	dataSizeInDwords = m_CurrentShaderData.GetDataSizeInDwords();
			if(p[1] == ((dataSizeInDwords << 16) | 0xFFFE))
			{
				memcpy(&m_CurrentShaderData, (p+2), sizeof(T));

				// Foreign-shader guard: 0xFFFE is the DX9 shader-bytecode COMMENT
				// opcode used by CTAB and other tools, not just our metadata. SR's
				// LeiaSR/Dimenco weaver vertex shader has a CTAB comment whose
				// size happens to match sizeof(SHADER_PRIVATE_DATA)/4, so the
				// magic check above passes but the bytes we memcpy'd are CTAB
				// data, not our struct — the `stereoShader` field becomes a
				// garbage pointer that crashes real d3d9 when SetVertexShader
				// passes it on (Tales of Berseria / FFIX x64, 2026-05-11).
				// Validate stereoShader is actually a wrapper-tracked shader
				// (every shader we create is pushed into m_ShadersList in
				// BaseStereoRenderer_Modified.cpp); if it isn't, this is a
				// coincidental bytecode match — pass through as a foreign shader.
				if (m_CurrentShaderData.stereoShader)
				{
					bool tracked = false;
					for (size_t i = 0, n = m_ShadersList.size(); i < n; ++i)
					{
						if (m_ShadersList[i].m_T.p == m_CurrentShaderData.stereoShader)
						{
							tracked = true;
							break;
						}
					}
					if (!tracked)
					{
						m_CurrentShaderData.Clear();
						m_bShaderDataAvailable = FALSE;
						m_ShaderVersion = 0;
						return FALSE;
					}
				}

				m_bShaderDataAvailable = TRUE;
				m_ShaderVersion = D3DXGetShaderVersion(p);
				return TRUE;
			}
		}
	}
	m_ShaderVersion = 0;
	m_bShaderDataAvailable = FALSE;
	return FALSE;
}

template <class T, class Q>
float ShaderPipelineData<T, Q>::GetMultiplayerFromTextures(float& convergenceShift)
{
	float multiplayer = 1.0f;
	for (DWORD i = 0; i < m_MaxTextures; i++) 
	{
		if (m_TextureCRCMultiplier[i])
		{
			if (m_TextureCRCMultiplier[i]->second.m_Multiplier != 1.f)
			{
				multiplayer = m_TextureCRCMultiplier[i]->second.m_Multiplier;
			}
			if (m_TextureCRCMultiplier[i]->second.m_ConvergenceShift != 0.f)
			{
				convergenceShift = m_TextureCRCMultiplier[i]->second.m_ConvergenceShift;
			}
		}
	}
	return multiplayer;
}

template <class T, class Q>
const ShaderProfileData* ShaderPipelineData<T, Q>::FindProfile( ShaderProfileDataMap &map )
{
	if (map.size() > 0) 
	{
		ShaderProfileDataMap::const_iterator i = map.find(m_CurrentShaderData.CRC32);
		if (i != map.end())
		{
			DEBUG_TRACE3(_T("ShaderCRC = 0x%X Multiplier = %f detected\n"), 
				i->first, i->second.m_Multiplier);
			return &i->second;
		}
		else
		{
			i = map.find(0);
			if (i != map.end())
			{
				DEBUG_TRACE3(_T("ShaderCRC = 0x%X Multiplier = %f detected\n"), 
					0, i->second.m_Multiplier);
				return &i->second;
			}
		}
	}
	return NULL;
}

template <class T, class Q>
Q* ShaderPipelineData<T, Q>::SetShader(Q* pShader, BOOL bStereoActive, ShaderProfileDataMap& map, DWORD defTextures)
{
	if (m_CurrentShader == pShader) {
		return m_CurrentStereoShader;
	}

	m_CurrentShader = pShader;
	m_CurrentStereoShader = pShader;
	m_pProfile = NULL;
	if (pShader)
	{
		if (GetShaderPrivateData(pShader))
		{
			m_pProfile = FindProfile(map);

			if (m_CurrentShaderData.stereoShader)
			{
				if (bStereoActive)
					m_CurrentStereoShader = m_CurrentShaderData.stereoShader;
				DEBUG_TRACE3(_T("Using modified shader (CRC 0x%X).\n"), m_CurrentShaderData.CRC32);
			}
		}
	}
	else
		m_bShaderDataAvailable = false;
	
	if (!m_bShaderDataAvailable)
		m_CurrentShaderData.textures = defTextures;
	
	return m_CurrentStereoShader;
}