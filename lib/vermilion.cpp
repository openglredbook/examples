#include "vapp.h"

void VermilionApplication::window_size_callback(GLFWwindow* window, int width, int height)
{
    VermilionApplication* pThis = (VermilionApplication*)glfwGetWindowUserPointer(window);

    pThis->Resize(width, height);
}

void VermilionApplication::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    VermilionApplication* pThis = (VermilionApplication*)glfwGetWindowUserPointer(window);

    pThis->OnKey(key, scancode, action, mods);
}

void VermilionApplication::char_callback(GLFWwindow* window, unsigned int codepoint)
{
    VermilionApplication* pThis = (VermilionApplication*)glfwGetWindowUserPointer(window);

    pThis->OnChar(codepoint);
}

unsigned int VermilionApplication::app_time()
{
    return (unsigned int) (glfwGetTime() * 1000.0);
}

void VermilionApplication::Initialize(const char * title)
{
    glfwInit();

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    m_pWindow = glfwCreateWindow(800, 600, title ? title : "OpenGL Application", nullptr, nullptr);
    glfwSetWindowUserPointer(m_pWindow, this);
    glfwSetWindowSizeCallback(m_pWindow, window_size_callback);
    glfwSetKeyCallback(m_pWindow, key_callback);
    glfwSetCharCallback(m_pWindow, char_callback);

    glfwMakeContextCurrent(m_pWindow);

    gl3wInit();

    Resize(800, 600);

#ifdef _DEBUG
    if (glDebugMessageCallbackARB != NULL)
        glDebugMessageCallbackARB(DebugOutputCallback, this);
#endif
}
