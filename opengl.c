#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/freeglut.h>
#include <math.h>
#include <time.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <inttypes.h>
#include <unistd.h>

int msleep(unsigned int tms) {
  return usleep(tms * 1000);
}

#define GLEW_STATIC

#define CheckedGLCall(x) do { PrintOpenGLErrors(">>BEFORE<< "#x, __FILE__, __LINE__); (x); PrintOpenGLErrors(#x, __FILE__, __LINE__); } while (0)
#define CheckedGLResult(x) (x); PrintOpenGLErrors(#x, __FILE__, __LINE__);
#define CheckExistingErrors(x) PrintOpenGLErrors(">>BEFORE<< "#x, __FILE__, __LINE__);

int WIDTH = 1600;
int HEIGHT = 800;
float SPEED = 0.5;
float FOV = 100.0;
float rotate_y = 0.0f;
float rotate_x = 0.0f;
float rotate_z = 0.0f;
float camX = 0;
float camY = 0;
float camZ = -2;
int MOVEX = 0;
int MOVEY = 0;
int VERT = 0;
int PAUSE = 1;
float R2DEG = (180.0f/M_PI);
float DEG2R = (M_PI/180.0f);
double MOUSEX,MOUSEY;
float SENSITIVITY = 0.5;
int FPS = 1000;
GLFWwindow* window;
GLuint shaderProgram;
GLuint VBO, VAO;
//Matrix *MAT;
//g++ opengl.c -g -o a -lfreeglut -lopengl32 -lglu32 -lm -lglew32 -lglfw3
//cd C:/Users/fazam/OneDrive/Programming/C/OpenGL

typedef struct pointClass{
    float x;
    float y;
    float z;
    float w;
} Point;

//Rewrite point-list class as addNode is taking too long

typedef struct pointListStruct{
    Point* l;
    int index;
    int size;
} PointList;

typedef struct objClass{
    Point* pos;
    Point* rotation;
    Point* points;
    Point* faces;
    PointList* colors;
    Point* u_v;
    int vertexCount;
    int faceCount;
} Obj;
//Obj file starts from 1

void print(Point*p){
    printf("%f,%f,%f,%f\n",p->x,p->y,p->z,p->w);
}

PointList * createPointList(){
    PointList* n = (PointList*)malloc(sizeof(PointList));
    n->size = 2;
    n->index = 0;
    n->l = (Point*) calloc(n->size,sizeof(Point));
    return n;
}

Point * add(Point* p1,Point* p2){
    Point* n = (Point*)malloc(sizeof(Point));
    n->x = p1->x+p2->x;
    n->y = p1->y+p2->y;
    n->z = p1->z+p2->z;
    n->w = p1->w+p2->w;
    return n;
}


Point * scale(Point* p,float d){
    Point* n = (Point*)malloc(sizeof(Point));
    n->x = p->x*d;
    n->y = p->y*d;
    n->z = p->z*d;
    n->w = p->w*d;
    return n;
}

void addPoint(PointList*h,Point* p){
    if (h->index < h->size){
        h->l[h->index] = *p;
    }else{
        Point* temp = (Point*)calloc(h->size,sizeof(Point));
        memcpy(temp,h->l,h->index*sizeof(Point));
        free(h->l);
        h->size *= 2;
        h->l = (Point*)calloc(h->size,sizeof(Point));
        memcpy(h->l,temp,h->index*sizeof(Point));
        free(temp);
        h->l[h->index] = *p;
    }
    h->index += 1;
}

void PrintOpenGLErrors(char const * const Function, char const * const File, int const Line)
{
	bool Succeeded = true;

	GLenum Error = glGetError();
	if (Error != GL_NO_ERROR)
	{
		char const * ErrorString = (char const *) gluErrorString(Error);
		if (ErrorString)
			std::cerr << ("OpenGL Error in %s at line %d calling function %s: '%s'", File, Line, Function, ErrorString) << std::endl;
		else
			std::cerr << ("OpenGL Error in %s at line %d calling function %s: '%d 0x%X'", File, Line, Function, Error, Error) << std::endl;
	}
}

void PrintShaderInfoLog(GLint const Shader)
{
	int InfoLogLength = 0;
	int CharsWritten = 0;

	glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, & InfoLogLength);

	if (InfoLogLength > 0)
	{
		GLchar * InfoLog = new GLchar[InfoLogLength];
		glGetShaderInfoLog(Shader, InfoLogLength, & CharsWritten, InfoLog);
		std::cout << "Shader Info Log:" << std::endl << InfoLog << std::endl;
		delete [] InfoLog;
	}
}

Point* clone(Point* p){
    Point* n = (Point*)malloc(sizeof(Point));
    n->x = p->x;
    n->y = p->y;
    n->z = p->z;
    n->w = p->w;

    return n;
}

Obj* creatObj(char* fileName){
    Obj* nObj = (Obj *)malloc(sizeof(Obj));
    PointList* pHead = createPointList();
    PointList* pFace = createPointList();
    PointList* colors= createPointList();
    PointList* uvPoint= createPointList();
    Point* p = (Point *)malloc(sizeof(Point));
    Point* f = (Point *)malloc(sizeof(Point));
    Point* color = (Point *)malloc(sizeof(Point));
    char* fName = (char*)calloc(200,sizeof(char));
    char colorNames[80][100];
    strcat(fName,fileName);
    strcat(fName,"tinker.obj");
    FILE *file = fopen(fName,"r");
    int MODE = 0;
    int fCount = 0;
    int pCount = 0;
    int currentCol;

    while (!feof(file)){
        //Default state: Scan for other terms
        if (MODE == 0){
            char c = fgetc(file);
            if (c == 'v'){
                MODE = 1;
            }
            else if (c == 'f'){
                MODE = 2;
            }else if(c == '#' || c=='g'){
                MODE = -1;
            }else if (c == 'm'){
                MODE = 3;
            }else if (c == 'u'){
                MODE = 4;
            }
            //printf("%c\n",c);
        }
        //Ignore comments and other stuff
        if (MODE == -1){
            char c2 = fgetc(file);
            while (c2 != '\n'){
                c2 = fgetc(file);
            }
            MODE = 0;
        }
        //Add vertice
        if (MODE == 1){
            //use atof to convert char* to float
            fscanf(file,"%f",&(p->x));
            fscanf(file,"%f",&(p->y));
            fscanf(file,"%f",&(p->z));

            // print(p);
            pCount++;
            addPoint(pHead,p);
            MODE = 0;
            // print(p);
            p = (Point *)malloc(sizeof(Point));
        }
        //Add triangle
        if (MODE == 2){
            int t;
            fscanf(file,"%d",&t);
            f->x = (float)t;
            fscanf(file,"%d",&t);
            f->y = (float)t;
            fscanf(file,"%d",&t);
            f->z = (float)t;
            // print(f);
            fCount++;
            addPoint(pFace,f);

            addPoint(uvPoint,clone(&(colors->l[currentCol])));
            MODE = 0;
            f = (Point *)malloc(sizeof(Point));
        }
        //Parse mtl file
        if (MODE == 3){
            char* name=(char*)calloc(100,sizeof(char));
            fscanf(file,"%s",&name[0]);
            fscanf(file,"%s",&name[0]);

            char* mtlName = (char*)calloc(200,sizeof(char));
            strcat(mtlName,fileName);
            strcat(mtlName,name);

            FILE *mtl = fopen(mtlName,"r");
            int MMODE = 0;

            while(!feof(mtl)){
                //Default state:Scan for other terms
                if (MMODE == 0){
                    char c;
                    c = fgetc(mtl);
                    // printf("%c\n",c);
                    if (c == '#'){
                        MMODE = -1;
                    }else if (c=='n'){
                        MMODE = 1;
                    }else if (c=='K'){
                        MMODE = 2;
                    }else if (c=='d'){
                        MMODE = 3;
                    }
                //Ignore comments and other stuff
                }else if (MMODE == -1){
                    char c2 = fgetc(mtl);
                    while (c2 != '\n'){
                        c2 = fgetc(mtl);
                    }
                    MMODE = 0;
                //Add color name
                }else if (MMODE == 1){
                    char* colName=(char*)calloc(80,sizeof(char));
                    fscanf(mtl,"%s",&colName[0]);
                    fscanf(mtl,"%s",&colName[0]);
                    //
                    // printf("%d:%s\n",colors->index,colName);
                    strcpy((char*)&colorNames[colors->index],colName);

                    MMODE = 0;
                //Add color rgb
                }else if (MMODE == 2){
                    char c2 = fgetc(mtl);
                    if (c2 == 'd'){
                        fscanf(mtl,"%f",&(color->x));
                        fscanf(mtl,"%f",&(color->y));
                        fscanf(mtl,"%f",&(color->z));
                        // print(color);
                        addPoint(colors,color);
                        // print(color);
                        color = (Point *)malloc(sizeof(Point));
                        MMODE = 0;
                    }else{
                        MMODE = -1;
                    }
                }
                else if (MMODE == 3){
                    float alph = 0;
                    fscanf(mtl,"%f",&(alph));
                    colors->l[colors->index-1].w = alph;
                    MMODE = 0;
                }
            }

            MODE = 0;
        }
        //Set current color
        if (MODE == 4){
            char* color=(char*)calloc(80,sizeof(char));
            fscanf(file,"%s",&color[0]);
            fscanf(file,"%s",&color[0]);
            currentCol = 0;
            for(int i = 0; i<colors->index;i++){
                // printf("%s\n",colorNames[i]);
                if (strcmp(color,colorNames[i]) == 0){
                    currentCol = i;
                    i = colors->index;
                    // printf("Color \"%s\" found\n",color);
                }
            }
            // printf("%s\n",color);
            MODE = 0;
        }
    }

    nObj->pos = (Point*)malloc(sizeof(Point));
    nObj->pos->x = 0;
    nObj->pos->y = 0;
    nObj->pos->z = 0;

    Point* vertices = pHead->l;
    Point* faces = pFace->l;

    nObj->points = (Point*)calloc(fCount*3,sizeof(Point));
    nObj->faces = (Point*)calloc(fCount,sizeof(Point));

    int k = 0;
    for(int i = 0;i<fCount*3;i+=3){
        // print(c->p);
        nObj->points[i] = vertices[(int)(faces[k].x-1)];
        nObj->points[i+1] = vertices[(int)(faces[k].y-1)];
        nObj->points[i+2] = vertices[(int)(faces[k].z-1)];
        // print(&(nObj->points[i]));
        // print(&(nObj->points[i+1]));
        // print(&(nObj->points[i+2]));
        // printf("\n");
        k++;
    }
    // printf("-----------------------\n");

    nObj->faceCount = fCount;
    nObj->vertexCount = fCount*3;
    nObj->u_v = (Point*)calloc(fCount,sizeof(Point));

    for(int i = 0;i<fCount-1;i++){
        nObj->u_v[i] = uvPoint->l[i];
    }

    return nObj;
}

float* addObj(float* vertices, int length, Obj* o,int* nLength){
    float* newV = (float*)calloc(length+(o->faceCount)*21,sizeof(float));
    memcpy(newV,vertices,length*sizeof(float));
    int k = 0;
    int f = 0;

    for (int i = 0;i<(o->faceCount)*21;i+=7){
        newV[length+i] = o->points[k].x;
        newV[length+i+1] = o->points[k].y;
        newV[length+i+2] = o->points[k].z;
        newV[length+i+3] = o->u_v[f].x;
        newV[length+i+4] = o->u_v[f].y;
        newV[length+i+5] = o->u_v[f].z;
        newV[length+i+6] = o->u_v[f].w;
        // printf("%d: %f, %f, %f, %f\n",k,newV[length+i+3],newV[length+i+4],newV[length+i+5],newV[length+i+6]);
        k++;
        if (k%3==0){
            f++;
        }
    }

    *nLength = (int)((o->faceCount)*21+length);
    // for(int i = 0;i<*nLength;i++){
    //     if (i%3==0){
    //         printf("\n");
    //     }
    //     printf("%f ",newV[i]);
    // }

    printf("%d Triangles, %d Vertices\n", (o->faceCount),(o->faceCount)*3);
    printf("%d Floats\n",*nLength);

    return newV;
}



void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods){
    if (action == 0){
        switch (key) {
            case GLFW_KEY_W:
            case GLFW_KEY_S:
                MOVEY = 0;
                break;
            case GLFW_KEY_A:
            case GLFW_KEY_D:
                MOVEX = 0;
                break;
            case GLFW_KEY_SPACE:
            case GLFW_KEY_LEFT_CONTROL:
                VERT = 0;
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);
                break;
        }
    }else{
        switch (key) {
            case GLFW_KEY_W:
                MOVEY = 1;
                break;
            case GLFW_KEY_S:
                MOVEY = -1;
                break;
            case GLFW_KEY_A:
                MOVEX = -1;
                break;
            case GLFW_KEY_D:
                MOVEX = 1;
                break;
            case GLFW_KEY_SPACE:
                VERT = -1;
                break;
            case GLFW_KEY_LEFT_CONTROL:
                VERT = 1;
                break;
        }
        if (action == 1 && key == GLFW_KEY_ESCAPE){
            if (PAUSE){
                PAUSE = 0;
            }else{
                PAUSE = 1;
            }
        }
    }
    // printf("%lf\n",rotate_x);
}

void mouseMoved(GLFWwindow* window, double xpos, double ypos)
{
    MOUSEX = xpos;
    MOUSEY = ypos;
}

void updateCamera(){
    if (abs(rotate_x)>90){
        if (rotate_x > 0)
            rotate_x = 90;
        else
            rotate_x = -90;
    }
    if(!PAUSE){
        if (MOVEY == -1){
            camX -= SPEED * sin(glm::radians(rotate_y));
            camZ -= SPEED * cos(glm::radians(rotate_y));
        }else if(MOVEY == 1){
            camX += SPEED * sin(glm::radians(rotate_y));
            camZ += SPEED * cos(glm::radians(rotate_y));
        }
        if (MOVEX == -1){
            camX += SPEED * sin(glm::radians(rotate_y + 90));
            camZ += SPEED * cos(glm::radians(rotate_y + 90));
        }else if(MOVEX == 1){
            camX += SPEED * sin(glm::radians(rotate_y - 90));
            camZ += SPEED * cos(glm::radians(rotate_y - 90));
        }
        if (VERT == -1){
            camY -= SPEED;
        }else if (VERT == 1){
            camY += SPEED;
        }
        rotate_y -= (MOUSEX - WIDTH/2)*SENSITIVITY;
        rotate_x -= (MOUSEY - HEIGHT/2)*SENSITIVITY;

        glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else{
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void draw(float vertices[], int length){
    // Clear the screen
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix = glm::rotate(rotation_matrix, rotate_x, glm::vec3(1.0f, 0.0f, 0.0f));
    rotation_matrix = glm::rotate(rotation_matrix, rotate_y, glm::vec3(0.0f, 1.0f, 0.0f));
    rotation_matrix = glm::rotate(rotation_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 projection = glm::perspective(glm::radians(FOV), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);

    // get the location of the projection matrix uniform in the shader
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // set the value of the projection matrix uniform in the shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Define camera position and orientation
    glm::vec3 cameraPosition(camX, camY, camZ);

    // Apply rotation matrice
    glm::mat4 rotationMatrix = glm::mat4(1.0f);
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotate_x), glm::vec3(1.0f, 0.0f, 0.0f));

    // Apply translation matrix
    glm::mat4 translationMatrix = glm::mat4(1.0f);
    translationMatrix = glm::translate(translationMatrix, -cameraPosition);

    // Combine rotation and translation matrices to form the view matrix
    glm::mat4 viewMatrix = translationMatrix*rotationMatrix;
    GLuint viewMatrixLoc = glGetUniformLocation(shaderProgram,"viewMatrix");
    // Set the view matrix as a uniform in the shader
    glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::inverse(viewMatrix)));

    // glUniform1i(glGetUniformLocation(shaderProgram,"length"), length);

    // Use the shader program
    glUseProgram(shaderProgram);

    // Draw the triangle
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, length);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}

int main(int argc, char **argv) {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW window
    window = glfwCreateWindow(WIDTH, HEIGHT, "Ray Tracer", NULL, NULL);
    if (window == NULL)
    {
        printf("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouseMoved);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        printf("Failed to initialize GLEW");
        return -1;
    }
    glfwSetKeyCallback(window, keyboard);
    // Define the vertices of the triangle
    float vert[] = {};

    Obj* o = creatObj("Obj Files/House/");
    int vertCount;


    float* vertices = addObj((float*)&vert,0,o,&vertCount);
    // o = creatObj("Obj Files/Car2/");
    // vertices = addObj(vert,vert,o,&vertCount);
    // printf("%d\n",vertCount);

    // for (int i = 0; i< vertCount;i+=7){
    //     printf("%f %f %f %f\n",vertices[i+3],vertices[i+4],vertices[i+5],vertices[i+6]);
    // }

    char const * vertexShaderSource = R"GLSL(
        #version 330 core

        in vec3 aPos; // the position variable has attribute position 0
        in vec4 color;

        uniform mat4 viewMatrix;  // the projection matrix uniform-
        uniform mat4 projection;  // the projection matrix uniform-
        // uniform int length;  // the projection matrix uniform-

        out vec4 vertexColor; // specify a color output to the fragment shader

        void main()
        {
            gl_Position = projection * viewMatrix * vec4(aPos, 1.0); // see how we directly give a vec3 to vec4's constructor
            // vertexColor = color;
            vertexColor=color.rgba;
        }
    )GLSL";

    char const * fragmentShaderSource = R"GLSL(
        #version 330 core
        out vec4 FragColor;
        in vec4 vertexColor; // the input variable from the vertex shader (same name and same type)

        // uniform int textureLength;
        // uniform vec3 colors[1];

        void main()
        {
            float depth = gl_FragCoord.z / gl_FragCoord.w/100;
            float depth_intensity = 1.0 - depth;
            FragColor = vec4((1-depth)*vertexColor.rgb,0) + vec4(0,0,0,vertexColor.a);
            // vec3 randomColor = vec3(fract(sin(dot(gl_FragCoord.xyz + seed, vec3(12.9898, 78.233, 45.5432))) * 43758.5453));
            // FragColor = vec4(randomColor, 1.0);
        }
    )GLSL";

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create a vertex buffer object (VBO) and vertex array object (VAO)
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Compile and link the shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint linkStatus;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE)
    {
        // Linking failed, retrieve the error log
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("Shader program linking failed: %s\n", infoLog);
    }

    // Copy the vertex data to the VBO
    glBufferData(GL_ARRAY_BUFFER, vertCount*sizeof(GL_FLOAT), vertices, GL_STATIC_DRAW);

    GLuint positionLocation = glGetAttribLocation(shaderProgram, "aPos");
    GLuint colorLocation = glGetAttribLocation(shaderProgram, "color");
    printf("%d %d\n",positionLocation,colorLocation);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,7*sizeof(GL_FLOAT), (void*)(0 * sizeof(GL_FLOAT)));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,7*sizeof(GL_FLOAT), (void*)(4 * sizeof(GL_FLOAT)));


    // Delete the shader objects
    glDeleteShader(fragmentShader);
    glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        updateCamera();
        draw(vertices,vertCount);
        msleep(1000.0/FPS);
    }

    // Delete the VAO and VBO
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    // Terminate GLFW
    glfwTerminate();

    return 0;
}
