//Real project week5
/** @file Week5-1-CrateApp.cpp
 *  @brief Texture Demo!
 *   Texture mapping is a technique that allows us to map image data onto a triangle, 
 *   thereby enabling us to increase the details and realism of our scene significantly. 
 *   In this demo, we build a cube and turn it into a stone by mapping a stone texture on each side.
 * 
 *   @note: uv coordinates start from the upper left corner (v-axis is facing down).
 *   st coordinates start from the lower left corner (t-axis is facing up).
 *   Direct3D uses a texture coordinate system that consists of a u-axis that runs
 *   horizontally to the image and a v-axis that runs vertically to the image. The coordinates,
 *   (u, v) such that 0 ≤ u, v ≤ 1, identify an element on the texture called a texel.
 *
 *   Controls:
 *   Hold down '1' key to view scene in wireframe mode.
 *   Hold the left mouse button down and move the mouse to rotate.
 *   Hold the right mouse button down and move the mouse to zoom in and out.
 *
 *  @author Hooman Salamat
 */

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <iostream>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace std;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

class CrateApp : public D3DApp
{
public:
    CrateApp(HINSTANCE hInstance);
    CrateApp(const CrateApp& rhs) = delete;
    CrateApp& operator=(const CrateApp& rhs) = delete;
    ~CrateApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	//step5
	void LoadTextures();

    void BuildRootSignature();

	//step9
	void BuildDescriptorHeaps();

    void BuildShadersAndInputLayout();
    void BuildShapeGeometry(string name, const GeometryGenerator::MeshData& shape);
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems(string name, string materials, float sX, float sY, float sZ, float tX, float tY, float tZ);
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	//step20
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	//step11
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

	//step7
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mOpaquePSO = nullptr;
 
	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

    PassConstants mMainPassCB;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.3f*XM_PI;
	float mPhi = 0.4f*XM_PI;
	float mRadius = 2.5f;

    POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        CrateApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

CrateApp::CrateApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

CrateApp::~CrateApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool CrateApp::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //step6
	LoadTextures();

    BuildRootSignature();

	//step10
	BuildDescriptorHeaps();

    BuildShadersAndInputLayout();
	GeometryGenerator geoGen;
	BuildShapeGeometry("Box", geoGen.CreateBox(1.0f, 1.0f, 1.0f, 0));
	BuildShapeGeometry("Box2", geoGen.CreateBox(1.0f, 1.0f, 1.0f, 0));
	BuildShapeGeometry("Box3", geoGen.CreateBox(1.0f, 1.0f, 1.0f, 0));
	BuildShapeGeometry("Cylinder", geoGen.CreateCylinder(1.0f, 1.0f, 1.0f, 30, 1));
	//BuildShapeGeometry("Cone", geoGen.CreateCylinder(1.0f, 0.0f, 1.0f, 30, 1), XMFLOAT4(DirectX::Colors::DarkRed));
	BuildShapeGeometry("Sphere", geoGen.CreateSphere(1.0f, 30, 30));
	//BuildShapeGeometry("Triangle", geoGen.CreateSphere(1.0f, 5, 2), XMFLOAT4(DirectX::Colors::Green));
	BuildShapeGeometry("Grid", geoGen.CreateGrid(1.0f, 5.0f, 2, 2));
	BuildShapeGeometry("Grid2", geoGen.CreateGrid(1.0f, 5.0f, 2, 2));
	BuildShapeGeometry("Grid3", geoGen.CreateGrid(1.0f, 5.0f, 2, 2));

	BuildShapeGeometry("Pyramid", geoGen.CreatePyramid(1.0f, 1.0f, 1.0f, 0));
	BuildShapeGeometry("Cone2", geoGen.CreateCone(1.0f, 1.0f, 10, 2));
	BuildShapeGeometry("Wedge", geoGen.CreateWedge(1.0f, 1.0f, 1.0f, 0));
	BuildShapeGeometry("TriangularPrism", geoGen.CreateTriangularPrism(1.0f, 1.0f, 1.0f, 0));
	BuildShapeGeometry("Diamond", geoGen.CreateDiamond(1.0f));
	BuildMaterials();
	////Alligators
	BuildRenderItems("TriangularPrism", "grass", 2.0f, 2.0f, 2.0f, 10.0f, -2.0f, -5.0f);
	BuildRenderItems("TriangularPrism", "grass", 2.0f, 2.0f, 2.0f, 10.0f, -2.0f, 5.0f);
	BuildRenderItems("TriangularPrism", "grass", 2.0f, 2.0f, 2.0f, 10.0f, -2.0f, -11.0f);
	BuildRenderItems("TriangularPrism", "grass", 2.0f, 2.0f, 2.0f, 5.0f, -2.0f, -11.0f);
	BuildRenderItems("TriangularPrism", "grass", 2.0f, 2.0f, 2.0f, 0.0f, -2.0f, -11.0f);
	BuildRenderItems("TriangularPrism", "grass", 2.0f, 2.0f, 2.0f, -5.0f, -2.0f, -11.0f);
	BuildRenderItems("TriangularPrism", "grass", 2.0f, 2.0f, 2.0f, -10.0f, -2.0f, -11.0f);

	//Decoration
	BuildRenderItems("Diamond", "ice", 2.0f, 2.0f, 2.0f, 0.0f, 10.0f, 0.0f);

	//Chains for bridge
	BuildRenderItems("Wedge", "woodCrate", 6.0f, 5.0f, 0.5f, 11.0f, 0.5f, -3.0f);
	BuildRenderItems("Wedge", "woodCrate", 6.0f, 5.0f, 0.5f, 11.0f, 0.5f, 3.0f);

	//Walls
	BuildRenderItems("Box", "bricks", 2.0f, 6.0f, 12.0f, -8.0f, 1.0f, 0.0f);
	BuildRenderItems("Box", "bricks", 2.0f, 6.0f, 12.0f, 8.0f, 1.0f, 0.0f);
	BuildRenderItems("Box", "bricks", 12.0f, 6.0f, 2.0f, 0.0f, 1.0f, 8.0f);
	BuildRenderItems("Box", "bricks", 12.0f, 6.0f, 2.0f, 0.0f, 1.0f, -8.0f);

	//Towers
	BuildRenderItems("Cylinder", "grass", 2.0f, 8.0f, 2.0f, -8.0f, 2.0f, 8.0f);
	BuildRenderItems("Cylinder", "grass", 2.0f, 8.0f, 2.0f, 8.0f, 2.0f, 8.0f);
	BuildRenderItems("Cylinder", "grass", 2.0f, 8.0f, 2.0f, -8.0f, 2.0f, -8.0f);
	BuildRenderItems("Cylinder", "grass", 2.0f, 8.0f, 2.0f, 8.0f, 2.0f, -8.0f);

	//Tower roofs
	BuildRenderItems("Cone2", "woodCrate", 2.0f, 8.0f, 2.0f, -8.0f, 8.0f, 8.0f);
	BuildRenderItems("Cone2", "woodCrate", 2.0f, 8.0f, 2.0f, 8.0f, 8.0f, 8.0f);
	BuildRenderItems("Cone2", "woodCrate", 2.0f, 8.0f, 2.0f, -8.0f, 8.0f, -8.0f);
	BuildRenderItems("Cone2", "woodCrate", 2.0f, 8.0f, 2.0f, 8.0f, 8.0f, -8.0f);

	////Tower balls
	BuildRenderItems("Sphere", "ice", 1.0f, 1.0f, 1.0f, -8.0f, 12.0f, 8.0f);
	BuildRenderItems("Sphere", "ice", 1.0f, 1.0f, 1.0f, 8.0f, 12.0f, 8.0f);
	BuildRenderItems("Sphere", "ice", 1.0f, 1.0f, 1.0f, -8.0f, 12.0f, -8.0f);
	BuildRenderItems("Sphere", "ice", 1.0f, 1.0f, 1.0f, 8.0f, 12.0f, -8.0f);

	////Gate and door
	BuildRenderItems("Box2", "woodCrate", 6.0f, 0.5f, 6.0f, 11.0f, -2.0f, 0.0f);
	BuildRenderItems("Box3", "woodCrate", 0.5f, 6.0f, 6.0f, 9.0f, 1.0f, 0.0f);

	////Decoration on walls
	BuildRenderItems("Pyramid", "ice", 1.0f, 1.0f, 1.0f, -8.0f, 4.0f, 0.0f);
	BuildRenderItems("Pyramid", "ice", 1.0f, 1.0f, 1.0f, -8.0f, 4.0f, 4.0f);
	BuildRenderItems("Pyramid", "ice", 1.0f, 1.0f, 1.0f, -8.0f, 4.0f, -4.0f);

	BuildRenderItems("Pyramid", "ice", 1.0f, 1.0f, 1.0f, 0.0f, 4.0f, 8.0f);
	BuildRenderItems("Pyramid", "ice", 1.0f, 1.0f, 1.0f, 4.0f, 4.0f, 8.0f);
	BuildRenderItems("Pyramid", "ice", 1.0f, 1.0f, 1.0f, -4.0f, 4.0f, 8.0f);

	BuildRenderItems("Pyramid", "ice", 1.0f, 1.0f, 1.0f, 4.0f, 4.0f, -8.0f);
	BuildRenderItems("Pyramid", "ice", 1.0f, 1.0f, 1.0f, 0.0f, 4.0f, -8.0f);
	BuildRenderItems("Pyramid", "ice", 1.0f, 1.0f, 1.0f, -4.0f, 4.0f, -8.0f);

	//Moat, floor and grass
	BuildRenderItems("Grid", "water", 25.0f, 20.0f, 5.0f, 0.0f, -2.0f, 0.0f);
	BuildRenderItems("Grid2", "grass", 40.0f, 20.0f, 10.0f, 0.0f, -2.8f, 0.0f);
	BuildRenderItems("Grid3", "woodCrate", 15.0f, 20.0f, 4.0f, 0.0f, -1.8f, 0.0f);
    BuildFrameResources();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void CrateApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void CrateApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
	UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
}

void CrateApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
 
void CrateApp::OnKeyboardInput(const GameTimer& gt)
{
}
 
void CrateApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void CrateApp::AnimateMaterials(const GameTimer& gt)
{
	
}

void CrateApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for(auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if(e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void CrateApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for(auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void CrateApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

//step 8
void CrateApp::LoadTextures()
{
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	woodCrateTex->Filename = L"../../Textures/stone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource, woodCrateTex->UploadHeap));
 
	mTextures[woodCrateTex->Name] = std::move(woodCrateTex);

	//Grass
	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../../Textures/grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));

	mTextures[grassTex->Name] = std::move(grassTex);

	//Bricks
	auto bricksTex = std::make_unique<Texture>();
	bricksTex->Name = "bricksTex";
	bricksTex->Filename = L"../../Textures/bricks3.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), bricksTex->Filename.c_str(),
		bricksTex->Resource, bricksTex->UploadHeap));

	mTextures[bricksTex->Name] = std::move(bricksTex);

	//Ice
	auto iceTex = std::make_unique<Texture>();
	iceTex->Name = "iceTex";
	iceTex->Filename = L"../../Textures/ice.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), iceTex->Filename.c_str(),
		iceTex->Resource, iceTex->UploadHeap));

	mTextures[iceTex->Name] = std::move(iceTex);

	//Water
	auto waterTex = std::make_unique<Texture>();
	waterTex->Name = "waterTex";
	waterTex->Filename = L"../../Textures/water1.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), waterTex->Filename.c_str(),
		waterTex->Resource, waterTex->UploadHeap));

	mTextures[waterTex->Name] = std::move(waterTex);
}

void CrateApp::BuildRootSignature()
{
	//step22
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0);
    slotRootParameter[2].InitAsConstantBufferView(1);
    slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

    // A root signature is an array of root parameters.
	//The Init function of the CD3DX12_ROOT_SIGNATURE_DESC class has two parameters that allow you to
		//define an array of so - called static samplers your application can use.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),  //6 samplers!
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

//step12
//Once a texture resource is created, we need to create an SRV descriptor to it which we
//can set to a root signature parameter slot for use by the shader programs.
void CrateApp::BuildDescriptorHeaps()
{
    //
    // Create the SRV heap.
    //
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 5; // Two descriptors (for two textures)
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

    //
    // Fill out the heap with actual descriptors.
    //
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    auto woodCrateTex = mTextures["woodCrateTex"]->Resource;
    auto grassTex = mTextures["grassTex"]->Resource;
    auto bricksTex = mTextures["bricksTex"]->Resource;
    auto iceTex = mTextures["iceTex"]->Resource;
    auto waterTex = mTextures["waterTex"]->Resource;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    // Set default shader 4 component mapping
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // Wood Crate Texture
    srvDesc.Format = woodCrateTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);

    // Offset the descriptor for the next texture
    hDescriptor.Offset(1, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    // Grass Texture
    srvDesc.Format = grassTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = grassTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

    // Specifying the minimum mipmap level that can be accessed
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	//bricks
	hDescriptor.Offset(1, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	// Bricks Texture
	srvDesc.Format = bricksTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = bricksTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, hDescriptor);

	// Specifying the minimum mipmap level that can be accessed
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	//ice
	hDescriptor.Offset(1, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	// Ice Texture
	srvDesc.Format = iceTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = iceTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(iceTex.Get(), &srvDesc, hDescriptor);

	// Specifying the minimum mipmap level that can be accessed
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	//water
	hDescriptor.Offset(1, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	// Water Texture
	srvDesc.Format = waterTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = waterTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

	// Specifying the minimum mipmap level that can be accessed
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
}


void CrateApp::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_0");
	
    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//step3
		//The texture coordinates determine what part of the texture gets mapped on the triangles.
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void CrateApp::BuildShapeGeometry(string name, const GeometryGenerator::MeshData& shape)
{
	//GeometryGenerator is a utility class for generating simple geometric shapes like grids, sphere, cylinders, and boxes
	//GeometryGenerator geoGen;
	//The MeshData structure is a simple structure nested inside GeometryGenerator that stores a vertex and index list
	GeometryGenerator::MeshData box = shape;

	//GeometryGenerator::MeshData box = geoGen.CreateCylinder(2.0f, 2.0f, 5.0f, 30, 1);


	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;


	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;


	// Define the SubmeshGeometry that cover different 
	// regions of the vertex/index buffers.

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;


	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount = box.Vertices.size();


	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[k].Normal;
		vertices[k].TexC = box.Vertices[k].TexC;
	}


	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));


	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = name;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;


	mGeometries[geo->Name] = std::move(geo);
}

void CrateApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), 
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mOpaquePSO)));
}

void CrateApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
    }
}
//step13
void CrateApp::BuildMaterials()
{
	auto woodCrate = std::make_unique<Material>();
	woodCrate->Name = "woodCrate";
	woodCrate->MatCBIndex = 0;
	//! We add an index to our material definition, which references an SRV in the descriptor
	//! heap specifying the texture associated with the material :
	woodCrate->DiffuseSrvHeapIndex = 0;
	woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	woodCrate->Roughness = 0.2f;

	mMaterials["woodCrate"] = std::move(woodCrate);

	//Grass
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 1;
	//! We add an index to our material definition, which references an SRV in the descriptor
	//! heap specifying the texture associated with the material :
	grass->DiffuseSrvHeapIndex = 1;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	grass->Roughness = 0.2f;

	mMaterials["grass"] = std::move(grass);

	//Bricks
	auto bricks = std::make_unique<Material>();
	bricks->Name = "bricks";
	bricks->MatCBIndex = 2;
	//! We add an index to our material definition, which references an SRV in the descriptor
	//! heap specifying the texture associated with the material :
	bricks->DiffuseSrvHeapIndex = 2;
	bricks->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	bricks->Roughness = 0.2f;

	mMaterials["bricks"] = std::move(bricks);

	//Ice
	auto ice = std::make_unique<Material>();
	ice->Name = "ice";
	ice->MatCBIndex = 3;
	//! We add an index to our material definition, which references an SRV in the descriptor
	//! heap specifying the texture associated with the material :
	ice->DiffuseSrvHeapIndex = 3;
	ice->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	ice->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	ice->Roughness = 0.2f;

	mMaterials["ice"] = std::move(ice);

	//Water
	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 4;
	//! We add an index to our material definition, which references an SRV in the descriptor
	//! heap specifying the texture associated with the material :
	water->DiffuseSrvHeapIndex = 4;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	water->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	water->Roughness = 0.2f;

	mMaterials["water"] = std::move(water);
}

//step14
void CrateApp::BuildRenderItems(string name, string materials, float sX, float sY, float sZ, float tX, float tY, float tZ)
{
	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(sX, sY, sZ) * XMMatrixTranslation(tX, tY, tZ));
	boxRitem->ObjCBIndex = static_cast<UINT>(mAllRitems.size());
	boxRitem->Mat = mMaterials[materials].get();
	boxRitem->Geo = mGeometries[name].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(boxRitem));

	// All the render items are opaque.
	mOpaqueRitems.clear();
	for(auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void CrateApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
 
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		//step18
		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		//step19: assuming the root signature has been defined to expect a table of shader
		//resource views to be bound to the 0th slot parameter
		cmdList->SetGraphicsRootDescriptorTable(0, tex);
        cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

//step21
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CrateApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return { 
		pointWrap, pointClamp,
		linearWrap, linearClamp, 
		anisotropicWrap, anisotropicClamp };
}









