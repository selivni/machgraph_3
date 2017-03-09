#include <iostream>
#include <vector>

#include "Utility.h"
#include "SOIL.h"

using namespace std;

const uint GRASS_INSTANCES = 10000;//16; // Количество травинок
const uint ROWS_COUNT = 100;
bool multisampling_enabled;

GL::Camera camera;               // Мы предоставляем Вам реализацию камеры. В OpenGL камера - это просто 2 матрицы. Модельно-видовая матрица и матрица проекции. // ###
                                 // Задача этого класса только в том чтобы обработать ввод с клавиатуры и правильно сформировать эти матрицы.
                                 // Вы можете просто пользоваться этим классом для расчёта указанных матриц.


enum
{
	wind_mode_toggle,
	wind_mode_accumulation
};

class Wind
{
	int instances;
	int rows;
	bool wind_started;
	float windforce;
	int windmode;
	bool debug;
	int zeroforce_loc;
	int zeromod_row;
	float* forces;
	float* velosities;
	float* accelerations;
	float* variances;
	VM::vec2** positions;
	vector<VM::vec2>* v_positions;
	vector<VM::vec4>* v_points;
	vector<VM::vec2>* v_differences;
	vector<VM::vec4>* v_variance;

public:
	Wind(const int count, const int rows_count);
	void GiveInfo(vector<VM::vec2>* positions_in,
				  vector<VM::vec4>* points_in,
				  vector<VM::vec2>* differences_in,
				  vector<VM::vec4>* variance_in);
	void ToggleWind();
	void RandomToggle();
	void IncWind();
	void ToggleDebug();
	void UpdateWindVariance();
	bool WindStarted();

	vector<VM::vec2> GeneratePositions();
};

void Timer(int)
{
	glutPostRedisplay();
	glutTimerFunc(16, Timer, 0);
}

void Wind::ToggleDebug()
{
	if (debug)
		debug = false;
	else debug = true;
}

Wind WindController(GRASS_INSTANCES, ROWS_COUNT);

void Wind::IncWind()
{
	wind_started = true;
	windmode = wind_mode_accumulation;
	if (windforce == 0)
		windforce = 0.01;
	else if (windforce >= 1)
		windforce *= 1.1;
	else
		windforce *= 1.5;
	if (windforce >= 1.5)
		windforce = 1.5;
}

void Wind::ToggleWind()
{
	windmode = wind_mode_toggle;
	wind_started = true;
	if (windforce == 0)
		windforce = 0.10;//((float)rand())/RAND_MAX*0.5;
	else 
		windforce = 0;
}

void Wind::RandomToggle()
{
	windmode = wind_mode_toggle;
	wind_started = true;
	if (windforce == 0)
		windforce = ((float)rand())/RAND_MAX*0.5;
	else
		windforce = 0;
}

void Wind::UpdateWindVariance()
{
	const int FPS=25;
	int count = 0;
	int cols;
	int sgn;
	for (int i = 0; i < rows; i++)
	{
		if (i < zeromod_row)
			cols = instances/rows+1;
		else
			cols = instances/rows;
		for (int j = 0; j < cols; j++)
		{
			variances[count] = variances[count] + 
				(velosities[count]+accelerations[count]/FPS)/FPS + 
			accelerations[count]/(FPS*FPS*2);
			velosities[count] += accelerations[count]/FPS;
			if (variances[count] > 0)
				sgn = 1;
			else
				sgn = -1;
			float var = variances[count];
			float vel = velosities[count];
			accelerations[count] = (forces[i] - sgn*var*var*20-vel*vel*vel*999)*(*v_differences)[count][0];
			(*v_variance)[count] = VM::vec4(variances[count],0,0,0);
			count++;
		}
		if (windforce == 0)
		{
			const int divider = 32;
			if (i == 0)
				forces[i] = windforce;
			else if (i < rows/divider)
			{
				float sum = 0;
				for (int j = i; j > -1; j--)
					sum += forces[j];
				sum /= i+1;
				forces[i] = sum;
			} else
			{
				float sum = 0;
				for (int j = i; j > i-rows/divider; j--)
					sum += forces[j];
				sum /= rows/divider;
				forces[i] = sum;
			}
		}
		else
			if (i == 0)
				forces[i] = (windforce);
			else if (i == 1)
				forces[i] = (forces[i-1]+forces[i])/2;
			else
				forces[i] = (forces[i-1]+forces[i-2]+forces[i])/3;
	}
	if (windmode == wind_mode_accumulation)
		windforce /= 1.01;
	if (debug)
		printf("%f, %f, %f, %f, %f\n",
			variances[0],velosities[0], accelerations[0], forces[0], windforce);
	
//		printf("%f, %f, %f, %f, %f\n",
//			variances[9*(instances/rows+1)+1],velosities[9*(instances/rows+1)+1],accelerations[9*(instances/rows+1)+1],forces[9], windforce);
}

Wind::Wind(const int count,const int rows_count)
{
	instances = count;
	rows = rows_count;
	windforce = 0;
	windmode = wind_mode_toggle;
	wind_started = false;
	debug = false;
	forces = new float[rows];
	velosities = new float[count];
	accelerations = new float[count];
	variances = new float[count];
	positions = new VM::vec2*[rows_count];
	for (int i = 0; i < count; i++)
	{
		velosities[i] = 0;
		accelerations[i] = 0;
		variances[i] = 0;
		if (i < rows)
			forces[i] = 0;
	}
}

void Wind::GiveInfo(vector<VM::vec2>* positions_in,
			  vector<VM::vec4>* points_in,
			  vector<VM::vec2>* differences_in,
			  vector<VM::vec4>* variance_in)
{
	v_positions = positions_in;
	v_points = points_in;
	v_differences = differences_in;
	v_variance = variance_in;
}

vector<VM::vec2> Wind::GeneratePositions()
{
    vector<VM::vec2> grassPositions(GRASS_INSTANCES);
	int ipr = GRASS_INSTANCES/rows;
	int rsd = GRASS_INSTANCES;
	float dx = ((float)1)/(rows+1);
	float x = dx/2;
	int global_counter = 0;
	zeromod_row = rows+1;
	for (int i = 0; i < rows; i++)
	{
		int inst = ipr;
//		printf("%d\n",rsd%ipr);
		if (rsd%ipr != 0)
			inst++;
		else 
			if (zeromod_row == rows+1)
				zeromod_row = i;
		rsd -= inst;
		positions[i] = new VM::vec2[inst];
		
		float dz = ((float)1)/(inst+1);
		float z = dz/2;
		for (int j = 0; j < inst; j++)
		{
			float position_x = x+((float)rand()/RAND_MAX-0.5)*dx;
			float position_z = z+((float)rand()/RAND_MAX-0.5)*dz;
//			printf("DX = %f; X = %f; DZ = %f; Z = %f; %f,%f\n",dx,x,dz,z,position_x,position_z);
			VM::vec2 position(position_x, position_z);
			positions[i][j] = position;
			grassPositions[global_counter++] = position;
			z+=dz;
		}
		x+=dx; 
	}
	return grassPositions;
}

const int SKYBOX_FILENUM = 6;

class Skybox
{
	string filenames[SKYBOX_FILENUM];
	GLuint skybox_texture;
	GLuint skyboxVAO;
	GLuint skyboxVBO;
	GLuint skyboxShader;
public:
	Skybox(const string& PosX,
				  const string& NegX,
				  const string& PosY,
				  const string& NegY,
				  const string& PosZ,
				  const string& NegZ);

	void LoadTextureFiles();
	void BindTexture(GLenum TextureUnit);
	void CreateSkybox();
	void DrawSky();
};

const char PosXFile[]="../Texture/posx.jpg";
const char NegXFile[]="../Texture/negx.jpg";
const char PosYFile[]="../Texture/posy.jpg";
const char NegYFile[]="../Texture/negy.jpg";
const char PosZFile[]="../Texture/posz.jpg";
const char NegZFile[]="../Texture/negz.jpg";
/*
const char PosXFile[]="../Texture/skybox_right.png";
const char NegXFile[]="../Texture/skybox_left.png";
const char PosYFile[]="../Texture/skybox_up.png";
const char NegYFile[]="../Texture/skybox_down.png";
const char PosZFile[]="../Texture/skybox_back.png";
const char NegZFile[]="../Texture/skybox_front.png";*/
Skybox SkyboxController(PosXFile,
						NegXFile,
						PosYFile,
						NegYFile,
						PosZFile,
						NegZFile);

Skybox::Skybox(
			  const string& PosX,
			  const string& NegX,
			  const string& PosY,
			  const string& NegY,
			  const string& PosZ,
			  const string& NegZ)
{
	filenames[0] = PosX;
	filenames[1] = NegX;
	filenames[2] = PosY;
	filenames[3] = NegY;
	filenames[4] = PosZ;
	filenames[5] = NegZ;
}

void Skybox::LoadTextureFiles()
{
	glGenTextures(1,&skybox_texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
	for (int i = 0; i < SKYBOX_FILENUM; i++)
	{
		int width, height;
		unsigned char* image = SOIL_load_image(&(filenames[i][0]), &width, &height,
											   0, SOIL_LOAD_RGBA);
		if (image == NULL)
			throw (string) "Error while loading skybox textures\n";
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_RGBA, width, height, 0,
					 GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i);
		SOIL_free_image_data(image);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Skybox::BindTexture(GLenum TextureUnit)
{
	glActiveTexture(TextureUnit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
}

void Skybox::CreateSkybox()
{
    vector<VM::vec4> verticles = 
	{
		VM::vec4(-1,1,-1,1),
		VM::vec4(-1,-1,-1,1),
		VM::vec4(1,-1,-1,1),
		VM::vec4(1,-1,-1,1),
		VM::vec4(1,1,-1,1),
		VM::vec4(-1,1,-1,1),

		VM::vec4(-1,-1,1,1),
		VM::vec4(-1,-1,-1,1),
		VM::vec4(-1,1,-1,1),
		VM::vec4(-1,1,-1,1),
		VM::vec4(-1,1,1,1),
		VM::vec4(-1,-1,1,1),

		VM::vec4(1,-1,-1,1),
		VM::vec4(1,-1,1,1),
		VM::vec4(1,1,1,1),
		VM::vec4(1,1,1,1),
		VM::vec4(1,1,-1,1),
		VM::vec4(1,-1,-1,1),

		VM::vec4(-1,-1,1,1),
		VM::vec4(-1,1,1,1),
		VM::vec4(1,1,1,1),
		VM::vec4(1,1,1,1),
		VM::vec4(1,-1,1,1),
		VM::vec4(-1,-1,1,1),

		VM::vec4(-1,1,-1,1),
		VM::vec4(1,1,-1,1),
		VM::vec4(1,1,1,1),
		VM::vec4(1,1,1,1),
		VM::vec4(-1,1,1,1),
		VM::vec4(-1,1,-1,1),

		VM::vec4(-1,-1,-1,1),
		VM::vec4(-1,-1,1,1),
		VM::vec4(1,-1,-1,1),
		VM::vec4(1,-1,-1,1),
		VM::vec4(-1,-1,1,1),
		VM::vec4(1,-1,1,1)
	};
	skyboxShader = GL::CompileShaderProgram("skybox");
	GLuint meshBuffer;
	glGenBuffers(1,&meshBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, meshBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * verticles.size(),
				 verticles.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &skyboxVAO);
	glBindVertexArray(skyboxVAO);
	GLuint meshLocation = glGetAttribLocation(skyboxShader, "position");
	glEnableVertexAttribArray(meshLocation);
	glVertexAttribPointer(meshLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,0);
}

void Skybox::DrawSky()
{
	glDepthMask(GL_FALSE);
	glUseProgram(skyboxShader);
	GLint cameraLocation = glGetUniformLocation(skyboxShader, "camera");
	glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data());
	glBindVertexArray(skyboxVAO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDepthMask(GL_TRUE);
}

GLuint grassPointsCount; // Количество вершин у модели травинки
GLuint grassShader;      // Шейдер, рисующий траву
GLuint grassVAO;         // VAO для травы (что такое VAO почитайте в доках)
GLuint grassVariance;    // Буфер для смещения координат травинок
GLuint grassDifferences;

GLuint texture_grass;	 // Textures ID's
GLuint texture_ground;
vector<VM::vec4> grassVarianceData(GRASS_INSTANCES); // Вектор со смещениями для координат травинок
vector<VM::vec2> grassDifferencesData(GRASS_INSTANCES);
vector<VM::vec2> grassPositions;
vector<VM::vec4> grassPoints;

GLuint groundShader; // Шейдер для земли
GLuint groundVAO; // VAO для земли

// Размеры экрана
uint screenWidth = 800;
uint screenHeight = 600;

// Это для захвата мышки. Вам это не потребуется (это не значит, что нужно удалять эту строку)
bool captureMouse = true;

// Функция, рисующая замлю
void DrawGround() {
    // Используем шейдер для земли
    glUseProgram(groundShader);                                                  CHECK_GL_ERRORS

    // Устанавливаем юниформ для шейдера. В данном случае передадим перспективную матрицу камеры
    // Находим локацию юниформа 'camera' в шейдере
    GLint cameraLocation = glGetUniformLocation(groundShader, "camera");         CHECK_GL_ERRORS
    // Устанавливаем юниформ (загружаем на GPU матрицу проекции?)                                                     // ###
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS

    // Подключаем VAO, который содержит буферы, необходимые для отрисовки земли
    glBindVertexArray(groundVAO);                                                CHECK_GL_ERRORS

	glBindTexture(GL_TEXTURE_2D, texture_ground);                                 CHECK_GL_ERRORS

    // Рисуем землю: 2 треугольника (6 вершин)
    glDrawArrays(GL_TRIANGLES, 0, 6);                                            CHECK_GL_ERRORS

    // Отсоединяем VAO
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
	
	glBindTexture(GL_TEXTURE_2D, 0);                                             CHECK_GL_ERRORS

    // Отключаем шейдер
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Обновление смещения травинок
void UpdateGrassVariance() {
	WindController.UpdateWindVariance();
//    float t = clock();
/*
	// Генерация случайных смещений
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassVarianceData[i].x = (sin(t/50000))/200;//(float)rand() / RAND_MAX / 100;

        grassVarianceData[i].z = 0;//(sin(t/10000) - 1)/200;//(float)rand() / RAND_MAX / 100;
    	grassVarianceData[i].y = 0;//(cos(t/10000) - 1)/200;
	}
*/
    // Привязываем буфер, содержащий смещения
    glBindBuffer(GL_ARRAY_BUFFER, grassVariance);                                CHECK_GL_ERRORS
    // Загружаем данные в видеопамять
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}


// Рисование травы
void DrawGrass() {
    // Тут то же самое, что и в рисовании земли
    glUseProgram(grassShader);                                                   CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(grassShader, "camera");          CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
    glBindVertexArray(grassVAO);                                                 CHECK_GL_ERRORS
    // Обновляем смещения для травы
    UpdateGrassVariance();
    // Отрисовка травинок в количестве GRASS_INSTANCES
	glBindTexture(GL_TEXTURE_2D, texture_grass);                                 CHECK_GL_ERRORS
    glDrawArraysInstanced(GL_TRIANGLES, 0, grassPointsCount, GRASS_INSTANCES);   CHECK_GL_ERRORS
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glUseProgram(0);                                                             CHECK_GL_ERRORS
	glBindTexture(GL_TEXTURE_2D, 0);
}

// Эта функция вызывается для обновления экрана
void RenderLayouts() {
    // Включение буфера глубины
    glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Очистка буфера глубины и цветового буфера
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Рисуем меши
	SkyboxController.DrawSky();
    DrawGround();
    DrawGrass();  
    glutSwapBuffers();
//	glDisable(GL_DEPTH_TEST);
}

// Завершение программы
void FinishProgram() {
    glutDestroyWindow(glutGetWindow());
}

void ToggleMultisampling()
{
	if (multisampling_enabled)
	{
		multisampling_enabled = false;
		glDisable(GL_MULTISAMPLE);
	} else
	{
		multisampling_enabled = true;
		glEnable(GL_MULTISAMPLE);
	}
}

// Обработка события нажатия клавиши (специальные клавиши обрабатываются в функции SpecialButtons)
void KeyboardEvents(unsigned char key, int x, int y) {
    if (key == 27) {
        FinishProgram();
	} else if (key == 'w') {
		WindController.ToggleWind();
	} else if (key == 'W') {
		WindController.IncWind();
	} else if (key == 'r') {
		WindController.RandomToggle();
	} else if (key == 'd') {
		WindController.ToggleDebug();
    } else if (key == 'h') {
        camera.goForward();
    } else if (key == 'j') {
        camera.goBack();
    } else if (key == 'm') {
        captureMouse = !captureMouse;
        if (captureMouse) {
            glutWarpPointer(screenWidth / 2, screenHeight / 2);
            glutSetCursor(GLUT_CURSOR_NONE);
        } else {
            glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
        }
    } else if (key == 'a' || key == 'A') {
		ToggleMultisampling();
	}
}

// Обработка события нажатия специальных клавиш
void SpecialButtons(int key, int x, int y) {
    if (key == GLUT_KEY_RIGHT) {
        camera.rotateY(0.02);
    } else if (key == GLUT_KEY_LEFT) {
        camera.rotateY(-0.02);
    } else if (key == GLUT_KEY_UP) {
        camera.rotateTop(-0.02);
    } else if (key == GLUT_KEY_DOWN) {
        camera.rotateTop(0.02);
    }
}

void IdleFunc() {
    glutPostRedisplay();
}

// Обработка события движения мыши
void MouseMove(int x, int y) {
    if (captureMouse) {
        int centerX = screenWidth / 2,
            centerY = screenHeight / 2;
        if (x != centerX || y != centerY) {
            camera.rotateY((x - centerX) / 1000.0f);
            camera.rotateTop((y - centerY) / 1000.0f);
            glutWarpPointer(centerX, centerY);
        }
    }
}

// Обработка нажатия кнопки мыши
void MouseClick(int button, int state, int x, int y) {
}

// Событие изменение размера окна
void windowReshapeFunc(GLint newWidth, GLint newHeight) {
    glViewport(0, 0, newWidth, newHeight);
    screenWidth = newWidth;
    screenHeight = newHeight;

    camera.screenRatio = (float)screenWidth / screenHeight;
}

// Инициализация окна
void InitializeGLUT(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitContextVersion(3, 0);
    glutInitWindowPosition(-1, -1);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Computer Graphics 3");
	glDisable(GL_MULTISAMPLE);
    glutWarpPointer(400, 300);
    glutSetCursor(GLUT_CURSOR_NONE);

    glutDisplayFunc(RenderLayouts);
    glutKeyboardFunc(KeyboardEvents);
    glutSpecialFunc(SpecialButtons);
//    glutIdleFunc(IdleFunc);
    glutPassiveMotionFunc(MouseMove);
    glutMouseFunc(MouseClick);
    glutReshapeFunc(windowReshapeFunc);
	glutTimerFunc(16, Timer, 0);
}

// Генерация позиций травинок (эту функцию вам придётся переписать)
vector<VM::vec2> GenerateGrassPositions() {
	return WindController.GeneratePositions();
	/*
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassPositions[i] = VM::vec2((float)rand()/RAND_MAX,(float) rand()/RAND_MAX);
    }
	*/
}

//
// HERE STARTS THE GREAT GRASS MESH GRAVEYARD

/*        VM::vec4(0, 0, 0, 1),
        VM::vec4(1, 0, 0, 1),
		VM::vec4(0.0625,0.125,0,1),

        VM::vec4(1, 0, 0, 1),
		VM::vec4(0.0625,0.125,0,1),
		VM::vec4(0.875,0.25,0,1),

		VM::vec4(0.0625,0.125,0,1),
		VM::vec4(0.875,0.25,0,1),
		VM::vec4(0.1875,0.375,0,1),

		VM::vec4(0.875,0.25,0,1),
		VM::vec4(0.1875,0.375,0,1),
		VM::vec4(0.75,0.5,0,1),

		VM::vec4(0.1875,0.375,0,1),
		VM::vec4(0.75,0.5,0,1),
		VM::vec4(0.3125,0.625,0,1),

		VM::vec4(0.75,0.5,0,1),
		VM::vec4(0.3125,0.625,0,1),
		VM::vec4(0.625,0.75,0,1),

		VM::vec4(0.3125,0.625,0,1),
		VM::vec4(0.625,0.75,0,1),
		VM::vec4(0.4375,0.875,0,1),

		VM::vec4(0.625,0.75,0,1),
		VM::vec4(0.4375,0.875,0,1),
        VM::vec4(0.5, 1, 0, 1),*/

/*
	VM::vec4(0,0,0,1),
	VM::vec4(0.8,0,0,1),
	VM::vec4(0.05,0.3,0,1),

	VM::vec4(0.8,0,0,1),
	VM::vec4(0.05,0.3,0,1),
	VM::vec4(0.75,0.3,0,1),

	VM::vec4(0.05,0.3,0,1),
	VM::vec4(0.75,0.3,0,1),
	VM::vec4(0.15,0.6,0,1),

	VM::vec4(0.75,0.3,0,1),
	VM::vec4(0.15,0.6,0,1),
	VM::vec4(0.65,0.6,0,1),

	VM::vec4(0.15,0.6,0,1),
	VM::vec4(0.65,0.6,0,1),
	VM::vec4(0.3,0.85,0,1),

	VM::vec4(0.65,0.6,0,1),
	VM::vec4(0.3,0.85,0,1),
	VM::vec4(0.5,0.85,0,1),

	VM::vec4(0.3,0.85,0,1),
	VM::vec4(0.5,0.85,0,1),
	VM::vec4(0.4,1.0,0,1)
*/
/*		VM::vec4(0,0,0,1),
		VM::vec4(0.4,0,0,1),
		VM::vec4(0.025,0.25,0,1),

		VM::vec4(0.4,0,0,1),
		VM::vec4(0.025,0.25,0,1),
		VM::vec4(0.375,0.25,0,1),

		VM::vec4(0.025,0.25,0,1),
		VM::vec4(0.375,0.25,0,1),
		VM::vec4(0.05,0.5,0,1),

		VM::vec4(0.375,0.25,0,1),
		VM::vec4(0.05,0.5,0,1),
		VM::vec4(0.35,0.5,0,1),
		
		VM::vec4(0.05,0.5,0,1),
		VM::vec4(0.35,0.5,0,1),
		VM::vec4(0.1,0.85,0,1),

		VM::vec4(0.35,0.5,0,1),
		VM::vec4(0.1,0.85,0,1),
		VM::vec4(0.3,0.85,0,1),

		VM::vec4(0.1,0.85,0,1),
		VM::vec4(0.3,0.85,0,1),
		VM::vec4(0.15,0.95,0,1),

		VM::vec4(0.3,0.85,0,1),
		VM::vec4(0.15,0.95,0,1),
		VM::vec4(0.25,0.95,0,1),

		VM::vec4(0.15,0.95,0,1),
		VM::vec4(0.25,0.95,0,1),
		VM::vec4(0.2,1,0,1) */

//        VM::vec4(0, 0, 0, 1),
//        VM::vec4(1, 0, 0, 1),
//        VM::vec4(0.5, 1, 0, 1),


//	THIS IS THE END OF THE GREAT GRASS MESH GRAVEYARD
//

// Здесь вам нужно будет генерировать меш


//And yes, I knew i could make it so it draws by taking previous two 
//points, adding a third one to them and making a triangle out of it and so on,
//but who cares about the memory, right?

vector<VM::vec4> GenMesh(uint n) {
    return {

//TC stands for Texture Coordinates

		VM::vec4(0,0,0,1),
		VM::vec4(0,0,0,1),//TC
		
		VM::vec4(0.4,0,0,1),
		VM::vec4(1,0,0,1),//TC

		VM::vec4(0.0033,0.05,0,1),
		VM::vec4(0.0083,0.05,0,1),//TC


		VM::vec4(0.4,0,0,1),
		VM::vec4(1,0,0,1),//TC

		VM::vec4(0.0033,0.05,0,1),
		VM::vec4(0.0083,0.05,0,1),//TC

		VM::vec4(0.3967,0.05,0,1),
		VM::vec4(0.9917,0.05,0,1),//TC


		VM::vec4(0.0033,0.05,0,1),
		VM::vec4(0.0083,0.05,0,1),//TC

		VM::vec4(0.3967,0.05,0,1),
		VM::vec4(0.9917,0.05,0,1),//TC

		VM::vec4(0.0066,0.1,0,1),
		VM::vec4(0.0166,0.1,0,1),//TC


		VM::vec4(0.3967,0.05,0,1),
		VM::vec4(0.9917,0.05,0,1),//TC

		VM::vec4(0.0066,0.1,0,1),
		VM::vec4(0.0166,0.1,0,1),//TC

		VM::vec4(0.3934,0.1,0,1),
		VM::vec4(0.9834,0.1,0,1),//TC


		VM::vec4(0.0066,0.1,0,1),
		VM::vec4(0.0166,0.1,0,1),//TC

		VM::vec4(0.3934,0.1,0,1),
		VM::vec4(0.9834,0.1,0,1),//TC

		VM::vec4(0.01,0.15,0,1),
		VM::vec4(0.025,0.15,0,1),//TC


		VM::vec4(0.3934,0.1,0,1),
		VM::vec4(0.9834,0.1,0,1),//TC

		VM::vec4(0.01,0.15,0,1),
		VM::vec4(0.025,0.15,0,1),//TC

		VM::vec4(0.39,0.15,0,1),
		VM::vec4(0.975,0.15,0,1),//TC


		VM::vec4(0.01,0.15,0,1),
		VM::vec4(0.025,0.15,0,1),//TC

		VM::vec4(0.39,0.15,0,1),
		VM::vec4(0.975,0.15,0,1),//TC

		VM::vec4(0.015,0.2,0,1),
		VM::vec4(0.0375,0.2,0,1),//TC


		VM::vec4(0.39,0.15,0,1),
		VM::vec4(0.975,0.15,0,1),//TC

		VM::vec4(0.015,0.2,0,1),
		VM::vec4(0.0375,0.2,0,1),//TC

		VM::vec4(0.385,0.2,0,1),
		VM::vec4(0.9625,0.2,0,1),//TC


		VM::vec4(0.015,0.2,0,1),
		VM::vec4(0.0375,0.2,0,1),//TC

		VM::vec4(0.385,0.2,0,1),
		VM::vec4(0.9625,0.2,0,1),//TC

		VM::vec4(0.02,0.25,0,1),
		VM::vec4(0.05,0.25,0,1),//TC


		VM::vec4(0.385,0.2,0,1),
		VM::vec4(0.9625,0.2,0,1),//TC

		VM::vec4(0.02,0.25,0,1),
		VM::vec4(0.05,0.25,0,1),//TC

		VM::vec4(0.38,0.25,0,1),
		VM::vec4(0.95,0.25,0,1),//TC


		VM::vec4(0.02,0.25,0,1),
		VM::vec4(0.05,0.25,0,1),//TC

		VM::vec4(0.38,0.25,0,1),
		VM::vec4(0.95,0.25,0,1),//TC

		VM::vec4(0.025,0.3,0,1),
		VM::vec4(0.0625,0.3,0,1),//TC


		VM::vec4(0.38,0.25,0,1),
		VM::vec4(0.95,0.25,0,1),//TC

		VM::vec4(0.025,0.3,0,1),
		VM::vec4(0.0625,0.3,0,1),//TC

		VM::vec4(0.375,0.3,0,1),
		VM::vec4(0.9375,0.3,0,1),//TC


		VM::vec4(0.025,0.3,0,1),
		VM::vec4(0.0625,0.3,0,1),//TC

		VM::vec4(0.375,0.3,0,1),
		VM::vec4(0.9375,0.3,0,1),//TC

		VM::vec4(0.03,0.35,0,1),
		VM::vec4(0.075,0.35,0,1),//TC


		VM::vec4(0.375,0.3,0,1),
		VM::vec4(0.9375,0.3,0,1),//TC

		VM::vec4(0.03,0.35,0,1),
		VM::vec4(0.075,0.35,0,1),//TC

		VM::vec4(0.37,0.35,0,1),
		VM::vec4(0.925,0.35,0,1),//TC


		VM::vec4(0.03,0.35,0,1),
		VM::vec4(0.075,0.35,0,1),//TC

		VM::vec4(0.37,0.35,0,1),
		VM::vec4(0.925,0.35,0,1),//TC

		VM::vec4(0.0366,0.4,0,1),
		VM::vec4(0.0916,0.4,0,1),//TC


		VM::vec4(0.37,0.35,0,1),
		VM::vec4(0.925,0.35,0,1),//TC

		VM::vec4(0.0366,0.4,0,1),
		VM::vec4(0.0916,0.4,0,1),//TC

		VM::vec4(0.3634,0.4,0,1),
		VM::vec4(0.9084,0.4,0,1),//TC


		VM::vec4(0.0366,0.4,0,1),
		VM::vec4(0.0916,0.4,0,1),//TC

		VM::vec4(0.3634,0.4,0,1),
		VM::vec4(0.9084,0.4,0,1),//TC

		VM::vec4(0.0433,0.45,0,1),
		VM::vec4(0.1083,0.45,0,1),//TC


		VM::vec4(0.3634,0.4,0,1),
		VM::vec4(0.9084,0.4,0,1),//TC

		VM::vec4(0.0433,0.45,0,1),
		VM::vec4(0.1083,0.45,0,1),//TC

		VM::vec4(0.3567,0.45,0,1),
		VM::vec4(0.8017,0.45,0,1),//TC


		VM::vec4(0.0433,0.45,0,1),
		VM::vec4(0.1083,0.45,0,1),//TC

		VM::vec4(0.3567,0.45,0,1),
		VM::vec4(0.8017,0.45,0,1),//TC

		VM::vec4(0.05,0.5,0,1),
		VM::vec4(0.125,0.5,0,1),//TC


		VM::vec4(0.3567,0.45,0,1),
		VM::vec4(0.8017,0.45,0,1),//TC

		VM::vec4(0.05,0.5,0,1),
		VM::vec4(0.125,0.5,0,1),//TC

		VM::vec4(0.35,0.5,0,1),
		VM::vec4(0.875,0.5,0,1),//TC


		VM::vec4(0.05,0.5,0,1),
		VM::vec4(0.125,0.5,0,1),//TC

		VM::vec4(0.35,0.5,0,1),
		VM::vec4(0.875,0.5,0,1),//TC

		VM::vec4(0.0566,0.55,0,1),
		VM::vec4(0.1416,0.55,0,1),//TC


		VM::vec4(0.35,0.5,0,1),
		VM::vec4(0.875,0.5,0,1),//TC

		VM::vec4(0.0566,0.55,0,1),
		VM::vec4(0.1416,0.55,0,1),//TC

		VM::vec4(0.3434,0.55,0,1),
		VM::vec4(0.8584,0.55,0,1),//TC


		VM::vec4(0.0566,0.55,0,1),
		VM::vec4(0.1416,0.55,0,1),//TC

		VM::vec4(0.3434,0.55,0,1),
		VM::vec4(0.8584,0.55,0,1),//TC

		VM::vec4(0.06,0.59,0,1),
		VM::vec4(0.15,0.59,0,1),//TC


		VM::vec4(0.3434,0.55,0,1),
		VM::vec4(0.8584,0.55,0,1),//TC

		VM::vec4(0.06,0.59,0,1),
		VM::vec4(0.15,0.59,0,1),//TC

		VM::vec4(0.34,0.59,0,1),
		VM::vec4(0.85,0.59,0,1),//TC


		VM::vec4(0.06,0.59,0,1),
		VM::vec4(0.15,0.59,0,1),//TC

		VM::vec4(0.34,0.59,0,1),
		VM::vec4(0.85,0.59,0,1),//TC

		VM::vec4(0.065,0.633,0,1),
		VM::vec4(0.1625,0.633,0,1),//TC


		VM::vec4(0.34,0.59,0,1),
		VM::vec4(0.85,0.59,0,1),//TC

		VM::vec4(0.065,0.633,0,1),
		VM::vec4(0.1625,0.633,0,1),//TC

		VM::vec4(0.335,0.633,0,1),
		VM::vec4(0.8375,0.633,0,1),//TC


		VM::vec4(0.065,0.633,0,1),
		VM::vec4(0.1625,0.633,0,1),//TC

		VM::vec4(0.335,0.633,0,1),
		VM::vec4(0.8375,0.633,0,1),//TC

		VM::vec4(0.07,0.666,0,1),
		VM::vec4(0.175,0.666,0,1),//TC


		VM::vec4(0.335,0.633,0,1),
		VM::vec4(0.8375,0.633,0,1),//TC

		VM::vec4(0.07,0.666,0,1),
		VM::vec4(0.175,0.666,0,1),//TC

		VM::vec4(0.33,0.666,0,1),
		VM::vec4(0.825,0.666,0,1),//TC


		VM::vec4(0.07,0.666,0,1),
		VM::vec4(0.175,0.666,0,1),//TC

		VM::vec4(0.33,0.666,0,1),
		VM::vec4(0.825,0.666,0,1),//TC

		VM::vec4(0.08,0.75,0,1),
		VM::vec4(0.2,0.75,0,1),//TC


		VM::vec4(0.33,0.666,0,1),
		VM::vec4(0.825,0.666,0,1),//TC

		VM::vec4(0.08,0.75,0,1),
		VM::vec4(0.2,0.75,0,1),//TC

		VM::vec4(0.32,0.75,0,1),
		VM::vec4(0.8,0.75,0,1),//TC


		VM::vec4(0.08,0.75,0,1),
		VM::vec4(0.2,0.75,0,1),//TC

		VM::vec4(0.32,0.75,0,1),
		VM::vec4(0.8,0.75,0,1),//TC

		VM::vec4(0.09,0.81,0,1),
		VM::vec4(0.225,0.81,0,1),//TC


		VM::vec4(0.32,0.75,0,1),
		VM::vec4(0.8,0.75,0,1),//TC

		VM::vec4(0.09,0.81,0,1),
		VM::vec4(0.225,0.81,0,1),//TC

		VM::vec4(0.31,0.81,0,1),
		VM::vec4(0.775,0.81,0,1),//TC


		VM::vec4(0.09,0.81,0,1),
		VM::vec4(0.225,0.81,0,1),//TC

		VM::vec4(0.31,0.81,0,1),
		VM::vec4(0.775,0.81,0,1),//TC

		VM::vec4(0.1,0.85,0,1),
		VM::vec4(0.25,0.85,0,1),//TC


		VM::vec4(0.31,0.81,0,1),
		VM::vec4(0.775,0.81,0,1),//TC

		VM::vec4(0.1,0.85,0,1),
		VM::vec4(0.25,0.85,0,1),//TC

		VM::vec4(0.3,0.85,0,1),
		VM::vec4(0.75,0.85,0,1),//TC


		VM::vec4(0.1,0.85,0,1),
		VM::vec4(0.25,0.85,0,1),//TC

		VM::vec4(0.3,0.85,0,1),
		VM::vec4(0.75,0.85,0,1),//TC

		VM::vec4(0.125,0.91,0,1),
		VM::vec4(0.31,0.91,0,1),//TC


		VM::vec4(0.3,0.85,0,1),
		VM::vec4(0.75,0.85,0,1),//TC

		VM::vec4(0.125,0.91,0,1),
		VM::vec4(0.31,0.91,0,1),//TC

		VM::vec4(0.275,0.91,0,1),
		VM::vec4(0.69,0.91,0,1),//TC


		VM::vec4(0.125,0.91,0,1),
		VM::vec4(0.31,0.91,0,1),//TC

		VM::vec4(0.275,0.91,0,1),
		VM::vec4(0.69,0.91,0,1),//TC

		VM::vec4(0.15,0.95,0,1),
		VM::vec4(0.375,0.95,0,1),//TC


		VM::vec4(0.275,0.91,0,1),
		VM::vec4(0.69,0.91,0,1),//TC

		VM::vec4(0.15,0.95,0,1),
		VM::vec4(0.375,0.95,0,1),//TC

		VM::vec4(0.25,0.95,0,1),
		VM::vec4(0.625,0.95,0,1),//TC


		VM::vec4(0.15,0.95,0,1),
		VM::vec4(0.375,0.95,0,1),//TC

		VM::vec4(0.25,0.95,0,1),
		VM::vec4(0.625,0.95,0,1),//TC

		VM::vec4(0.2,1.0,0,1),
		VM::vec4(0.5,1.0,0,1)//TC
};
}

vector<VM::vec2> GenerateGrassDifferences()
{
	vector<VM::vec2> grassDifferences(GRASS_INSTANCES);
	for (uint i = 0; i < GRASS_INSTANCES; ++i) 
	{
		grassDifferences[i] = VM::vec2((float)rand()/RAND_MAX+0.5,(float) rand()/RAND_MAX);
	}
	return grassDifferences;
}

// Создание травы
void CreateGrass() {
    uint LOD = 1;
    // Создаём меш
    grassPoints = GenMesh(LOD);
    // Сохраняем количество вершин в меше травы
    grassPointsCount = grassPoints.size();
    // Создаём позиции для травинок
    grassPositions = GenerateGrassPositions();

	grassDifferencesData = GenerateGrassDifferences();

    // Инициализация смещений для травинок
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassVarianceData[i] = VM::vec4(0, 0, 0, 0);
    }

    /* Компилируем шейдеры
    Эта функция принимает на вход название шейдера 'shaderName',
    читает файлы shaders/{shaderName}.vert - вершинный шейдер
    и shaders/{shaderName}.frag - фрагментный шейдер,
    компилирует их и линкует.
    */
    grassShader = GL::CompileShaderProgram("grass");

    // Здесь создаём буфер
    GLuint pointsBuffer;
    // Это генерация одного буфера (в pointsBuffer хранится идентификатор буфера)
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    // Привязываем сгенерированный буфер
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    // Заполняем буфер данными из вектора
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * grassPoints.size(), grassPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    // Создание VAO
    // Генерация VAO
    glGenVertexArrays(1, &grassVAO);                                             CHECK_GL_ERRORS
    // Привязка VAO
    glBindVertexArray(grassVAO);                                                 CHECK_GL_ERRORS

    // Получение локации параметра 'point' в шейдере
    GLuint pointsLocation = glGetAttribLocation(grassShader, "point");           CHECK_GL_ERRORS
    // Подключаем массив атрибутов к данной локации
    glEnableVertexAttribArray(pointsLocation);                                   CHECK_GL_ERRORS
    // Устанавливаем параметры для получения данных из массива (по 4 значение типа float на одну вершину)
    glVertexAttribPointer(pointsLocation, 4, GL_FLOAT, GL_FALSE, 2*sizeof(VM::vec4), 0);          CHECK_GL_ERRORS

	GLuint index = glGetAttribLocation(grassShader, "texCoord_in");                    CHECK_GL_ERRORS
	glEnableVertexAttribArray(index);											 CHECK_GL_ERRORS
	glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, 2*sizeof(VM::vec4),
		(GLvoid*) 16);  CHECK_GL_ERRORS

    // Создаём буфер для позиций травинок
    GLuint positionBuffer;
    glGenBuffers(1, &positionBuffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassPositions.size(), grassPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint positionLocation = glGetAttribLocation(grassShader, "position");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(positionLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    // Здесь мы указываем, что нужно брать новое значение из этого буфера для каждого инстанса (для каждой травинки)
    glVertexAttribDivisor(positionLocation, 1);                                  CHECK_GL_ERRORS

    // Создаём буфер для смещения травинок
    glGenBuffers(1, &grassVariance);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, grassVariance);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint varianceLocation = glGetAttribLocation(grassShader, "variance");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(varianceLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(varianceLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(varianceLocation, 1);                                  CHECK_GL_ERRORS
// Creating Differences buffer
    glGenBuffers(1, &grassDifferences);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, grassDifferences);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * GRASS_INSTANCES, grassDifferencesData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint differenceLocation = glGetAttribLocation(grassShader, "difference");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(differenceLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(differenceLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(differenceLocation, 1);                                  CHECK_GL_ERRORS

    // Отвязываем VAO
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}

// Создаём камеру (Если шаблонная камера вам не нравится, то можете переделать, но я бы не стал)
void CreateCamera() {
    camera.angle = 45.0f / 180.0f * M_PI;
    camera.direction = VM::vec3(0, 0.3, -1);
    camera.position = VM::vec3(0, -0.8, -0.8);
    camera.screenRatio = (float)screenWidth / screenHeight;
    camera.up = VM::vec3(0, 1, 0);
    camera.zfar = 50.0f;
    camera.znear = 0.05f;
}

// Создаём замлю
void CreateGround() {
    // Земля состоит из двух треугольников
    vector<VM::vec4> meshPoints = {
        VM::vec4(0, 0, 0, 1),
		VM::vec4(0.25, 0.75, 0, 0),

        VM::vec4(1, 0, 0, 1),
		VM::vec4(0.75, 0.75, 0, 0),

        VM::vec4(1, 0, 1, 1),
		VM::vec4(0.75, 0.25, 0, 0),

        VM::vec4(0, 0, 0, 1),
		VM::vec4(0.25, 0.75, 0, 0),

        VM::vec4(1, 0, 1, 1),
		VM::vec4(0.75, 0.25, 0, 0),

        VM::vec4(0, 0, 1, 1),
		VM::vec4(0.25, 0.25, 0, 0),
    };

    // Подробнее о том, как это работает читайте в функции CreateGrass

    groundShader = GL::CompileShaderProgram("ground");

    GLuint pointsBuffer;
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * meshPoints.size(), meshPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    glGenVertexArrays(1, &groundVAO);                                            CHECK_GL_ERRORS
    glBindVertexArray(groundVAO);                                                CHECK_GL_ERRORS

    GLuint index = glGetAttribLocation(groundShader, "point");                   CHECK_GL_ERRORS
    glEnableVertexAttribArray(index);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, 2*sizeof(VM::vec4), 0);  CHECK_GL_ERRORS

	index = glGetAttribLocation(groundShader, "texCoord_in");                    CHECK_GL_ERRORS
	glEnableVertexAttribArray(index);											 CHECK_GL_ERRORS
	glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, 2*sizeof(VM::vec4),
		(GLvoid*) 16);  CHECK_GL_ERRORS


    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}


void LoadTextures()
{
	int w_ground,h_ground,
		w_grass, h_grass;
	unsigned char* image_ground = SOIL_load_image("../Texture/ground_texture.jpg",
		&w_ground, &h_ground, 0, SOIL_LOAD_RGBA);
	unsigned char* image_grass = SOIL_load_image("../Texture/grass.jpeg",
		&w_grass, &h_grass, 0, SOIL_LOAD_RGBA);
	if (image_ground == NULL || image_grass == NULL)
		throw (string)"Error while reading texture files;"
			  " please look through the source code again\n";
	glGenTextures(1, &texture_ground);
	glBindTexture(GL_TEXTURE_2D, texture_ground);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_ground, h_ground, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image_ground);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image_ground);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &texture_grass);
	glBindTexture(GL_TEXTURE_2D, texture_grass);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_grass, h_grass, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image_grass);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image_grass);
	glBindTexture(GL_TEXTURE_2D,0);
	SkyboxController.LoadTextureFiles();
}

int main(int argc, char **argv)
{
	char gl_version_override[] = "MESA_GL_VERSION_OVERRIDE=3.3COMPAT";
	char enable_vsync[] = "__GL_SYNC_TO_VBLANK=1";
//	char set_anisotrope[]= "__GL_LOG_MAX_ANISO=1";
//	char unset_anisotrope[]= "__GL_LOG_MAX_ANISO=0";
	int errors = 0;
	try
	{
		errors += putenv(gl_version_override);
		errors += putenv(enable_vsync);
//		errors += putenv(set_anisotrope);
		if (errors != 0)
			throw (string) "An error occured while setting environmental variables\n";
        cout << "Start" << endl;
		multisampling_enabled = false;
        InitializeGLUT(argc, argv);
        cout << "GLUT inited" << endl;
		glewExperimental = true;
        glewInit();
        cout << "glew inited" << endl;
		LoadTextures();
		cout << "Textures loaded" << endl;
        CreateCamera();
        cout << "Camera created" << endl;
        CreateGrass();
        cout << "Grass created" << endl;
        CreateGround();
        cout << "Ground created" << endl;
		SkyboxController.CreateSkybox();
		cout << "Skybox created" << endl;
		WindController.GiveInfo(&grassPositions,
								&grassPoints,
								&grassDifferencesData,
								&grassVarianceData);
        glutMainLoop();
    } catch (string s) {
        cout << s << endl;
    }
}
