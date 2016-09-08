#ifndef __VAPP_H__
#define __VAPP_H__

#include "vgl.h"

class VermilionApplication
{
protected:
    inline VermilionApplication(void) {}
    virtual ~VermilionApplication(void) {}

    static VermilionApplication * s_app;
    GLFWwindow* m_pWindow;

#ifdef _WIN32
    ULONGLONG       m_appStartTime;
#else
    struct timeval  m_appStartTime;
#endif

    static void window_size_callback(GLFWwindow* window, int width, int height);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void char_callback(GLFWwindow* window, unsigned int codepoint);
    unsigned int app_time();

#ifdef _DEBUG
    static void APIENTRY DebugOutputCallback(GLenum source,
                                             GLenum type,
                                             GLuint id,
                                             GLenum severity,
                                             GLsizei length,
                                             const GLchar* message,
                                             GLvoid* userParam);
#endif

public:
    void MainLoop(void);

    virtual void Initialize(const char * title = 0);

    virtual void Display(bool auto_redraw = true)
    {
        glfwSwapBuffers(m_pWindow);
    }

    virtual void Finalize(void) {}

    virtual void Resize(int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    virtual void OnKey(int key, int scancode, int action, int mods) { /* NOTHING */ }
    virtual void OnChar(unsigned int codepoint) { /* NOTHING */ }
};

#define BEGIN_APP_DECLARATION(appclass)                     \
class appclass : public VermilionApplication                \
{                                                           \
public:                                                     \
    typedef class VermilionApplication base;                \
    static VermilionApplication * Create(void)              \
    {                                                       \
        return (s_app = new appclass);                      \
    }

#define END_APP_DECLARATION()                               \
};

#ifdef _DEBUG
//*
#define DEBUG_OUTPUT_CALLBACK                                                   \
void APIENTRY VermilionApplication::DebugOutputCallback(GLenum source,          \
                                                         GLenum type,           \
                                                         GLuint id,             \
                                                         GLenum severity,       \
                                                         GLsizei length,        \
                                                         const GLchar* message, \
                                                         GLvoid* userParam)     \
{                                                                               \
    OutputDebugStringA(message);                                                \
    OutputDebugStringA("\n");                                                   \
}
/*/
#define DEBUG_OUTPUT_CALLBACK                                                   \
void APIENTRY VermilionApplication::DebugOutputCallback(GLenum source,         \
                                                         GLenum type,           \
                                                         GLuint id,             \
                                                         GLenum severity,       \
                                                         GLsizei length,        \
                                                         const GLchar* message, \
                                                         GLvoid* userParam)     \
{                                                                               \
    printf("Debug Message: SOURCE(0x%04X), "\
                          "TYPE(0x%04X), "\
                          "ID(0x%08X), "\
                          "SEVERITY(0x%04X), \"%s\"\n",\
           source, type, id, severity, message);\
}
*/
#else
#define DEBUG_OUTPUT_CALLBACK
#endif

#ifdef _WIN32
#define MAIN_DECL int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
#else
#define MAIN_DECL int main(int argc, char ** argv)
#endif

#define DEFINE_APP(appclass,title)                          \
VermilionApplication * VermilionApplication::s_app;         \
                                                            \
void VermilionApplication::MainLoop(void)                   \
{                                                           \
    do                                                      \
    {                                                       \
        Display();                                          \
        glfwPollEvents();                                   \
    } while (!glfwWindowShouldClose(m_pWindow));            \
}                                                           \
                                                            \
MAIN_DECL                                                   \
{                                                           \
    VermilionApplication * app = appclass::Create();        \
                                                            \
    app->Initialize(title);                                 \
    app->MainLoop();                                        \
    app->Finalize();                                        \
                                                            \
    return 0;                                               \
}                                                           \
                                                            \
DEBUG_OUTPUT_CALLBACK

#endif /* __VAPP_H__ */
