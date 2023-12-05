/******************************************************************************
 * @file    src/main.hpp
 * @project ColorTestSDL2
 * @brief   Main function, init, and quit routines
 * @author  ImpendingMoon
 * @created 9/2/2023
 ******************************************************************************/



#include <iostream>
#include <SDL2/SDL.h>
#include "main.hpp"

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Surface* render_surface = nullptr;
SDL_Surface* lit_surface = nullptr;
SDL_Texture* render_texture = nullptr;
SDL_Palette* indexed_palette = new SDL_Palette{ 256, const_cast<SDL_Color*>(palette.data()) };

bool exitRequested = false;
int darkLevel = 0;
bool underWater = false;

int SDL_main(int argc, char** argv)
{
    int err;
    err = initSDL2();
    if(err != 0)
    {
        std::cerr << "Could not initialize SDL2: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Main loop
    while(!exitRequested)
    {
        SDL_RenderClear(renderer);
        if(render_texture != nullptr)
        {
            SDL_RenderCopy(renderer, render_texture, nullptr, nullptr);
        }
        SDL_RenderPresent(renderer);

        handleEvents();
    }

    quitSDL2();

    return 0;
}



int initSDL2()
{
    int err;
    err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if(err != 0) { return 1; }

    window = SDL_CreateWindow(
        "TerraDOS Color Test",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        800,
        600,
        0
        | SDL_WINDOW_SHOWN
        | SDL_WINDOW_ALLOW_HIGHDPI
        | SDL_WINDOW_RESIZABLE
    );
    if(window == nullptr) { return 1; }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        0
        | SDL_RENDERER_SOFTWARE // Indexed rendering is slow on modern GPUs
        | SDL_RENDERER_PRESENTVSYNC
    );
    if(renderer == nullptr) { return 1; }

    SDL_RenderSetLogicalSize(
        renderer,
        800,
        600
    );

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    return 0;
}



void quitSDL2()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(render_surface);
    SDL_DestroyTexture(render_texture);
    SDL_FreePalette(indexed_palette);

    SDL_Quit();
}



void handleEvents()
{
    SDL_Event event;
    int err;

    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
        case SDL_QUIT:
        {
            exitRequested = true;
            break;
        }

        case SDL_DROPFILE:
        {
            err = loadNewBMP(event.drop.file);
            if(err != 0)
            {
                std::string msg = "Could not load bitmap: ";
                msg += SDL_GetError();

                SDL_ShowSimpleMessageBox(
                        SDL_MESSAGEBOX_ERROR,
                        "Error",
                        msg.c_str(),
                        window
                );
            }
            break;
        }

        case SDL_KEYDOWN:
        {
            switch(event.key.keysym.scancode)
            {
            case SDL_SCANCODE_UP:
            {
                if(darkLevel > 0) { darkLevel--; }
                updateDarkLevel();
                updateUnderwater();
                break;
            }

            case SDL_SCANCODE_DOWN:
            {
                if(darkLevel < 8) { darkLevel++; }
                updateDarkLevel();
                updateUnderwater();
                break;
            }

            case SDL_SCANCODE_SPACE:
            {
                underWater = !underWater;
                updateDarkLevel();
                updateUnderwater();
                break;
            }

            default: break;
            }

        }
        }
    }
}



/**
 * @brief Loads a new bitmap, and renders it as an indexed texture
 * @param filepath
 * @return 0 on success, 1 on failure
 */
int loadNewBMP(const std::string& filepath)
{
    int err;
    SDL_Surface* temp = SDL_LoadBMP(filepath.c_str());
    if(temp == nullptr) { return 1; }

    err = renderNewSurface(temp);

    SDL_FreeSurface(temp);

    return err;
}


/**
 * @brief Clears the window and renders a new surface onto it.
 * @param surface Pointer to an existing surface. Main does not obtain ownership of it.
 */
int renderNewSurface(SDL_Surface* surface)
{
    int err;

    // FIXME: This causes a crash, but not freeing causes memory leak.
    //SDL_FreeSurface(render_surface);
    //SDL_DestroyTexture(render_texture);

    render_surface = SDL_CreateRGBSurfaceWithFormat(
            0,
            surface->w, surface->h,
            8, SDL_PIXELFORMAT_INDEX8
    );
    if(render_surface == nullptr) { return 1; }

    err = SDL_SetSurfacePalette(
            render_surface,
            indexed_palette
    );
    if(err != 0) { return 1; }

    err = convertSurfaceToIndex(surface, render_surface);
    if(err != 0) { return 1; }

    render_texture = SDL_CreateTextureFromSurface(
            renderer,
            render_surface
    );
    if(render_texture == nullptr) { return 1; }

    // Use SDL2 for easy scaling
    err = SDL_RenderSetLogicalSize(
            renderer,
            render_surface->w,
            render_surface->h
    );
    if(err != 0) { return 1; }

    SDL_RenderClear(renderer);

    err = SDL_RenderCopy(
            renderer,
            render_texture,
            nullptr, nullptr
    );
    if(err != 0) { return 1; }

    SDL_RenderPresent(renderer);

    return 0;
}



int convertSurfaceToIndex(SDL_Surface* source, SDL_Surface* dest)
{
    // Lots of error checking
    if(source == nullptr || dest == nullptr) { return 1; }

    int err;
    size_t source_count = source->w * source->h;
    size_t dest_count = dest->w * dest->h;

    uint8_t* source_pixels = static_cast<uint8_t*>(source->pixels);
    uint8_t* dest_pixels = static_cast<uint8_t*>(dest->pixels);

    if(source->format->BitsPerPixel != 24)
    {
        SDL_SetError("Source Surface is not RGB888.");
        return 1;
    }

    if(dest->format->BitsPerPixel != 8)
    {
        SDL_SetError("Dest Surface is not Index8.");
        return 1;
    }

    if(source_count != dest_count)
    {
        SDL_SetError("Source Surface and Dest Surface resolutions are not equal.");
        return 1;
    }

    err = SDL_LockSurface(source);
    if(err != 0) {
        SDL_UnlockSurface(source);
        return 1;
    }

    err = SDL_LockSurface(dest);
    if(err != 0)
    {
        SDL_UnlockSurface(source);
        SDL_UnlockSurface(dest);
        return 1;
    }


    // Copy pixels
    for(int i = 0; i < source_count; i++)
    {
        int offset = i * 3;
        SDL_Color color = {
                source_pixels[offset + 2],
                source_pixels[offset + 1],
                source_pixels[offset]
        };

        dest_pixels[i] = findClosestPaletteEntry(color);
    }

    SDL_UnlockSurface(source);
    SDL_UnlockSurface(dest);

    return 0;
}


uint8_t findClosestPaletteEntry(SDL_Color color)
{
    uint8_t closestIndex = 0;
    double lowestDistance = INFINITY;

    for(size_t i = 0; i < palette.size(); i++)
    {
        SDL_Color p_color = palette.at(i);
        // Weighted euclidean algorithm. Green is most sensitive.
        double distance =
                (((p_color.r - color.r) * (p_color.r - color.r)) * 0.30)
                + (((p_color.g - color.g) * (p_color.g - color.g)) * 0.59)
                + (((p_color.b - color.b) * (p_color.b - color.b)) * 0.11);

        if(distance < lowestDistance)
        {
            lowestDistance = distance;
            closestIndex = i;
        }
    }

    return closestIndex;
}



void updateDarkLevel()
{
    if(render_surface == nullptr) { return; }

    std::string render_format = SDL_GetPixelFormatName(render_surface->format->format);
    std::cout << "Render Format: " << render_format << std::endl;

    size_t surface_size = render_surface->w * render_surface->h;

    SDL_FreeSurface(lit_surface);
    lit_surface = SDL_CreateRGBSurfaceWithFormat(
        0,
        render_surface->w,
        render_surface->h,
        8,
        SDL_PIXELFORMAT_INDEX8
    );
    SDL_SetSurfacePalette(lit_surface, indexed_palette);

    uint8_t* source_pixels = static_cast<uint8_t*>(render_surface->pixels);
    uint8_t* lit_pixels = static_cast<uint8_t*>(lit_surface->pixels);

    SDL_LockSurface(lit_surface);

    // Apply light level to every pixel
    for(size_t i = 0; i < surface_size; i++)
    {
        uint8_t color = source_pixels[i];
        if((darkLevel == 8) || (color > UINT8_MAX - (32 * darkLevel)))
        {
            color = 255;
        } else
        {
            color += (32 * darkLevel);
        }

        lit_pixels[i] = color;
    }

    SDL_DestroyTexture(render_texture);
    render_texture = SDL_CreateTextureFromSurface(renderer, lit_surface);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, render_texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}



void updateUnderwater()
{
    if(render_surface == nullptr) { return; }

    std::string render_format = SDL_GetPixelFormatName(render_surface->format->format);
    std::cout << "Render Format: " << render_format << std::endl;

    size_t surface_size = lit_surface->w * lit_surface->h;

    uint8_t* lit_pixels = static_cast<uint8_t*>(lit_surface->pixels);

    SDL_LockSurface(lit_surface);

    // Apply light level to every pixel
    for(size_t i = 0; i < surface_size; i++)
    {
        uint8_t color = lit_pixels[i];
        if(underWater)
        {
            color |= 0x10;
        }

        lit_pixels[i] = color;
    }

    SDL_DestroyTexture(render_texture);
    render_texture = SDL_CreateTextureFromSurface(renderer, lit_surface);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, render_texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}
