
#include <gl3/gl3.h>
#include <gl3/gl3w.h>

void test_gl()
{
	gl3wInit();

	GLint i = 10;
	i++;

	glCullFace(0);
	glDrawArrays(GL_TRIANGLES, 0, 0);
}

int WINAPI WinMain(HINSTANCE	hInstance,			// Instance
	HINSTANCE	hPrevInstance,		// Previous Instance
	LPSTR		lpCmdLine,			// Command Line Parameters
	int			nCmdShow)			// Window Show State
{
	return 0;
}

