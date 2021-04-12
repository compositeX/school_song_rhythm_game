/*
Version 1.0 - 20210329a
Program/ZBH Rhythm/YZL

compile command：-lglu32 -lglut -lopengl32 -lwinmm -lgdi32
(GLUT libs required)
*/
#include<bits/stdc++.h>
#include<windows.h>
#define GLUT_DISABLE_ATEXIT_HACK
#include<gl/glut.h>
#include<mmsystem.h>

using std::vector;
using std::queue;
using std::ifstream;
using std::string;

#define DATA_PATH "assets\\data.txt"

#define REACT_PERFECT 0x01
#define REACT_GOOD 0x02
#define REACT_EARLY 0x03
#define REACT_LATE 0x04
#define REACT_MISS 0x05

class note {
	public:
		int line;
		int tickToReach;
		note(ifstream& FILE) {
			FILE>>line>>tickToReach;
		};
};

class _dataLock {
	int lockStatus;
	public:
		_dataLock() {
			lockStatus = 0;
		}
		void claim(void) {
			while(lockStatus);
			lockStatus = 1;
		}
		void free(void) {
			lockStatus = 0;
		}
} dataLock;

class _gameStat {
	public:
		int started;
		int dataLength;
		int totalTicks;
		vector<note> mainData;
		int dataLoadPos;
		queue<int> loadedNotes[4];
		
		int initialTick;
		
		__int64 maxScore, score;
		__int64 scoreRate, scoreRateShow;
		int combo; 
		
		_gameStat() {
			started = 0;
		}
		
		void init(void) { // 初始化数据，读取配置 
			char dataPath[] = DATA_PATH;
			
			ifstream FILE(dataPath);
			FILE>>dataLength>>totalTicks;
			for(int i=0; i < dataLength; i++) {
				note newNote(FILE);
				mainData.push_back(newNote);
			}
			
			if(dataLength > 5)
				maxScore = dataLength * 15 - 25;
			else maxScore = dataLength * 10;
			
			dataLoadPos = 0;
			score = 0;
			combo = 0;
			return;
		}
		
		void start(void) {
			initialTick = clock();
			PlaySound(TEXT("assets\\mus.wav"), NULL, SND_FILENAME | SND_ASYNC);
			started = 1;
			return;
		}
		
		int currentTick(void){
			return clock()-initialTick;
		}
		
		void loadData(void){ // 刷新数据加载
			int tick = currentTick();
			if(dataLoadPos >= dataLength)
				return;
			while(mainData[dataLoadPos].tickToReach < tick + 1000) {
				
				loadedNotes[mainData[dataLoadPos].line].push(mainData[dataLoadPos].tickToReach);
				dataLoadPos++;
				if(dataLoadPos >= dataLength)
					return;
			}
		}
		
		void refreshScoreShow(void){ // 刷新显示分数 
			scoreRate = score * 10000000 / maxScore;
			if(scoreRate == 10000000)
				scoreRate = 9999999;
			
			if(currentTick() > totalTicks){
				char scoreInfo[32]="";
				sprintf(scoreInfo, "得分：%d", scoreRate);
				MessageBox(NULL, scoreInfo, "OpenGL Project", MB_OK);
				exit(0);
			}

			if(currentTick() > totalTicks - 100) 
				scoreRateShow = scoreRate;
			else {
				scoreRateShow += (scoreRate - scoreRateShow) / 30 + 10000000 / totalTicks;
				if(scoreRateShow > scoreRate)
					scoreRateShow = scoreRate;
			}
		}
} gameStat;

class _lineStatus { // 存储操作结果，以提供反馈 
	public:
		int reactionType;
		int reactionTick;
		void refresh(int newReactionType) {
			reactionType = newReactionType;
			reactionTick = gameStat.currentTick();
		}
		_lineStatus() {
			reactionType = 0;
			reactionTick = 0;
		}
} lineStatus[4];

void reactToOperation(int lineOperated){ // 响应有效键盘操作 
	dataLock.claim();
	if(gameStat.loadedNotes[lineOperated].empty()) {
		dataLock.free();
		return;
	}
	
	int tick = gameStat.currentTick();
	int tickStandard = gameStat.loadedNotes[lineOperated].front();
	int tickDelay = tick - tickStandard;
	if(tickDelay < -150) {
		dataLock.free();
		return;
	}
	
	if(tickDelay < -60) {
		lineStatus[lineOperated].refresh(REACT_EARLY);
		if(gameStat.combo > 4)
			gameStat.score += 6;
		else gameStat.score += 4;
	}
	else if(tickDelay > 60) {
		lineStatus[lineOperated].refresh(REACT_LATE);
		if(gameStat.combo > 4)
			gameStat.score += 6;
		else gameStat.score += 4;
	}
	else if(tickDelay > 30 || tickDelay < -30) {
		lineStatus[lineOperated].refresh(REACT_GOOD);
		if(gameStat.combo > 4)
			gameStat.score += 12;
		else gameStat.score += 8;
	}
	else {
		lineStatus[lineOperated].refresh(REACT_PERFECT);
		if(gameStat.combo > 4)
			gameStat.score += 15;
		else gameStat.score += 10;
	}
	
	gameStat.combo++;
	gameStat.loadedNotes[lineOperated].pop();
	dataLock.free();
}

void reactToKey(unsigned char key, int, int) { // 处理键盘操作 
	if(gameStat.started) {
		if(key > 48 && key < 53) 
			reactToOperation((int)(key) - 49);
	}
	else gameStat.start();
}

void refreshStatus(void) {  // 维护数据状态 
	int tick = gameStat.currentTick();
	
	for(int i=0; i<4; i++) {
		if(gameStat.loadedNotes[i].empty())
			continue;
		int tickStandard = gameStat.loadedNotes[i].front();
		if(tick - tickStandard > 150) {
			lineStatus[i].refresh(REACT_MISS);
			gameStat.combo = 0;
			gameStat.loadedNotes[i].pop();
		}
	}
	
	gameStat.refreshScoreShow();
	gameStat.loadData();
}

void initLights() { // 初始化光照 
	GLfloat ambientLight[]={0.5f, 0.5f, 0.5f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	
	GLfloat diffuseLight[]={0.9f, 0.9f, 0.9f, 1.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	
	GLfloat specularLight[]={0.6f, 0.6f, 0.6f, 1.0f};
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	
	GLfloat lightPos[]={3.0f, 1.0f, 7.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	
	glEnable(GL_LIGHT0); 
	
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specularLight);
	glMateriali(GL_FRONT, GL_SHININESS, 100);
}

class fontData {
	public:
		HFONT handle;
		GLuint showList;
};
class _fontOperator {
	class fontInfo {
		string name;
		int bold;
		int height;
		public:
			fontData data;
			fontInfo(char* _name, int _bold, int _height) {
				name = _name;
				bold = _bold;
				height = _height;
			}
			bool operator==(fontInfo another) {
				return (another.name == name && another.bold == bold && another.height == height);
			}
	};
	vector<fontInfo> fontsLoaded;
	
	fontData getFont(char* name, int bold, int height) { // 加载文本字体 
		fontInfo newFont(name, bold, height);
		if(!fontsLoaded.empty())
			for(int i=0; i < fontsLoaded.size(); i++)
				if(newFont == fontsLoaded[i])
					return fontsLoaded[i].data;
		newFont.data.handle = CreateFontA(height, 0, 0, 0, bold, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name);
		newFont.data.showList = glGenLists(128);
		fontsLoaded.push_back(newFont);
		return newFont.data;
	}
	
	public:
		int windowW, windowH;
		GLuint setFont(char* name, int bold, int height) { // 设置文本字体 
			fontData newFontData = getFont(name, bold, height);
			SelectObject(wglGetCurrentDC(), newFontData.handle);
			wglUseFontBitmaps(wglGetCurrentDC(), 0, 128, newFontData.showList);
			return newFontData.showList;
		}
} fontOperator;

void drawText(char* text, GLfloat posX, GLfloat posY, GLfloat heightDefault, char* fontName, int fontBold) { // 绘制文本 
	GLuint height;
	if(fontOperator.windowW > fontOperator.windowH * 4/3)
		height = heightDefault * (float)(fontOperator.windowH) / 600.0f;
	else height = heightDefault * (float)(fontOperator.windowW) / 800.0f;
	
	GLuint showList = fontOperator.setFont(fontName, fontBold, (int)height);
	
	glPushMatrix();
	
	glRasterPos2f(posX, posY);
	for(; *text != '\0'; text++)
		glCallList(showList + *text);
	
	glPopMatrix();
}

void drawCuboid(GLfloat fromX, GLfloat fromY, GLfloat fromZ, GLfloat toX, GLfloat toY, GLfloat toZ) { // 绘制长方体 
	glPushMatrix();
	
	glTranslatef((toX + fromX) / 2.0f, (toY + fromY) / 2.0f, (toZ + fromZ) / 2.0f);
	glScalef(toX - fromX, toY - fromY, toZ - fromZ);
	glutSolidCube(1.0f);
	
	glPopMatrix();
}

void refreshFrame(void) { // 绘制帧 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	if(gameStat.started) {
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
		
		dataLock.claim();
		
		refreshStatus();
		
		int score = gameStat.scoreRateShow;
		int combo = gameStat.combo;
		queue<int> dataBuffer[4];
		for(int i=0; i<4; i++)
			dataBuffer[i] = gameStat.loadedNotes[i];
		_lineStatus lineBuffer[4];
		for(int i=0; i<4; i++)
			lineBuffer[i] = lineStatus[i];
		
		dataLock.free();
		
		int tick = gameStat.currentTick();
		
		for(int i=0; i<4; i++) {
			GLfloat xPos = (float)(i) - 1.9f;
			glColor3f(0.9f, 0.9f, 0.9f);
			drawCuboid(xPos, 0.0f, -24.0f, xPos + 0.8f, 0.1f, -2.0f);
			glColor3f(0.1f, 0.3f, 0.9f);
			while(!dataBuffer[i].empty()) {
				int tickDelta = tick - dataBuffer[i].front();
				dataBuffer[i].pop();
				GLfloat zPos = (float)(tickDelta) * 0.02f - 4.0f;
				drawCuboid(xPos - 0.01f, 0.1f, zPos, xPos + 0.81f, 0.2f, zPos + 2.0f);
			}
		}
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		char fontName[32] = "Segoe UI";
		char text[32] = "";
		
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		sprintf(text, "SCORE %d", score);
		drawText(text, -1.2f, 1.65f, 32.0f, fontName, FW_NORMAL);
		
		text[0] = '\0';
		sprintf(text, "COMBO %d", combo);
		if(combo >= 5)
			glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
		drawText(text, -1.2f, 1.5f, 32.0f, fontName, FW_NORMAL);
		
		for(int i=0; i<4; i++) {
			int type = lineBuffer[i].reactionType;
			int tickDelta = tick - lineBuffer[i].reactionTick;
			switch(type) {
				case 0:
					if(tickDelta < 2000) {
						glColor4f(0.0f, 0.5f, 1.0f, std::min(1.0f, 2.0f - (float)tickDelta / 1000.0f));
						text[0] = '\0';
						sprintf(text, "[%d]", i+1);
						drawText(text, 0.61f * ((float)i - 1.6f), 0.6f, 32.0f, fontName, FW_SEMIBOLD);
					}
					break;
				case REACT_PERFECT:
					if(tickDelta < 1000) {
						glColor4f(0.0f, 0.5f, 1.0f, 1.0f - (float)tickDelta / 1000.0f);
						text[0] = '\0';
						sprintf(text, "PERFECT");
						drawText(text, 0.61f * ((float)i - 1.83f), 0.6f, 32.0f, fontName, FW_SEMIBOLD);
					}
					break;
				case REACT_GOOD:
					if(tickDelta < 1000) {
						glColor4f(0.0f, 0.75f, 0.0f, 1.0f - (float)tickDelta / 1000.0f);
						text[0] = '\0';
						sprintf(text, "GOOD");
						drawText(text, 0.61f * ((float)i - 1.77f), 0.6f, 32.0f, fontName, FW_SEMIBOLD);
					}
					break;
				case REACT_EARLY:
					if(tickDelta < 1000) {
						glColor4f(0.75f, 0.0f, 1.0f, 1.0f - (float)tickDelta / 1000.0f);
						text[0] = '\0';
						sprintf(text, "EARLY");
						drawText(text, 0.61f * ((float)i - 1.77f), 0.6f, 32.0f, fontName, FW_SEMIBOLD);
					}
					break;
				case REACT_LATE:
					if(tickDelta < 1000) {
						glColor4f(1.0f, 0.5f, 0.0f, 1.0f - (float)tickDelta / 1000.0f);
						text[0] = '\0';
						sprintf(text, "LATE");
						drawText(text, 0.61f * ((float)i - 1.73f), 0.6f, 32.0f, fontName, FW_SEMIBOLD);
					}
					break;
				case REACT_MISS:
					if(tickDelta < 2000) {
						glColor4f(0.75f, 0.0f, 0.0f, std::min(1.0f, 2.0f - (float)tickDelta / 1000.0f));
						text[0] = '\0';
						sprintf(text, "MISS");
						drawText(text, 0.61f * ((float)i - 1.8f), 0.6f, 32.0f, fontName, FW_HEAVY);
					}
					break;
			}
		}
		glDisable(GL_BLEND);
	}
	else {
		char fontName[32] = "Segoe UI";
		char text[32] = "Press Any Key To Start";
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		drawText(text, -0.8f, 1.0f, 42.0f, fontName, FW_NORMAL);
	}
	
	glPopMatrix();	
	
	glutSwapBuffers();
	glutPostRedisplay();
	Sleep(10);
}

void reshape(int w, int h) { // 处理窗口变形操作 
	fontOperator.windowW = w;
	fontOperator.windowH = h;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(w > h*4/3) {
		int wFixed = h*4/3;
		glViewport((w - wFixed) / 2, 0, wFixed, h);
	}
	else if(w*3/4 < h) {
		int hFixed = w*3/4;
		glViewport(0, (h - hFixed) / 2, w, hFixed);
	}
	else glViewport(0, 0, w, h);
	gluPerspective(46.8265f, 1.3333f, 3.4641f, 27.4642f);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0f, 1.0f, 3.4641f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
}

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH);
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(800, 600);
	glutCreateWindow("OpenGL Project");
	
	glutReshapeFunc(reshape);
	glutDisplayFunc(refreshFrame);
	glutKeyboardFunc(reactToKey);
	
	initLights();
	
	gameStat.init();
	glutMainLoop();
}
