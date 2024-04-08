#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(char const * path);

unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool movementBool = false;
bool lanternBool = false;
bool binocularsBool = false;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    float sharkScale = 2;

    glm::vec3 boatPosition = glm::vec3(60, 0, 0);
    float boatScale = 10;

    glm::vec3 islandPosition = glm::vec3(0, 15, -1000);
    float islandScale = 0.2;

    glm::vec3 lanternPosition = glm::vec3(64, 10.47, 2.7);
    float lanternScale = 0.04;

    PointLight pointLight;
    DirLight dirLight;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader binocularsShader("resources/shaders/binoculars.vs", "resources/shaders/binoculars.fs");
    Shader planeShader("resources/shaders/planeShader.vs", "resources/shaders/planeShader.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    // load models
    // -----------
    Model shark1("resources/objects/shark/shark.obj");
    shark1.SetShaderTextureNamePrefix("material.");

    Model shark2("resources/objects/shark/shark.obj");
    shark2.SetShaderTextureNamePrefix("material.");

    Model boat("resources/objects/boat/boat.obj");
    boat.SetShaderTextureNamePrefix("material.");

    Model island("resources/objects/island/obj.obj");
    island.SetShaderTextureNamePrefix("material.");

    Model lantern("resources/objects/lantern/lantern_obj.obj");
    lantern.SetShaderTextureNamePrefix("material.");

    // binoculars
    float transparentVertices[] = {
            0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  0.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  0.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  1.0f
    };

    unsigned int cameraVAO, cameraVBO;
    glGenVertexArrays(1, &cameraVAO);
    glGenBuffers(1, &cameraVBO);
    glBindVertexArray(cameraVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cameraVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    stbi_set_flip_vertically_on_load(true);
    unsigned int binocularsTexture = loadTexture(FileSystem::getPath("resources/textures/binoculars.png").c_str());
    binocularsShader.use();
    binocularsShader.setInt("texture1",0);


    // ocean
    float planeVertices[] = {
            //      vertex           texture        normal
            50.0f, -1.0f,  50.0f,  2.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -50.0f, -1.0f, -50.0f,  0.0f, 2.0f, 0.0f, 1.0f, 0.0f,
            -50.0f, -1.0f,  50.0f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f,


            50.0f, -1.0f,  50.0f,  2.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            50.0f, -1.0f, -50.0f,  2.0f, 2.0f, 0.0f, 1.0f, 0.0f,
            -50.0f, -1.0f, -50.0f,  0.0f, 2.0f, 0.0f, 1.0f, 0.0f
    };

    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);

    glBindVertexArray(planeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

    //load ocean and gold
    unsigned int oceanTexture = loadTexture("resources/textures/ocean.jpg");
    planeShader.use();
    planeShader.setInt("texture1", 0);

    glBindVertexArray(0);


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(64, 10.47, 2.7);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 15.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(-2.0f, -1.0f, 0);
    dirLight.ambient = glm::vec3(0.35f, 0.35f, 0.35f);
    dirLight.diffuse = glm::vec3(1, 0.8, 0.1);
    dirLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };
    //skybox
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    vector<std::string> faces
            {
                    FileSystem::getPath("/resources/textures/skybox/tropic_rt.jpg"),
                    FileSystem::getPath("/resources/textures/skybox/tropic_lf.jpg"),
                    FileSystem::getPath("/resources/textures/skybox/tropic_up.jpg"),
                    FileSystem::getPath("/resources/textures/skybox/tropic_dn.jpg"),
                    FileSystem::getPath("/resources/textures/skybox/tropic_ft.jpg"),
                    FileSystem::getPath("/resources/textures/skybox/tropic_bk.jpg")
            };


    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox",0);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ocean
        planeShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
        planeShader.setMat4("view", view);
        planeShader.setMat4("projection", projection);
        glBindVertexArray(planeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, oceanTexture);
        model = glm::translate(model, glm::vec3(0.0f, 103.6f, 0.0f));
        model = glm::scale(model, glm::vec3(100));
        planeShader.setMat4("model", model);
        planeShader.setVec3("dirLight.direction", dirLight.direction);
        planeShader.setVec3("dirLight.ambient", dirLight.ambient);
        planeShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        planeShader.setVec3("dirLight.specular", dirLight.specular);
        planeShader.setFloat("shininess", 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        // directional light
        ourShader.setVec3("dirLight.direction", dirLight.direction);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 10000.0f);
        view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded model

        // boat
        glm::mat4 boatModel = glm::mat4(1.0f);
        boatModel = glm::translate(boatModel, programState->boatPosition);
        boatModel = glm::scale(boatModel, glm::vec3(programState->boatScale));
        boatModel = glm::rotate(boatModel, glm::radians(180.0f), glm::vec3 (.0, 1.0f, 0.0f));
        ourShader.setMat4("model", boatModel);
        boat.Draw(ourShader);

        // lantern
        glm::mat4 lanternModel = glm::mat4(1.0f);
        lanternModel = glm::translate(lanternModel, programState->lanternPosition);
        lanternModel = glm::scale(lanternModel, glm::vec3(programState->lanternScale));
        ourShader.setMat4("model", lanternModel);
        lantern.Draw(ourShader);

        if(lanternBool){
            pointLight.ambient = glm::vec3(20.0, 20.0, 20.0);
        }else{
            pointLight.ambient = glm::vec3(0.0, 0.0, 0.0);
        }

        // island
        glm::mat4 islandModel = glm::mat4(1.0f);
        islandModel = glm::translate(islandModel, programState->islandPosition);
        islandModel = glm::scale(islandModel, glm::vec3(programState->islandScale));
        ourShader.setMat4("model", islandModel);
        island.Draw(ourShader);

        // shark1
        glm::mat4 shark1Model = glm::mat4(1.0f);
        glm::vec3 rotationCenter = glm::vec3(60, 0, 0);
        glm::mat4 translateToOrigin = glm::translate(glm::mat4(1.0f), -rotationCenter);
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), (float) currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 translateBack = glm::translate(glm::mat4(1.0f), rotationCenter);
        shark1Model = translateBack * rotationMatrix * translateToOrigin;
        shark1Model = glm::scale(shark1Model, glm::vec3(programState->sharkScale));
        ourShader.setMat4("model", shark1Model);
        shark1.Draw(ourShader);

        // shark2
        glm::mat4 shark2Model = glm::mat4(1.0f);
        rotationCenter = glm::vec3(60, 0, 0);
        translateToOrigin = glm::translate(glm::mat4(1.0f), rotationCenter);
        rotationMatrix = glm::rotate(glm::mat4(1.0f), (float) currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
        translateBack = glm::translate(glm::mat4(1.0f), rotationCenter);
        shark2Model = translateBack * rotationMatrix * translateToOrigin;
        shark2Model = glm::scale(shark2Model, glm::vec3(programState->sharkScale));
        shark2Model = glm::rotate(shark2Model, glm::radians(180.0f), glm::vec3(0.0f,1.0f,0.0f));
        ourShader.setMat4("model", shark2Model);
        shark2.Draw(ourShader);

        // binoculars
        if(binocularsBool) {
            binocularsShader.use();
            programState->camera.Zoom = 20.0f;
            projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                          (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 30000.0f);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-1.0f,0.0f,-0.5));
            model = glm::scale(model, glm::vec3(2.0f));


            binocularsShader.setMat4("model", model);

            glBindVertexArray(cameraVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, binocularsTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }else{
            programState->camera.Zoom = 45.0f;
        }

        //skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        //skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && movementBool)
        programState->camera.ProcessKeyboard(FORWARD, 0.5);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && movementBool)
        programState->camera.ProcessKeyboard(BACKWARD, 0.5);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && movementBool)
        programState->camera.ProcessKeyboard(LEFT, 0.5);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && movementBool)
        programState->camera.ProcessKeyboard(RIGHT, 0.5);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (key == GLFW_KEY_V && action == GLFW_PRESS) {
        movementBool = !movementBool;
    }
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        lanternBool = !lanternBool;
    }
    if (key == GLFW_KEY_B && action == GLFW_PRESS) {
        binocularsBool = !binocularsBool;
    }
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RED;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
