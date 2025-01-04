#ifdef WIN32
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include "imgui.h"

#include "logger.h"

#ifdef __ANDROID__
#include <GLES3/gl32.h>
#include "imgui_impl_sdl_es2.h"
#include "imgui_impl_sdl_es3.h"
#include <unistd.h>
#include <dirent.h>
#else
#include "GL/glew.h"
#include "imgui_impl_sdl_gl3.h"
#endif
#include "renderer.h"
//
//#include <unistd.h>
//#include <dirent.h>

/**
 * A convenience function to create a context for the specified window
 * @param w Pointer to SDL_Window
 * @return An SDL_Context value
 */

typedef bool(initImgui_t)(SDL_Window*);
typedef bool(processEvent_t)(SDL_Event*);
typedef void(newFrame_t)(SDL_Window*);
typedef void(shutdown_t)();

static initImgui_t *initImgui;
static processEvent_t *processEvent;
static newFrame_t *newFrame;
static shutdown_t *shutdown;

static SDL_GLContext createCtx(SDL_Window *w)
{
    // Prepare and create context
#ifdef __ANDROID__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GLContext ctx = SDL_GL_CreateContext(w);

    if (!ctx)
    {
        Log(LOG_ERROR) << "Could not create context! SDL reports error: " << SDL_GetError();
        return ctx;
    }

    int major, minor, mask;
    int r, g, b, a, depth;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

    SDL_GL_GetAttribute(SDL_GL_RED_SIZE,   &r);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE,  &b);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);

    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);

    const char* mask_desc;

    if (mask & SDL_GL_CONTEXT_PROFILE_CORE) {
        mask_desc = "core";
    } else if (mask & SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) {
        mask_desc = "compatibility";
    } else if (mask & SDL_GL_CONTEXT_PROFILE_ES) {
        mask_desc = "es";
    } else {
        mask_desc = "?";
    }

    Log(LOG_INFO) << "Got context: " << major << "." << minor << mask_desc
                  << ", R" << r << "G" << g << "B" << b << "A" << a << ", depth bits: " << depth;

    SDL_GL_MakeCurrent(w, ctx);
#ifdef __ANDROID__
    if (major == 3)
    {
        Log(LOG_INFO) << "Initializing ImGui for GLES3";
        initImgui = ImGui_ImplSdlGLES3_Init;
        Log(LOG_INFO) << "Setting processEvent and newFrame functions appropriately";
        processEvent = ImGui_ImplSdlGLES3_ProcessEvent;
        newFrame = ImGui_ImplSdlGLES3_NewFrame;
        shutdown = ImGui_ImplSdlGLES3_Shutdown;
    }
    else
    {
        Log(LOG_INFO) << "Initializing ImGui for GLES2";
        initImgui = ImGui_ImplSdlGLES2_Init;
        Log(LOG_INFO) << "Setting processEvent and newFrame functions appropriately";
        processEvent = ImGui_ImplSdlGLES2_ProcessEvent;
        newFrame = ImGui_ImplSdlGLES2_NewFrame;
        shutdown = ImGui_ImplSdlGLES2_Shutdown;
    }
#else
    initImgui = ImGui_ImplSdlGL3_Init;
    processEvent = ImGui_ImplSdlGL3_ProcessEvent;
    newFrame = ImGui_ImplSdlGL3_NewFrame;
    shutdown = ImGui_ImplSdlGL3_Shutdown;
#endif
    Log(LOG_INFO) << "Finished initialization";
    Log(LOG_INFO) << "Rendering on " << glGetString(GL_RENDERER);
    return ctx;
}


int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

#ifdef __ANDROID__
    if (argc < 2)
    {
        Log(LOG_FATAL) << "Not enough arguments! Usage: " << argv[0] << " path_to_data_dir";
        SDL_Quit();
        return 1;
    }
    if (chdir(argv[1])) {
        Log(LOG_ERROR) << "Could not change directory properly!";
    } else {
        dirent **namelist;
        int numdirs = scandir(".", &namelist, NULL, alphasort);
        if (numdirs < 0) {
            Log(LOG_ERROR) << "Could not list directory";
        } else {
            for (int dirid = 0; dirid < numdirs; ++dirid) {
                Log(LOG_INFO) << "Got file: " << namelist[dirid]->d_name;
            }
            free(namelist);
        }
    }
#endif
    // Create window
    Log(LOG_INFO) << "Creating SDL_Window";
    SDL_Window *window = SDL_CreateWindow("Shade Order Tester", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_GLContext ctx = createCtx(window);
#ifndef __ANDROID__
    if (glewInit() != GLEW_OK) {
        Log(LOG_FATAL) << "Cannot init glew.";
        return 1;
    }
#endif
    initImgui(window);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 32.0f);

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(0, 0, 0);

    Log(LOG_INFO) << "Entering main loop";
    {

        bool done = false;

        Renderer renderer;
        if (!renderer.init())
            return -1;

        int deltaX = 0, deltaY = 0;
        int prevX , prevY;
        SDL_GetMouseState(&prevX, &prevY);
        float frag_percentage = 0;
        float frag_speed = 1;
        int tri_count = 200;
        bool auto_increment = false;
        while (!done) {
            SDL_Event e;

            deltaX = 0;
            deltaY = 0;

            float deltaZoom = 0.0f;

            while (SDL_PollEvent(&e)) {
                bool handledByImGui = processEvent(&e);
                {
                    switch (e.type) {
                        case SDL_QUIT:
                            done = true;
                            break;
                        case SDL_MOUSEBUTTONDOWN:
                            prevX = e.button.x;
                            prevY = e.button.y;
                            break;
                        case SDL_MOUSEMOTION:
                            if (e.motion.state & SDL_BUTTON_LMASK) {
                                deltaX += prevX - e.motion.x;
                                deltaY += prevY - e.motion.y;
                                prevX = e.motion.x;
                                prevY = e.motion.y;
                            }
                            break;
                        case SDL_MULTIGESTURE:
                            if (e.mgesture.numFingers > 1) {
                                deltaZoom += e.mgesture.dDist * 10.0f;
                            }
                            break;
                        case SDL_MOUSEWHEEL:
                            deltaZoom += e.wheel.y / 100.0f;
                            break;
                        default:
                            break;
                    }
                }
            }
            if (io.WantTextInput) {
                SDL_StartTextInput();
            } else {
                SDL_StopTextInput();
            }
            newFrame(window);

            {
                ImGui::Begin("Controls");

                ImGui::SetWindowFontScale(3);

                ImGui::Text("glGetString(GL_VENDOR): \"%s\"", glGetString(GL_VENDOR));
                ImGui::Text("glGetString(GL_RENDERER): \"%s\"", glGetString(GL_RENDERER));
                ImGui::Text("glGetString(GL_VERSION): \"%s\"", glGetString(GL_VERSION));

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                            ImGui::GetIO().Framerate);

                ImGui::ColorEdit3("clear color", (float *) &clear_color);
//                ImGui::SliderInt("Triangle Count", &renderer.triangle_count, 0, 100);
                ImGui::SliderFloat("Rendered/Screen %", &frag_percentage, 0, 500);
                if (ImGui::Button("0%")) {
                    frag_percentage = 0;
                }
                ImGui::SameLine();
                if (ImGui::Button("100%")) {
                    frag_percentage = 100;
                }

                ImGui::Checkbox("Auto Increment", &auto_increment);
                if (auto_increment) {
                    ImGui::SliderFloat("ppf", &frag_speed, 0, 10, "%.1f%");
                    frag_percentage += frag_speed;
                }

                ImGui::SliderInt("Tris", &tri_count, 0, 200);

                float percentage = frag_percentage;
                GLint viewport[4];
                glGetIntegerv(GL_VIEWPORT, viewport);

                renderer.frag_count = (uint32_t)
                        (viewport[2] * viewport[3] *
                         percentage / 100.0 * renderer.triangle_count / 2);

                ImGui::Text("Frag Count <= %d\n", renderer.frag_count);

                ImGui::End();
            }


            // Rendering
            glViewport(0, 0, (int) ImGui::GetIO().DisplaySize.x, (int) ImGui::GetIO().DisplaySize.y);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderer.draw();
            renderer.drawRandomTris(tri_count);

//            ImGui::Begin("Time");
//            ImGui::Text("%llu ms", ms);
//            ImGui::End();

            ImGui::Render();
            SDL_GL_SwapWindow(window);
        }
    }
    shutdown();
    SDL_GL_DeleteContext(ctx);
    SDL_Quit();
    return 0;
}
