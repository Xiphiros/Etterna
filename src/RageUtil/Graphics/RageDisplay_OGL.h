/* RageDisplay_Legacy: OpenGL renderer. */

#ifndef RAGE_DISPLAY_OGL_H
#define RAGE_DISPLAY_OGL_H

#include "Etterna/Models/Misc/DisplaySpec.h"
#include "RageDisplay.h"
#include "Etterna/Actor/Base/Sprite.h"
#include "RageUtil/Graphics/RageTextureRenderTarget.h"

#include <unordered_map>

/* Making an OpenGL call doesn't also flush the error state; if we happen
 * to have an error from a previous call, then the assert below will fail.
 * Flush it. */
#define FlushGLErrors()                                                        \
	do {                                                                       \
		if (unlikely(DISPLAY != nullptr && DISPLAY->IsBenchRecording())) {     \
			DISPLAY->BenchIncGLGetQueries();                                   \
		}                                                                      \
	} while (glGetError() != GL_NO_ERROR)
#define AssertNoGLError()                                                      \
                                                                               \
	{                                                                          \
		if (unlikely(DISPLAY != nullptr && DISPLAY->IsBenchRecording())) {     \
			DISPLAY->BenchIncGLGetQueries();                                   \
		}                                                                      \
		GLenum error = glGetError();                                           \
		ASSERT_M(error == GL_NO_ERROR,                                         \
				 RageDisplay_Legacy_Helpers::GLToString(error));               \
	}

#if defined(DEBUG) || !defined(GL_GET_ERROR_IS_SLOW)
#define DebugFlushGLErrors()                                                   \
	do {                                                                       \
		if (unlikely(DISPLAY != nullptr &&                                     \
					 DISPLAY->DisplayDebugModeEnabled())) {                    \
			FlushGLErrors();                                                   \
		}                                                                      \
	} while (false)
#define DebugAssertNoGLError()                                                 \
	do {                                                                       \
		if (unlikely(DISPLAY != nullptr &&                                     \
					 DISPLAY->DisplayDebugModeEnabled())) {                    \
			AssertNoGLError();                                                 \
		}                                                                      \
	} while (false)
#else
#define DebugFlushGLErrors()
#define DebugAssertNoGLError()
#endif

class RageDisplay_Legacy : public RageDisplay
{
  public:
	RageDisplay_Legacy();
	~RageDisplay_Legacy() override;
	std::string Init(VideoModeParams&& p,
					 bool bAllowUnacceleratedRenderer) override;

	std::string GetApiDescription() const override { return "OpenGL"; }
	void GetDisplaySpecs(DisplaySpecs& out) const override;
	void ResolutionChanged() override;
	const RagePixelFormatDesc* GetPixelFormatDesc(
	  RagePixelFormat pf) const override;

	bool SupportsThreadedRendering() override;
	void BeginConcurrentRenderingMainThread() override;
	void EndConcurrentRenderingMainThread() override;
	void BeginConcurrentRendering() override;
	void EndConcurrentRendering() override;

	bool BeginFrame() override;
	void EndFrame() override;
	const ActualVideoModeParams* GetActualVideoModeParams() const override;
	void SetBlendMode(BlendMode mode) override;
	bool SupportsTextureFormat(RagePixelFormat pixfmt,
							   bool realtime = false) override;
	bool SupportsPerVertexMatrixScale() override;
	intptr_t CreateTexture(RagePixelFormat pixfmt,
						   RageSurface* img,
						   bool bGenerateMipMaps) override;
	void UpdateTexture(intptr_t iTexHandle,
					   RageSurface* img,
					   int xoffset,
					   int yoffset,
					   int width,
					   int height) override;
	void DeleteTexture(intptr_t iTexHandle) override;
	bool UseOffscreenRenderTarget();
	RageSurface* GetTexture(intptr_t iTexture) override;
	RageTextureLock* CreateTextureLock() override;

	void ClearAllTextures() override;
	int GetNumTextureUnits() override;
	void SetTexture(TextureUnit tu, intptr_t iTexture) override;
	void SetTextureMode(TextureUnit tu, TextureMode tm) override;
	void SetTextureWrapping(TextureUnit tu, bool b) override;
	int GetMaxTextureSize() const override;
	void SetTextureFiltering(TextureUnit tu, bool b) override;
	void SetEffectMode(EffectMode effect) override;
	bool IsEffectModeSupported(EffectMode effect) override;
	bool SupportsRenderToTexture() const override;
	bool SupportsFullscreenBorderlessWindow() const override;
	bool SupportsNonPowerOfTwoTextures() const override;
	intptr_t CreateRenderTarget(const RenderTargetParam& param,
								int& iTextureWidthOut,
								int& iTextureHeightOut) override;
	intptr_t GetRenderTarget() override;
	void SetRenderTarget(intptr_t iHandle, bool bPreserveTexture) override;
	bool IsZWriteEnabled() const override;
	bool IsZTestEnabled() const override;
	void SetZWrite(bool b) override;
	void SetZBias(float f) override;
	void SetZTestMode(ZTestMode mode) override;
	void ClearZBuffer() override;
	void SetCullMode(CullMode mode) override;
	void SetAlphaTest(bool b) override;
	void SetMaterial(const RageColor& emissive,
					 const RageColor& ambient,
					 const RageColor& diffuse,
					 const RageColor& specular,
					 float shininess) override;
	void SetLighting(bool b) override;
	void SetLightOff(int index) override;
	void SetLightDirectional(int index,
							 const RageColor& ambient,
							 const RageColor& diffuse,
							 const RageColor& specular,
							 const RageVector3& dir) override;

	void SetSphereEnvironmentMapping(TextureUnit tu, bool b) override;
	void SetCelShaded(int stage) override;

	bool IsD3DInternal() override;

	RageCompiledGeometry* CreateCompiledGeometry() override;
	void DeleteCompiledGeometry(RageCompiledGeometry* p) override;

	// hacks for cell-shaded models
	void SetPolygonMode(PolygonMode pm) override;
	void SetLineWidth(float fWidth) override;

	std::string GetTextureDiagnostics(unsigned id) const override;
	void OnContextInvalidated();
	void OnShaderProgramReset()
	{
		m_bEffectModeValid = false;
		m_iCelShadedStage = 0;
	}
	[[nodiscard]] auto IsLightingEnabled() const -> bool
	{
		return m_bCachedLighting;
	}
	[[nodiscard]] auto IsSphereEnvironmentMappingEnabled() const -> bool
	{
		return m_uSphereMapMask != 0;
	}
	[[nodiscard]] auto IsCelShadedEnabled() const -> bool
	{
		return m_iCelShadedStage != 0;
	}

  protected:
	void OnBenchBeginPhase(bool bIsWarmup) override;
	void OnBenchEndPhase() override;

	void DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts) override;
	void DrawQuadStripInternal(const RageSpriteVertex v[],
							   int iNumVerts) override;
	void DrawFanInternal(const RageSpriteVertex v[], int iNumVerts) override;
	void DrawStripInternal(const RageSpriteVertex v[], int iNumVerts) override;
	void DrawTrianglesInternal(const RageSpriteVertex v[],
							   int iNumVerts) override;
	void DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
									  int iMeshIndex) override;
	void DrawLineStripInternal(const RageSpriteVertex v[],
							   int iNumVerts,
							   float LineWidth) override;
	void DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
										int iNumVerts) override;

	std::string TryVideoMode(const VideoModeParams& p,
							 bool& bNewDeviceOut) override;
	RageSurface* CreateScreenshot() override;
	RagePixelFormat GetImgPixelFormat(RageSurface*& img,
									  bool& FreeImg,
									  int width,
									  int height,
									  bool bPalettedTexture);
	bool SupportsSurfaceFormat(RagePixelFormat pixfmt);

	void SendCurrentMatrices();

  private:
	void InvalidateStateCache(bool bFlushBatch = true,
							  bool bResetContextDefaults = false);
	void FlushQuadBatch();
	auto SetTextureUnit(TextureUnit tu) -> bool;
	void SetupVertices(const RageSpriteVertex v[], int iNumVerts);
	void HarvestAvailableGpuQueries();
	void DrainAllGpuQueries();
	auto CaptureBenchScreenshot(const std::string& sVfsPngPath) -> bool;

	struct OGLTexInfo
	{
		int iWidth{ 0 };
		int iHeight{ 0 };
		unsigned int glInternalFormat{ 0 };
		bool bHasMipMaps{ false };
		int minFilter{ 0 };
		int magFilter{ 0 };
		int wrapS{ 0 };
		int wrapT{ 0 };
	};
	std::unordered_map<unsigned int, OGLTexInfo> m_mapTexInfo;
	mutable int m_iMaxTextureSize{ 0 };

	int m_iActiveTexUnit{ -1 };
	unsigned int m_auBoundTex[NUM_TextureUnit]{};
	bool m_abTex2DEnabled[NUM_TextureUnit]{};
	bool m_abTexUnitStateValid[NUM_TextureUnit]{};
	uint32_t m_uEnabledTexUnitMask{ 0 };
	TextureMode m_aCachedTexMode[NUM_TextureUnit]{};
	bool m_abTexModeValid[NUM_TextureUnit]{};
	EffectMode m_CachedEffectMode{ EffectMode_Normal };
	int m_iCachedEffectTexWidth{ -1 };
	bool m_bEffectModeValid{ false };
	BlendMode m_CachedBlendMode{ BLEND_NORMAL };
	bool m_bBlendModeValid{ false };
	bool m_bCachedZWrite{ false };
	bool m_bZWriteValid{ false };
	ZTestMode m_CachedZTestMode{ ZTEST_OFF };
	bool m_bZTestValid{ false };
	float m_fCachedZBias{ 0.0f };
	bool m_bZBiasValid{ false };
	CullMode m_CachedCullMode{ CULL_NONE };
	bool m_bCullModeValid{ false };
	bool m_bCachedAlphaTest{ false };
	bool m_bAlphaTestValid{ false };
	bool m_bCachedLighting{ false };
	bool m_bLightingValid{ false };
	uint32_t m_uSphereMapMask{ 0 };
	int m_iCelShadedStage{ 0 };
	PolygonMode m_CachedPolygonMode{ POLYGON_FILL };
	bool m_bPolygonModeValid{ false };

	bool m_bClientStateValid{ false };
	bool m_bClientTex1Enabled{ false };
	bool m_bClientNormalEnabled{ false };
	std::vector<uint8_t> m_vFallbackRGBA;

	uint64_t m_uUploadedCenteringRev{ 0 };
	uint64_t m_uUploadedProjRev{ 0 };
	bool m_bUploadedInvertY{ false };
	bool m_bProjUploadValid{ false };
	uint64_t m_uUploadedViewRev{ 0 };
	uint64_t m_uUploadedWorldRev{ 0 };
	bool m_bModelViewIsIdentity{ false };
	bool m_bModelViewUploadValid{ false };
	uint64_t m_uUploadedTexRev{ 0 };
	bool m_bTexMatrixUploadedIsIdentity{ false };
	bool m_bTexUploadValid{ false };

	static constexpr size_t kMaxQuadBatchQuads = 2048;
	static constexpr size_t kMaxQuadBatchVerts = kMaxQuadBatchQuads * 4;
	std::vector<RageSpriteVertex> m_vQuadBatch;
	std::vector<uint16_t> m_vQuadIndices;
	RageMatrix m_QuadBatchProjection{};
	uint64_t m_uQuadBatchCenteringRev{ 0 };
	uint64_t m_uQuadBatchProjRev{ 0 };
	bool m_bQuadBatchInvertY{ false };

	static constexpr size_t kGpuTimerRingSize = 8;
	unsigned int m_aGpuTimerQueries[kGpuTimerRingSize]{};
	bool m_bGpuTimerQueriesInitialized{ false };
	size_t m_uGpuRingHead{ 0 };
	size_t m_uGpuRingTail{ 0 };
	size_t m_uGpuRingCount{ 0 };
	bool m_bGpuQueryActive{ false };
	bool m_bBenchFrameActive{ false };
	bool m_bHasPrevFrameEnd{ false };
	std::chrono::steady_clock::time_point m_BenchDrawStart{};
	std::chrono::steady_clock::time_point m_BenchPrevFrameEnd{};

	RageTextureRenderTarget* offscreenRenderTarget = nullptr;
};

#endif
