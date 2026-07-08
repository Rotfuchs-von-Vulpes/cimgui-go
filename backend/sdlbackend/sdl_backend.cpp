#if 1 //defined(CIMGUI_GO_USE_SDL2)
#define CIMGUI_USE_SDL2
#define CIMGUI_USE_OPENGL3

#include "sdl_backend.h"
#include "../../cwrappers/cimgui.h"
#include "../../cwrappers/cimgui_impl.h"

#include <cstdlib>
#include <stdint.h>
#include <stdio.h>
#include "../../thirdparty/SDL/include/SDL.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include "../../thirdparty/SDL/include/SDL_opengles2.h"
#else
#include "../../thirdparty/SDL/include/SDL_opengl.h"
#endif

#ifdef __EMSCRIPTEN__
#include "../../libs/emscripten/emscripten_mainloop_stub.h"
#endif

ImVec4 sdl_clear_color = *ImVec4_ImVec4_Float(0.45, 0.55, 0.6, 1.0);

// Setup SDL

unsigned int sdl_target_fps = 30;

static const SDL_WindowFlags sdl_opengl_default_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
static const SDL_WindowFlags sdl_software_default_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
static SDL_WindowFlags sdl_flags = sdl_opengl_default_flags;
static bool sdl_use_software_renderer = false;
static SDL_Renderer* sdl_renderer = NULL;
static SDL_GLContext sdl_gl_context = NULL;

static SDL_WindowFlags igSDLDefaultWindowFlags() {
    return sdl_use_software_renderer ? sdl_software_default_flags : sdl_opengl_default_flags;
}

static Uint8 igSDLColorComponent(float value) {
    if (value < 0.0f) {
        value = 0.0f;
    }
    if (value > 1.0f) {
        value = 1.0f;
    }
    return (Uint8)(value * 255.0f);
}

void igSDLSetSoftwareRendering(int enabled) {
    sdl_use_software_renderer = enabled != 0;
    sdl_flags = igSDLDefaultWindowFlags();
}

int igInitSDL() {
    return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
}

// Main code
SDL_Window* igCreateSDLWindow(const char* title, int width, int height, VoidCallback afterCreateContext)
{
    
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    if (!sdl_use_software_renderer) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    }
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    if (!sdl_use_software_renderer) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    }
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    if (!sdl_use_software_renderer) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    }
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_WindowFlags window_flags = sdl_flags;
    if (sdl_use_software_renderer) {
        window_flags = (SDL_WindowFlags)(window_flags & ~SDL_WINDOW_OPENGL);
    }

    if (!sdl_use_software_renderer) {
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    }

    SDL_Window* window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
    sdl_flags = igSDLDefaultWindowFlags();
    if (!window) {
        printf("Error creating SDL window %s\n", SDL_GetError());
        return NULL;
    }

    if (sdl_use_software_renderer) {
        sdl_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!sdl_renderer) {
            printf("Error creating SDL software renderer %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            return NULL;
        }
        SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);
    } else {
        sdl_gl_context = SDL_GL_CreateContext(window);
        if (!sdl_gl_context) {
            printf("Error creating SDL context %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            return NULL;
        }
        SDL_GL_MakeCurrent(window, sdl_gl_context);
        SDL_GL_SetSwapInterval(1);
    }

    // Setup Dear ImGui context
    igCreateContext(0);

    if (afterCreateContext != NULL) {
        afterCreateContext();
    }

    ImGuiIO* io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (!sdl_use_software_renderer) {
        io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    igStyleColorsDark(0);
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle* style = igGetStyle();
    if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style->WindowRounding = 0.0f;
        style->Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    if (sdl_use_software_renderer) {
        ImGui_ImplSDL2_InitForSDLRenderer(window, sdl_renderer);
        ImGui_ImplSDLRenderer2_Init(sdl_renderer);
    } else {
        ImGui_ImplSDL2_InitForOpenGL(window, sdl_gl_context);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }
// Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    return window;
}

void igRefresh() {
    SDL_Event redrawEvent;
    redrawEvent.type = SDL_WINDOWEVENT;
    redrawEvent.window.event = SDL_WINDOWEVENT_EXPOSED;
    SDL_PushEvent(&redrawEvent);
}

void igSDLRunLoop(SDL_Window *window, VoidCallback loop, VoidCallback beforeRender, VoidCallback afterRender,
               VoidCallback beforeDestroyContext) {
    ImGuiIO* io = igGetIO_Nil();
    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        if (beforeRender != NULL) {
            beforeRender();
        }

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        if (sdl_use_software_renderer) {
            ImGui_ImplSDLRenderer2_NewFrame();
        } else {
            ImGui_ImplOpenGL3_NewFrame();
        }
        ImGui_ImplSDL2_NewFrame();
        igNewFrame();

        if (sdl_use_software_renderer) {
            SDL_SetRenderDrawColor(sdl_renderer,
                                   igSDLColorComponent(sdl_clear_color.x * sdl_clear_color.w),
                                   igSDLColorComponent(sdl_clear_color.y * sdl_clear_color.w),
                                   igSDLColorComponent(sdl_clear_color.z * sdl_clear_color.w),
                                   igSDLColorComponent(sdl_clear_color.w));
            SDL_RenderClear(sdl_renderer);
        } else {
            glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
            glClearColor(sdl_clear_color.x * sdl_clear_color.w, sdl_clear_color.y * sdl_clear_color.w, sdl_clear_color.z * sdl_clear_color.w, sdl_clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        // rendering
        if (loop != NULL) {
            loop();
        }

        // Rendering
        igRender();
        if (sdl_use_software_renderer) {
            ImGui_ImplSDLRenderer2_RenderDrawData(igGetDrawData(), sdl_renderer);
            SDL_RenderPresent(sdl_renderer);
        } else {
            ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

            // Update and Render additional Platform Windows
            // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
            //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
            if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
                SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
                igUpdatePlatformWindows();
                igRenderPlatformWindowsDefault(0, 0);
                SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
            }

            SDL_GL_SwapWindow(window);
        }

        if (afterRender != NULL) {
            afterRender();
        }
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    if (sdl_use_software_renderer) {
        ImGui_ImplSDLRenderer2_Shutdown();
    } else {
        ImGui_ImplOpenGL3_Shutdown();
    }
    ImGui_ImplSDL2_Shutdown();

    if (beforeDestroyContext != NULL) {
        beforeDestroyContext();
    }

    igDestroyContext(0);

    if (sdl_use_software_renderer) {
        SDL_DestroyRenderer(sdl_renderer);
        sdl_renderer = NULL;
    } else {
        SDL_GL_DeleteContext(sdl_gl_context);
        sdl_gl_context = NULL;
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void igSDLWindow_GetContentScale(SDL_Window *window, float *width, float *height) {
    SDL_Renderer* renderer = SDL_GetRenderer(window);
    if (renderer != NULL) {
        SDL_RenderGetScale(renderer, width, height);
        return;
    }
    *width = ImGui_ImplSDL2_GetContentScaleForWindow(window);
    *height = *width;
}

void igSDLWindow_GetDisplaySize(SDL_Window *window, int *width, int *height) {
    SDL_GetWindowSize(window, width, height);
}

void igSDLWindow_GetWindowPos(SDL_Window *window, int *x, int *y) { SDL_GetWindowPosition(window, x, y); }

void igSDLWindow_SetWindowPos(SDL_Window *window, int x, int y) { SDL_SetWindowPosition(window, x, y); }

void igSDLWindow_SetSize(SDL_Window *window, int width, int height) { SDL_SetWindowSize(window, width, height); }

void igSDLWindow_SetTitle(SDL_Window *window, const char *title) { SDL_SetWindowTitle(window, title); }

void igSDLWindow_SetSizeLimits(SDL_Window *window, int minWidth, int minHeight, int maxWidth, int maxHeight) {
    SDL_SetWindowMinimumSize(window, minWidth, minHeight);
    SDL_SetWindowMaximumSize(window, maxWidth, maxHeight);
}

// set flag if value is 1, clear flag if value is 0
void igSDLWindowHint(SDL_WindowFlags hint, int value) {
    if (value == 1) {
        sdl_flags = (SDL_WindowFlags)(sdl_flags | hint);
    } else {
        sdl_flags = (SDL_WindowFlags)(sdl_flags & ~hint);
    }
    if (hint == 0) {
        sdl_flags = (SDL_WindowFlags)(0);
    }
}

ImTextureID igCreateTexture(unsigned char *pixels, int width, int height) {
    if (sdl_use_software_renderer) {
        SDL_Texture* texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
        if (texture == NULL) {
            return (ImTextureID)0;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_UpdateTexture(texture, NULL, pixels, width * 4);
        return (ImTextureID)(intptr_t)texture;
    }

    GLint last_texture;
    GLuint texId;

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return ImTextureID((intptr_t(texId)));
}

void igDeleteTexture(ImTextureID id) {
    if (sdl_use_software_renderer) {
        SDL_DestroyTexture((SDL_Texture*)(intptr_t)id);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, (GLuint *)(&id));
}

int igSDLSetSwapInterval(int interval) {
    if (sdl_use_software_renderer) {
        return 0;
    }
    return SDL_GL_SetSwapInterval(interval);
}

void igSetBgColor(ImVec4 color) { sdl_clear_color = color; }

void igSetTargetFPS(unsigned int fps) { sdl_target_fps = fps; }

#endif // CIMGUI_GO_USE_SDL2
