#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <c-util/c-util.h>

void framebuffer_size_callback(GLFWwindow *window, i32 width, i32 height) {
  glViewport(0, 0, width, height);
}

int main() {
  // Initialize GLFW
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  // Configure GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create a window
  GLFWwindow *window = glfwCreateWindow(800, 600, "MinecraftClone", NULL, NULL);
  if (window == NULL) {
    fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize GLAD\n");
    return -1;
  }

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    // Clear the screen with a blue color
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Clean up
  glfwTerminate();
  return 0;
}
