#include <GL/glut.h>
#include <cmath>
#include <string>

//=====================================
// VARIAVEIS
//=====================================

float jogadorX = -6.0f;
float jogadorY = 1.0f;
float velJogadorY = 0.0f;

float cpuX = 6.0f;
float cpuY = 1.0f;
float velCpuY = 0.0f;

float bolaX = 0.0f;
float bolaY = 5.0f;

float velBolaX = 0.08f;
float velBolaY = 0.0f;

float gravidade = -0.005f;

int placarJogador = 0;
int placarCPU = 0;

const float raioCabeca = 1.0f;
const float raioBola = 0.5f;

//=====================================
// TEXTO
//=====================================

void desenharTexto(float x, float y, std::string texto)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1280, 0, 720);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);

    glColor3f(1,1,1);
    glRasterPos2f(x,y);

    for(char c : texto)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

    glEnable(GL_LIGHTING);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

//=====================================
// RESET BOLA
//=====================================

void resetarBola()
{
    bolaX = 0;
    bolaY = 5;

    velBolaX = (rand()%2==0)?0.08f:-0.08f;
    velBolaY = 0;
}

//=====================================
// COLISAO
//=====================================

void colisaoJogador(
    float px,
    float py
)
{
    float dx = bolaX - px;
    float dy = bolaY - py;

    float dist =
    sqrt(dx*dx + dy*dy);

    if(dist < raioCabeca + raioBola)
    {
        velBolaX = dx * 0.08f;
        velBolaY = 0.15f;
    }
}

//=====================================
// UPDATE
//=====================================

void update(int)
{
    // Gravidade da bola
    velBolaY += gravidade;
    bolaY += velBolaY;
    bolaX += velBolaX;

    // Chao
    if(bolaY < 0.5f)
    {
        bolaY = 0.5f;
        velBolaY *= -0.8f;
    }

    // Paredes
    if(bolaX > 12 || bolaX < -12)
        velBolaX *= -1;

    // Jogador
    velJogadorY += gravidade;
    jogadorY += velJogadorY;

    if(jogadorY < 1)
    {
        jogadorY = 1;
        velJogadorY = 0;
    }

    // CPU
    velCpuY += gravidade;
    cpuY += velCpuY;

    if(cpuY < 1)
    {
        cpuY = 1;
        velCpuY = 0;
    }

    // IA
    if(bolaX > cpuX)
        cpuX += 0.03f;

    if(bolaX < cpuX)
        cpuX -= 0.03f;

    // Colisoes
    colisaoJogador(jogadorX,jogadorY);
    colisaoJogador(cpuX,cpuY);

    // Gol esquerdo
    if(bolaX < -11 && bolaY < 4)
    {
        placarCPU++;
        resetarBola();
    }

    // Gol direito
    if(bolaX > 11 && bolaY < 4)
    {
        placarJogador++;
        resetarBola();
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

//=====================================
// TECLADO
//=====================================

void tecladoEspecial(int key,int x,int y)
{
    switch(key)
    {
        case GLUT_KEY_LEFT:
            jogadorX -= 0.3f;
            break;

        case GLUT_KEY_RIGHT:
            jogadorX += 0.3f;
            break;

        case GLUT_KEY_UP:

            if(jogadorY <= 1.05f)
                velJogadorY = 0.12f;

            break;
    }
}

//=====================================
// CAMPO
//=====================================

void desenharCampo()
{
    glColor3f(0,0.7f,0);

    glBegin(GL_QUADS);

    glNormal3f(0,1,0);

    glVertex3f(-15,0,-5);
    glVertex3f(15,0,-5);
    glVertex3f(15,0,5);
    glVertex3f(-15,0,5);

    glEnd();
}

//=====================================
// GOLS
//=====================================

void desenharGol(float x)
{
    glPushMatrix();

    glTranslatef(x,1.5f,0);

    glutWireCube(3);

    glPopMatrix();
}

//=====================================
// DISPLAY
//=====================================

void display()
{
    glClear(GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    gluLookAt(
        0,10,20,
        0,0,0,
        0,1,0
    );

    GLfloat pos[] =
    {
        0,
        20,
        10,
        1
    };

    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        pos
    );

    desenharCampo();

    desenharGol(-13);
    desenharGol(13);

    // Jogador
    glPushMatrix();

    glColor3f(0,0,1);

    glTranslatef(
        jogadorX,
        jogadorY,
        0
    );

    glutSolidSphere(
        raioCabeca,
        30,
        30
    );

    glPopMatrix();

    // CPU
    glPushMatrix();

    glColor3f(1,0,0);

    glTranslatef(
        cpuX,
        cpuY,
        0
    );

    glutSolidSphere(
        raioCabeca,
        30,
        30
    );

    glPopMatrix();

    // Bola
    glPushMatrix();

    glColor3f(1,1,1);

    glTranslatef(
        bolaX,
        bolaY,
        0
    );

    glutSolidSphere(
        raioBola,
        30,
        30
    );

    glPopMatrix();

    desenharTexto(
        550,
        680,
        std::to_string(placarJogador)
        + " x " +
        std::to_string(placarCPU)
    );

    glutSwapBuffers();
}

//=====================================
// INIT
//=====================================

void init()
{
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);

    glClearColor(
        0.4f,
        0.7f,
        1.0f,
        1.0f
    );
}

//=====================================
// RESHAPE
//=====================================

void reshape(int w,int h)
{
    if(h == 0) h = 1;

    glViewport(0,0,w,h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        45,
        (float)w/h,
        0.1,
        100
    );

    glMatrixMode(GL_MODELVIEW);
}

//=====================================
// MAIN
//=====================================

int main(int argc,char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutInitWindowSize(
        1280,
        720
    );

    glutCreateWindow(
        "Head Soccer 3D"
    );

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glutSpecialFunc(
        tecladoEspecial
    );

    glutTimerFunc(
        16,
        update,
        0
    );

    glutMainLoop();

    return 0;
}