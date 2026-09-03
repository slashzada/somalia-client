#pragma once
#include <d3d9.h>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace TextureLoader
{
    inline IDirect3DTexture9* CreateTextureFromMemory(IDirect3DDevice9* pDevice, const unsigned char* data, int dataSize)
    {
        if (!pDevice || !data || dataSize <= 0)
            return nullptr;

        int width = 0, height = 0, channels = 0;
        unsigned char* image = stbi_load_from_memory(data, dataSize, &width, &height, &channels, 4);
        if (!image)
            return nullptr;

        IDirect3DTexture9* pTexture = nullptr;
        HRESULT hr = pDevice->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);
        if (FAILED(hr) || !pTexture)
        {
            stbi_image_free(image);
            return nullptr;
        }

        D3DLOCKED_RECT lockedRect;
        if (SUCCEEDED(pTexture->LockRect(0, &lockedRect, NULL, 0)))
        {
            unsigned char* dest = static_cast<unsigned char*>(lockedRect.pBits);
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    int srcIdx = (y * width + x) * 4;
                    int dstIdx = y * lockedRect.Pitch + x * 4;
                    // D3DFMT_A8R8G8B8 é ordenado como BGRA em memória Little-Endian
                    dest[dstIdx + 0] = image[srcIdx + 2]; // Blue
                    dest[dstIdx + 1] = image[srcIdx + 1]; // Green
                    dest[dstIdx + 2] = image[srcIdx + 0]; // Red
                    dest[dstIdx + 3] = image[srcIdx + 3]; // Alpha
                }
            }
            pTexture->UnlockRect(0);
        }

        stbi_image_free(image);
        return pTexture;
    }
}
