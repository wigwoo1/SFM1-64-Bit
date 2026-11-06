#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "viewport_copy_fixture_candidate.h"

using Microsoft::WRL::ComPtr;

static void check(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        std::cerr << operation << " HRESULT=0x" << std::hex
                  << static_cast<unsigned long>(result) << std::dec << '\n';
        throw std::runtime_error(operation);
    }
}

struct TestWindow {
    HWND handle = CreateWindowW(L"STATIC", L"", WS_POPUP, 0, 0, 16, 16,
                                nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    ~TestWindow() { if (handle) DestroyWindow(handle); }
};

struct Target {
    ComPtr<IDirect3DTexture9> texture;
    ComPtr<IDirect3DSurface9> surface;
    ComPtr<IDirect3DSurface9> readback;
};

static Target makeTarget(IDirect3DDevice9* device, UINT width, D3DFORMAT format) {
    Target target;
    check(device->CreateTexture(width, 1, 1, D3DUSAGE_RENDERTARGET, format,
                               D3DPOOL_DEFAULT, target.texture.GetAddressOf(), nullptr), "create target");
    check(target.texture->GetSurfaceLevel(0, target.surface.GetAddressOf()), "get target surface");
    check(device->CreateOffscreenPlainSurface(width, 1, format, D3DPOOL_SYSTEMMEM,
                                            target.readback.GetAddressOf(), nullptr), "create readback");
    return target;
}

struct Vertex { float x, y, z, rhw, u, v; };

static void drawCopy(IDirect3DDevice9* device, IDirect3DTexture9* input,
                     const Target& output, UINT width, bool decode, bool encode) {
    check(device->SetTexture(0, nullptr), "clear previous sampling binding");
    check(device->SetRenderTarget(0, output.surface.Get()), "bind output");
    D3DVIEWPORT9 viewport{0, 0, width, 1, 0.0f, 1.0f};
    check(device->SetViewport(&viewport), "set viewport");
    check(device->SetTexture(0, input), "bind input");
    check(device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, decode ? TRUE : FALSE), "set read conversion");
    check(device->SetRenderState(D3DRS_SRGBWRITEENABLE, encode ? TRUE : FALSE), "set output conversion");
    const float right = static_cast<float>(width) - 0.5f;
    const Vertex quad[] = {
        {-0.5f, -0.5f, 0, 1, 0, 0}, {right, -0.5f, 0, 1, 1, 0},
        {-0.5f, 0.5f, 0, 1, 0, 1}, {right, 0.5f, 0, 1, 1, 1},
    };
    check(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF00FFFF, 1, 0), "clear target");
    check(device->BeginScene(), "begin copy");
    const HRESULT draw = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));
    const HRESULT end = device->EndScene();
    check(draw, "draw copy");
    check(end, "end copy");
}

static std::vector<std::array<uint8_t, 3>> readRgb(IDirect3DDevice9* device,
                                                  const Target& target, UINT width) {
    check(device->GetRenderTargetData(target.surface.Get(), target.readback.Get()), "read target");
    D3DLOCKED_RECT locked{};
    check(target.readback->LockRect(&locked, nullptr, D3DLOCK_READONLY), "lock readback");
    std::vector<std::array<uint8_t, 3>> result(width);
    const auto* pixels = static_cast<const uint8_t*>(locked.pBits);
    for (UINT index = 0; index < width; ++index) {
        result[index] = {pixels[index * 4 + 2], pixels[index * 4 + 1], pixels[index * 4]};
    }
    check(target.readback->UnlockRect(), "unlock readback");
    return result;
}

int main() {
    static_assert(sizeof(void*) == 8, "This fixture must be native x64");
    try {
        TestWindow window;
        if (!window.handle) throw std::runtime_error("create hidden test window");
        ComPtr<IDirect3D9> api;
        api.Attach(Direct3DCreate9(D3D_SDK_VERSION));
        if (!api) throw std::runtime_error("Direct3D9 unavailable");
        UINT adapter = api->GetAdapterCount();
        D3DADAPTER_IDENTIFIER9 identifier{};
        for (UINT index = 0; index < api->GetAdapterCount(); ++index) {
            D3DADAPTER_IDENTIFIER9 candidate{};
            check(api->GetAdapterIdentifier(index, 0, &candidate), "query adapter");
            if (candidate.VendorId == 0x10DE && candidate.DeviceId == 0x2D18) {
                adapter = index;
                identifier = candidate;
                break;
            }
        }
        if (adapter == api->GetAdapterCount()) throw std::runtime_error("benchmark NVIDIA GPU unavailable");
        std::cout << "adapter=" << adapter << " vendor=0x" << std::hex << identifier.VendorId
                  << " device=0x" << identifier.DeviceId << std::dec
                  << " description=" << identifier.Description << '\n';
        D3DPRESENT_PARAMETERS parameters{};
        parameters.Windowed = TRUE;
        parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        parameters.BackBufferWidth = parameters.BackBufferHeight = 16;
        parameters.hDeviceWindow = window.handle;
        ComPtr<IDirect3DDevice9> device;
        check(api->CreateDevice(adapter, D3DDEVTYPE_HAL, window.handle,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &parameters,
                                device.GetAddressOf()), "create device");
        check(device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1), "set vertices");
        check(device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), "disable culling");
        check(device->SetRenderState(D3DRS_ZENABLE, FALSE), "disable depth");
        check(device->SetRenderState(D3DRS_LIGHTING, FALSE), "disable lighting");
        check(device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE), "disable blending");
        check(device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1), "select texture");
        check(device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE), "texture color");
        check(device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE), "disable next stage");
        check(device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT), "point minification");
        check(device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT), "point magnification");
        check(device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE), "disable mip selection");

        const auto fixture = viewportCopyFixtureCandidate();
        const UINT width = static_cast<UINT>(fixture.size());
        ComPtr<IDirect3DTexture9> input;
        check(device->CreateTexture(width, 1, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED,
                                    input.GetAddressOf(), nullptr), "create float input");
        D3DLOCKED_RECT locked{};
        check(input->LockRect(0, &locked, nullptr, 0), "lock float input");
        std::memcpy(locked.pBits, fixture.data(), fixture.size() * sizeof(fixture[0]));
        check(input->UnlockRect(0), "unlock float input");
        size_t encodingChecks = 0, copyChecks = 0;
        unsigned worstEncodingError = 0;
        for (D3DFORMAT format : {D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8}) {
            auto encoded = makeTarget(device.Get(), width, format);
            auto displayed = makeTarget(device.Get(), width, format);
            drawCopy(device.Get(), input.Get(), encoded, width, false, true);
            const auto first = readRgb(device.Get(), encoded, width);
            for (UINT index = 0; index < width; ++index) {
                for (UINT channel = 0; channel < 3; ++channel) {
                    const int expected = viewportCopySrgbByteCandidate(fixture[index][channel]);
                    const unsigned error = static_cast<unsigned>(std::abs(int(first[index][channel]) - expected));
                    worstEncodingError = std::max(worstEncodingError, error);
                    if (error > 1) throw std::runtime_error("output conversion exceeds one-byte tolerance");
                    ++encodingChecks;
                }
            }
            drawCopy(device.Get(), encoded.texture.Get(), displayed, width, false, false);
            const auto second = readRgb(device.Get(), displayed, width);
            if (first != second) throw std::runtime_error("viewport copy altered encoded RGB bytes");
            copyChecks += width * 3;
            reportViewportCopyPixelsCandidate("d3d9", static_cast<unsigned>(format), first, second);
            // These controls establish that the corpus detects either misplaced conversion.
            drawCopy(device.Get(), encoded.texture.Get(), displayed, width, true, false);
            const auto decoded = readRgb(device.Get(), displayed, width);
            drawCopy(device.Get(), encoded.texture.Get(), displayed, width, false, true);
            const auto reencoded = readRgb(device.Get(), displayed, width);
            size_t decodeDifferences = 0, encodeDifferences = 0;
            for (UINT index = 0; index < width; ++index) {
                if (decoded[index] != first[index]) ++decodeDifferences;
                if (reencoded[index] != first[index]) ++encodeDifferences;
            }
            if (decodeDifferences < 200 || encodeDifferences < 200)
                throw std::runtime_error("conversion controls failed to distinguish wrong copy settings");
            std::cout << "target_format=" << static_cast<unsigned>(format)
                      << " pixels=" << width << " copy_rgb_exact=1"
                      << " wrong_read_changed_pixels=" << decodeDifferences
                      << " wrong_write_changed_pixels=" << encodeDifferences << '\n';
            check(device->SetTexture(0, nullptr), "release sampling binding");
        }
        std::cout << "PASS native_x64=1 float_input_format=116 encoding_checks=" << encodingChecks
                  << " copy_checks=" << copyChecks << " max_encoding_byte_error=" << worstEncodingError
                  << " original_sfm_execution=0\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
