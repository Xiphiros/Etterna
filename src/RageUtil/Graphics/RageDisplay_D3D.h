/* RageDisplay_D3D - Direct3D renderer. */

#ifndef RAGE_DISPLAY_D3D_H
#define RAGE_DISPLAY_D3D_H

class RageDisplay_D3D : public RageDisplay
{
  public:
	RageDisplay_D3D();
	~RageDisplay_D3D() override;
	auto Init(VideoModeParams&& p, bool bAllowUnacceleratedRenderer)
	  -> std::string override;

	[[nodiscard]] auto GetApiDescription() const -> std::string override
	{
		return "D3D";
	}
	virtual void GetDisplaySpecs(DisplaySpecs& out) const override;
	void ResolutionChanged() override;
	[[nodiscard]] auto GetPixelFormatDesc(RagePixelFormat pf) const
	  -> const RagePixelFormatDesc* override;

	auto BeginFrame() -> bool override;
	void EndFrame() override;
	[[nodiscard]] auto GetActualVideoModeParams() const
	  -> const ActualVideoModeParams* override;
	void SetBlendMode(BlendMode mode) override;
	auto SupportsTextureFormat(RagePixelFormat pixfmt, bool realtime = false)
	  -> bool override;
	auto SupportsThreadedRendering() -> bool override;
	auto SupportsPerVertexMatrixScale() -> bool override { return false; }
	auto CreateTexture(RagePixelFormat pixfmt,
					   RageSurface* img,
					   bool bGenerateMipMaps) -> intptr_t override;
	void UpdateTexture(intptr_t uTexHandle,
					   RageSurface* img,
					   int xoffset,
					   int yoffset,
					   int width,
					   int height) override;
	void DeleteTexture(intptr_t iTexHandle) override;
	void ClearAllTextures() override;
	auto GetNumTextureUnits() -> int override;
	void SetTexture(TextureUnit tu, intptr_t iTexture) override;
	void SetTextureMode(TextureUnit tu, TextureMode tm) override;
	void SetTextureWrapping(TextureUnit tu, bool b) override;
	[[nodiscard]] auto GetMaxTextureSize() const -> int override;
	void SetTextureFiltering(TextureUnit tu, bool b) override;
	[[nodiscard]] auto IsZWriteEnabled() const -> bool override;
	[[nodiscard]] auto IsZTestEnabled() const -> bool override;
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

	auto CreateRenderTarget(const RenderTargetParam& param,
							int& iTextureWidthOut,
							int& iTextureHeightOut) -> intptr_t override;
	auto GetRenderTarget() -> intptr_t override;
	void SetRenderTarget(intptr_t uTexHandle, bool bPreserveTexture) override;

	void SetSphereEnvironmentMapping(TextureUnit tu, bool b) override;
	void SetCelShaded(int stage) override;

	auto IsD3DInternal() -> bool override;
	[[nodiscard]] auto SupportsFullscreenBorderlessWindow() const
	  -> bool override
	{
		return true;
	}
	[[nodiscard]] auto SupportsNonPowerOfTwoTextures() const -> bool override;

	auto CreateCompiledGeometry() -> RageCompiledGeometry* override;
	void DeleteCompiledGeometry(RageCompiledGeometry* p) override;

  protected:
	void DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts) override;
	void DrawQuadStripInternal(const RageSpriteVertex v[],
							   int iNumVerts) override;
	void DrawFanInternal(const RageSpriteVertex v[], int iNumVerts) override;
	void DrawStripInternal(const RageSpriteVertex v[], int iNumVerts) override;
	void DrawTrianglesInternal(const RageSpriteVertex v[],
							   int iNumVerts) override;
	void DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
										int iNumVerts) override;
	void DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
									  int iMeshIndex) override;

	auto TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut)
	  -> std::string override;
	auto CreateScreenshot() -> RageSurface* override;
	auto GetOrthoMatrix(float l, float r, float b, float t, float zn, float zf)
	  -> RageMatrix override;

	void RecoverFromDeviceLoss();
	void SendCurrentMatrices();

  private:
	void InvalidateStateCache(bool bFlushBatch = false,
							  bool bResetDeviceDefaults = false);
	void FlushQuadBatch();
	void ComputeD3DProjectionMatrix(RageMatrix& out);
	void SetRenderStateCached(uint32_t state,
							  uint32_t value,
							  uint32_t& cached,
							  bool& valid);
	void SetSamplerStateCached(TextureUnit tu,
							   uint32_t type,
							   uint32_t value,
							   uint32_t& cached,
							   bool& valid);

	uint32_t m_dwCachedAlphaBlend{ 0 };
	bool m_bAlphaBlendValid{ false };
	uint32_t m_dwCachedBlendOp{ 0 };
	bool m_bBlendOpValid{ false };
	uint32_t m_dwCachedSrcBlend{ 0 };
	bool m_bSrcBlendValid{ false };
	uint32_t m_dwCachedDestBlend{ 0 };
	bool m_bDestBlendValid{ false };
	uint32_t m_dwCachedSrcBlendAlpha{ 0 };
	bool m_bSrcBlendAlphaValid{ false };
	uint32_t m_dwCachedDestBlendAlpha{ 0 };
	bool m_bDestBlendAlphaValid{ false };
	uint32_t m_dwCachedZWrite{ 1 };
	bool m_bZWriteValid{ false };
	uint32_t m_dwCachedZEnable{ 1 };
	bool m_bZEnableValid{ false };
	uint32_t m_dwCachedZFunc{ 4 };
	bool m_bZFuncValid{ false };
	float m_fCachedZBias{ 0.0F };
	bool m_bZBiasValid{ false };
	uint32_t m_dwCachedAlphaTest{ 0 };
	bool m_bAlphaTestValid{ false };
	bool m_bAlphaRefFuncValid{ false };
	uint32_t m_dwCachedCullMode{ 0 };
	bool m_bCullModeValid{ false };
	uint32_t m_dwCachedLighting{ 1 };
	bool m_bLightingValid{ false };

	intptr_t m_aBoundTex[NUM_TextureUnit]{};
	bool m_abBoundTexValid[NUM_TextureUnit]{};
	uint32_t m_adwMinFilter[NUM_TextureUnit]{};
	bool m_abMinFilterValid[NUM_TextureUnit]{};
	uint32_t m_adwMagFilter[NUM_TextureUnit]{};
	bool m_abMagFilterValid[NUM_TextureUnit]{};
	uint32_t m_adwAddressU[NUM_TextureUnit]{};
	bool m_abAddressUValid[NUM_TextureUnit]{};
	uint32_t m_adwAddressV[NUM_TextureUnit]{};
	bool m_abAddressVValid[NUM_TextureUnit]{};
	TextureMode m_aCachedTexMode[NUM_TextureUnit]{};
	bool m_abTexModeValid[NUM_TextureUnit]{};

	uint64_t m_uUploadedCenteringRev{ 0 };
	uint64_t m_uUploadedProjRev{ 0 };
	bool m_bUploadedInvertY{ false };
	bool m_bProjUploadValid{ false };
	uint64_t m_uUploadedViewRev{ 0 };
	bool m_bViewUploadValid{ false };
	uint64_t m_uUploadedWorldRev{ 0 };
	bool m_bWorldIsIdentity{ false };
	bool m_bWorldUploadValid{ false };
	uint64_t m_auUploadedTexRev[NUM_TextureUnit]{};
	bool m_abUploadedSphereMap[NUM_TextureUnit]{};
	bool m_abTexMatrixIsIdentity[NUM_TextureUnit]{};
	bool m_abTexMatrixValid[NUM_TextureUnit]{};

	static constexpr size_t kMaxQuadBatchQuads = 2048;
	static constexpr size_t kMaxQuadBatchVerts = kMaxQuadBatchQuads * 4;
	std::vector<RageSpriteVertex> m_vQuadBatchVerts;
	std::vector<uint16_t> m_vQuadBatchIndices;
	RageMatrix m_QuadBatchProjection{};
	RageMatrix m_QuadBatchView{};
	uint64_t m_uQuadBatchCenteringRev{ 0 };
	uint64_t m_uQuadBatchProjRev{ 0 };
	uint64_t m_uQuadBatchViewRev{ 0 };
	bool m_bQuadBatchInvertY{ false };
};

class RenderTarget
{
  public:
	virtual ~RenderTarget() = default;

	virtual void Create(const RenderTargetParam& param,
						int& iTextureWidthOut,
						int& iTextureHeightOut) = 0;

	[[nodiscard]] virtual auto GetTexture() const -> intptr_t = 0;

	/* Render to this RenderTarget. */
	virtual void StartRenderingTo() = 0;

	/* Stop rendering to this RenderTarget.  Update the texture, if necessary,
	 * and make it available. */
	virtual void FinishRenderingTo() = 0;

	[[nodiscard]] virtual auto InvertY() const -> bool { return false; }

	[[nodiscard]] auto GetParam() const -> const RenderTargetParam&
	{
		return m_Param;
	}

  protected:
	RenderTargetParam m_Param;
};

#endif
