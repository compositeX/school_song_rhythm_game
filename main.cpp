/*
Version 2.0 - 20210411a
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

#define TICK_JUDGEMENT_PERFECT 50
#define SCORE_PERFECT 10
#define TICK_JUDGEMENT_GOOD 100
#define SCORE_GOOD 9
#define TICK_JUDGEMENT_FAR 150
#define SCORE_FAR 6

#define TICK_LOAD_DATA 1000
#define TICK_UNLOAD_DATA 500
#define TICK_PRESS_CHECK 300
#define TICK_PRESS_CHECK_IGNORANCE 150
#define SINGLE_PRESS_TIME_THRESHOLD 60

#define JUDGEMENT_PERFECT 0
#define JUDGEMENT_GOOD 1
#define JUDGEMENT_EARLY 2
#define JUDGEMENT_LATE 3
#define JUDGEMENT_LOST 4

#define STATUS_NOT_PRESSED 0
#define STATUS_PRESSED 1
#define STATUS_LOST 2
#define STATUS_PRESSING 3

class _note {
		class noteUnit {
			public:
				int track;
				int tickFrom;
				int tickTo;
				int load(ifstream& FILE, int isContinuous) {
					FILE>>track;
					if(track == -1)
						return -1;
					if(isContinuous)
						FILE>>tickFrom>>tickTo;
					else {
						FILE>>tickFrom;
						tickTo = tickFrom;
					}
					return 0;
				}
		};
	public:
		int identifier;
		int isFloating;
		int isContinuous;
		vector<noteUnit> data;

		_note(ifstream& FILE, int identifierGiven) {
			identifier = identifierGiven;
			FILE>>isFloating>>isContinuous;
			if(isContinuous && isFloating) {
				while(true) {
					noteUnit newUnit;
					if(newUnit.load(FILE, 1) == -1)
						break;
					data.push_back(newUnit);
				}
				return;
			}
			noteUnit unit;
			unit.load(FILE, isContinuous);
			data.push_back(unit);
		}

		int checkedTimes(void) { // 计算被判定次数
			if(!isContinuous)
				return 1;
			int sum = 1;
			for(int i=0; i < data.size(); i++) {
				int firstCheck = (data[i].tickFrom + TICK_PRESS_CHECK_IGNORANCE - 1) / TICK_PRESS_CHECK;
				int lastCheck = (data[i].tickTo - TICK_PRESS_CHECK_IGNORANCE) / TICK_PRESS_CHECK;
				if(firstCheck <= lastCheck)
					sum += lastCheck - firstCheck + 1;
			}
			return sum;
		}
};
bool operator<(_note a, _note b) {
	return a.data[0].tickFrom < b.data[0].tickFrom;
}

class levelStat {
		string filePath;
	public:
		string name;
		int dataLength;
		int lastTick;
		vector<_note> musicData;

		levelStat() {}

		levelStat(ifstream& FILE0) {
			FILE0>>filePath;
			filePath.insert(0, "assets\\");
			ifstream FILE1(filePath.c_str());
			FILE1>>name>>dataLength>>lastTick;
			for(int i=0; i < dataLength; i++) {
				_note newNote(FILE1, i);
				musicData.push_back(newNote);
			}
			FILE1.close();
			std::sort(musicData.begin(), musicData.end());
			for(int i=0; i < dataLength; i++) 
				musicData[i].identifier = i;
		}
};

class levelActivated: public levelStat {
	public:
		vector<int> noteStatus;

		vector<_note> noteBuffer;
		int dataLoadPos;

		int initialTick;
		int nextCheckTick;

		__int64 maxScore, score;
		__int64 scoreRate, scoreRateShow;
		int combo;

		__int64 calcMaxScore(void) {
			int sum = 0;
			for(int i=0; i < dataLength; i++)
				sum += musicData[i].checkedTimes();
			return sum * SCORE_PERFECT;
		}

		void operator= (levelStat stat) {
			name = stat.name;
			dataLength = stat.dataLength;
			lastTick = stat.lastTick;
			musicData = stat.musicData;
			noteStatus.resize(dataLength);
			std::fill(noteStatus.begin(), noteStatus.end(), STATUS_NOT_PRESSED);

			maxScore = calcMaxScore();
			dataLoadPos = 0;
			score = 0;
			combo = 0;

			nextCheckTick = TICK_PRESS_CHECK;
			initialTick = clock();
			PlaySound(TEXT("assets\\mus.wav"), NULL, SND_FILENAME | SND_ASYNC);
		}


		int currentTick(void) {
			return clock()-initialTick;
		}

		int nearestNote(int track_8) { // 确定某轨道上最近音符
			int track = track_8 % 4;
			int isFloating = track_8 / 4;
			int noteId = -1;
			int minTick = 0;
			for(int i=0; i < noteBuffer.size(); i++) {
				int currentNoteId = noteBuffer[i].identifier;
				if(noteStatus[currentNoteId] == STATUS_NOT_PRESSED && noteBuffer[i].data[0].track == track && noteBuffer[i].isFloating == isFloating)
					if(noteId == -1 || noteBuffer[i].data[0].tickFrom < minTick) {
						noteId = currentNoteId;
						minTick = noteBuffer[i].data[0].tickFrom;
					}
			}
			return noteId;
		}

		void maintainBuffer(void) { // 维护缓冲列表
			int tick = currentTick();

			for(int i=0; i < noteBuffer.size(); i++) { // 移除
				int noteId = noteBuffer[i].identifier;
				if(tick > musicData[noteId].data[musicData[noteId].data.size() - 1].tickTo + TICK_UNLOAD_DATA) {
					noteBuffer.erase(noteBuffer.begin() + i);
					i--;
				}
			}

			if(dataLoadPos >= dataLength)
				return;
			while(musicData[dataLoadPos].data[0].tickFrom < tick + TICK_LOAD_DATA) { // 加载
				noteBuffer.push_back(musicData[dataLoadPos]);
				dataLoadPos++;
				if(dataLoadPos >= dataLength)
					return;
			}
		}

		void showResult(void) {
			char scoreInfo[32] = "";
			sprintf(scoreInfo, "得分：%07d", scoreRate);
			MessageBox(NULL, scoreInfo, "OpenGL Project", MB_OK);
			exit(0);
		}

		void refreshScoreShow(void) { // 刷新分数显示
			scoreRate = score * 9999999 / maxScore;
			
			if(currentTick() > lastTick)
				showResult();

			if(currentTick() > lastTick - 50)
				scoreRateShow = scoreRate;
			else {
				scoreRateShow += (scoreRate - scoreRateShow) / 20 + 9999999 / lastTick;
				if(scoreRateShow > scoreRate)
					scoreRateShow = scoreRate;
			}
		}
} currentLevel;

class _globalStat {
	public:
		int inLevel;
		int listLength;
		vector<levelStat> levelList;
		void load(void) {
			ifstream FILE("assets\\level");
			FILE>>listLength;
			if(listLength > 6)
				listLength = 6;
			for(int i=0; i < listLength; i++) {
				levelStat newLevel(FILE);
				levelList.push_back(newLevel);
			}
			FILE.close();
			inLevel = 0;
			return;
		}
} globalStat;

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

class _showWindowOperator {
	public:
		int width;
		int height;
		int isInLevel;
		_showWindowOperator() {
			width = 800;
			height = 600;
			isInLevel = 0;
		}
		void reshapeShow(int forced) {
			if(globalStat.inLevel == isInLevel && (!forced))
				return;
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			if(width > height * 4 / 3) {
				int wFixed = height * 4 / 3;
				glViewport((width - wFixed) / 2, 0, wFixed, height);
			}
			else if(width * 3 / 4 < height) {
				int hFixed = width * 3 / 4;
				glViewport(0, (height - hFixed) / 2, width, hFixed);
			}
			else glViewport(0, 0, width, height);
			
			if(globalStat.inLevel) {
				gluPerspective(60.0f, 1.333333f, 0.01f, 100.0f);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				gluLookAt(0.0f, 5.196152f, 3.0f, 0.0f, 0.0f, -6.0f, 0.0f, 0.866025f, -0.5f);
			}
			else {
				glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				gluLookAt(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
			}
			isInLevel = globalStat.inLevel;
		}
} showWindowOperator;
void reshape(int w, int h) { // 处理窗口变形操作
	showWindowOperator.width = w;
	showWindowOperator.height = h;
	showWindowOperator.reshapeShow(1);
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
			SelectObject(wglGetCurrentDC(), newFont.data.handle);
			wglUseFontBitmaps(wglGetCurrentDC(), 0, 128, newFont.data.showList);
			return newFont.data;
		}

	public:
		GLuint setFont(char* name, int bold, int height) { // 设置文本字体
			fontData newFontData = getFont(name, bold, height);
			SelectObject(wglGetCurrentDC(), newFontData.handle);
			return newFontData.showList;
		}
} fontOperator;

void drawText(char* text, GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat heightDefault, char* fontName, int fontBold) { // 绘制文本
	GLuint height;
	if(showWindowOperator.width > showWindowOperator.height * 4/3)
		height = heightDefault * (float)(showWindowOperator.height) / 600.0f;
	else height = heightDefault * (float)(showWindowOperator.width) / 800.0f;

	GLuint showList = fontOperator.setFont(fontName, fontBold, (int)height);

	glPushMatrix();

	glRasterPos3f(posX, posY, posZ);
	for(; *text != '\0'; text++)
		glCallList(showList + *text);

	glPopMatrix();
}


class _drawShape {
	#define POS_JUDGEMENT_LINE 1.50f
	#define POS_DISTANCE_PER_MS 0.05f
	#define POS_FARTHEST (POS_JUDGEMENT_LINE + POS_DISTANCE_PER_MS * (float)TICK_LOAD_DATA)
	class _drawNote {
		void cuboid(GLfloat fromX, GLfloat fromY, GLfloat fromZ, GLfloat toX, GLfloat toY, GLfloat toZ) { // 绘制长方体
			glPushMatrix();
	
			glTranslatef((toX + fromX) / 2.0f, (toY + fromY) / 2.0f, (toZ + fromZ) / 2.0f);
			glScalef(toX - fromX, toY - fromY, toZ - fromZ);
			glutSolidCube(1.0f);
	
			glPopMatrix();
		}
		public:
			void tap(int track, int tickDistance) {
				float posX = -2.7f + 1.8f * (float)track;
				float posZ = -(POS_JUDGEMENT_LINE + POS_DISTANCE_PER_MS * (float)tickDistance);
				glColor3f(0.1f, 0.5f, 0.9f);
				glBegin(GL_POLYGON);
				glVertex3f(posX - 0.89f, 0.01f, posZ);
				glVertex3f(posX - 0.89f, 0.01f, posZ + 2.0f);
				glVertex3f(posX + 0.89f, 0.01f, posZ + 2.0f);
				glVertex3f(posX + 0.89f, 0.01f, posZ);
				glEnd();
			}
			void press(int track, int tickFromDistance, int tickToDistance, int status) {
				float posX = -2.7f + 1.8f * (float)track;
				float posZFrom = -(POS_JUDGEMENT_LINE + POS_DISTANCE_PER_MS * (float)tickFromDistance);
				float posZTo = -(POS_JUDGEMENT_LINE + POS_DISTANCE_PER_MS * (float)tickToDistance);
				if(posZTo < -POS_FARTHEST)
					posZTo = -POS_FARTHEST;
				switch(status) {
					case STATUS_PRESSING:
						if(posZTo > -POS_JUDGEMENT_LINE)
							break;
						if(posZFrom > -POS_JUDGEMENT_LINE)
							posZFrom = -POS_JUDGEMENT_LINE;
					case STATUS_NOT_PRESSED:
						glBegin(GL_POLYGON);
						glColor3f(0.1f, 0.5f, 0.9f);
						glVertex3f(posX - 0.89f, 0.01f, posZFrom);
						glVertex3f(posX + 0.89f, 0.01f, posZFrom);
						glColor3f(0.5f, 0.5f, 0.9f);
						glVertex3f(posX + 0.89f, 0.01f, posZTo);
						glVertex3f(posX - 0.89f, 0.01f, posZTo);
						glEnd();
						break;
					case STATUS_LOST:
						glBegin(GL_POLYGON);
						glColor3f(0.35f, 0.65f, 0.95f);
						glVertex3f(posX - 0.89f, 0.01f, posZFrom);
						glVertex3f(posX + 0.89f, 0.01f, posZFrom);
						glColor3f(0.65f, 0.65f, 0.95f);
						glVertex3f(posX + 0.89f, 0.01f, posZTo);
						glVertex3f(posX - 0.89f, 0.01f, posZTo);
						glEnd();
						break;
				}
				
			}
			void floatingTap(int track, int tickDistance) {
				float posX = -2.7f + 1.8f * (float)track;
				float posZ = -(POS_JUDGEMENT_LINE + POS_DISTANCE_PER_MS * (float)tickDistance);
				glColor4f(0.8f, 0.7f, 0.95f, 0.8f);
				cuboid(posX - 0.85f, 2.0f, posZ, posX + 0.85f, 2.2f, posZ + 0.4f);
			}
	} drawNote;
	public:
		void noteVisual(_note note, int status) {
			int tick = currentLevel.currentTick();
			if(note.isFloating) {
				if(note.isContinuous) { }
				else {
					if(status != STATUS_PRESSED)
						drawNote.floatingTap(note.data[0].track, note.data[0].tickFrom - tick);
				}
			}
			else {
				if(note.isContinuous) 
					drawNote.press(note.data[0].track, note.data[0].tickFrom - tick, note.data[0].tickTo - tick, status);
				else {
					if(status != STATUS_PRESSED)
						drawNote.tap(note.data[0].track, note.data[0].tickFrom - tick);
				}
			}
		}
		void track(void) {
			glColor3f(0.95f, 0.95f, 0.95f);
			glBegin(GL_POLYGON);
			glVertex3f(3.6f, 0.0f, 0.0f);
			glVertex3f(3.6f, 0.0f, -POS_FARTHEST);
			glVertex3f(-3.6f, 0.0f, -POS_FARTHEST);
			glVertex3f(-3.6f, 0.0f, 0.0f);
			glEnd();
		} 
		void lines(void) {
			glDepthFunc(GL_ALWAYS);
			
			glColor3f(0.45f, 0.45f, 0.45f);
			glLineWidth(2.0f);
			glBegin(GL_LINES);
			glVertex3f(1.8f, 0.0f, 0.0f);
			glVertex3f(1.8f, 0.0f, -POS_FARTHEST);
			glEnd();
			glBegin(GL_LINES);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(0.0f, 0.0f, -POS_FARTHEST);
			glEnd();
			glBegin(GL_LINES);
			glVertex3f(-1.8f, 0.0f, 0.0f);
			glVertex3f(-1.8f, 0.0f, -POS_FARTHEST);
			glEnd();
			
			glLineWidth(3.5f);
			glColor3f(0.3f, 0.9f, 0.6f);
			glBegin(GL_LINES);
			glVertex3f(3.6f, 0.0f, -POS_JUDGEMENT_LINE);
			glVertex3f(-3.6f, 0.0f, -POS_JUDGEMENT_LINE);
			glEnd();
			
			glColor3f(0.6f, 0.6f, 0.6f);
			glBegin(GL_LINES);
			glVertex3f(3.61f, 0.0f, 0.0f);
			glVertex3f(3.61f, 0.0f, -POS_FARTHEST);
			glEnd();
			glBegin(GL_LINES);
			glVertex3f(-3.61f, 0.0f, 0.0f);
			glVertex3f(-3.61f, 0.0f, -POS_FARTHEST);
			glEnd();
			
			glDepthFunc(GL_LESS);
			
			glColor4f(0.2f, 0.7f, 0.9f, 0.7f);
			glBegin(GL_LINES);
			glVertex3f(4.5f, 2.0f, -1.5f-POS_JUDGEMENT_LINE);
			glVertex3f(-4.5f, 2.0f, -1.5f-POS_JUDGEMENT_LINE);
			glEnd();
		}
} drawShape;

class _keyOperator {
		int lastInputTick[8];
	public:
		_keyOperator() {
			memset(lastInputTick, 0, sizeof(lastInputTick));
		}
		int keyCorresponding(int track) {
			switch(track) {
				case 0:
					return 49;
				case 1:
					return 50;
				case 2:
					return 51;
				case 3:
					return 52;
				case 4:
					return 55;
				case 5:
					return 56;
				case 6:
					return 57;
				case 7:
					return 48;
			}
			return -1;
		}
		int trackCorresponding(int key) {
			switch(key) {
				case 48:
					return 7;
				case 49:
					return 0;
				case 50:
					return 1;
				case 51:
					return 2;
				case 52:
					return 3;
				case 55:
					return 4;
				case 56:
					return 5;
				case 57:
					return 6;
			}
			return -1;
		}

		int checkValidity(int key) { // 检测短按有效性
			int track_8 = trackCorresponding(key);
			if(track_8 == -1)
				return 0;
			int tick = currentLevel.currentTick();
			int lastTick = lastInputTick[track_8];
			lastInputTick[track_8] = tick;
			if(tick - lastTick > SINGLE_PRESS_TIME_THRESHOLD)
				return 1;
			return 0;
		}

		int checkIfPressing(int track_8) { // 检测是否长按
			int key = keyCorresponding(track_8);
			if(GetAsyncKeyState(key) & 0x8000)
				return 1;
			return 0;
		}
} keyOperator;

class _operationFeedback {
		class feedbackUnit {
			public:
				int type;
				int track;
				int isFloating;
				int tick;
				feedbackUnit(int setType, int setTrack_8) {
					type = setType;
					track = setTrack_8 % 4;
					isFloating = setTrack_8 / 4;
					tick = currentLevel.currentTick();
				}
				void showEffect(void) {
					int tickDistance = currentLevel.currentTick() - tick;
					if(type == JUDGEMENT_LOST) {
						float showPos = (float)tickDistance / 1000.0f - 0.3f;
						if(showPos > 1.0f)
							return;
						if(showPos < 0.0f)
							showPos = 0.0f;
						glColor4f(0.7f, 0.1f, 0.1f, 1.0f - showPos);
						drawText((char*)"LOST", -2.99f + 1.8f * (float)track, 0.2f + 2.0f * (float)isFloating + showPos, -1.5f - 1.5f *(float)isFloating, 26.0f, (char*)"Consolas", FW_NORMAL);
					}
					else {
						float showPos = (float)tickDistance / 1000.0f;
						if(showPos > 1.0f)
							return;
						switch(type) {
							case JUDGEMENT_PERFECT:
								glColor4f(0.3f, 0.8f, 0.9f, 1.0f - showPos);
								drawText((char*)"PERFECT", -3.21f + 1.8f * (float)track, 0.2f + 2.0f * (float)isFloating + showPos, -1.5f - 1.5f *(float)isFloating, 26.0f, (char*)"Consolas", FW_NORMAL);
								break;
							case JUDGEMENT_GOOD:
								glColor4f(0.1f, 0.75f, 0.1f, 1.0f - showPos);
								drawText((char*)"GOOD", -2.99f + 1.8f * (float)track, 0.2f + 2.0f * (float)isFloating + showPos, -1.5f - 1.5f *(float)isFloating, 26.0f, (char*)"Consolas", FW_NORMAL);
								break;
							case JUDGEMENT_EARLY:
								glColor4f(0.9f, 0.4f, 0.1f, 1.0f - showPos);
								drawText((char*)"EARLY", -3.06f + 1.8f * (float)track, 0.2f + 2.0f * (float)isFloating + showPos, -1.5f - 1.5f *(float)isFloating, 26.0f, (char*)"Consolas", FW_NORMAL);
								break;
							case JUDGEMENT_LATE:
								glColor4f(0.9f, 0.4f, 0.1f, 1.0f - showPos);
								drawText((char*)"LATE", -2.99f + 1.8f * (float)track, 0.2f + 2.0f * (float)isFloating + showPos, -1.5f - 1.5f *(float)isFloating, 26.0f, (char*)"Consolas", FW_NORMAL);
								break;
						}
					}
				}
		};
		vector<feedbackUnit> feedbackList;

		void maintainList(void) {
			int tick = currentLevel.currentTick();
			for(int i=0; i < feedbackList.size(); i++) {
				if(feedbackList[i].tick < tick - 2000) {
					feedbackList.erase(feedbackList.begin() + i);
					i--;
				}
			}
		}
	public:
		void showEffects(void) {
			maintainList();
			for(vector<feedbackUnit>::iterator i = feedbackList.begin(); i < feedbackList.end(); i++)
				(*i).showEffect();
			glColor4f(0.1f, 0.5f, 0.9f, 1.0f);
		}

		void newFeedback(int type, int track_8) {
			feedbackUnit newUnit(type, track_8);
			feedbackList.push_back(newUnit);
		}

} operationFeedback;

void responseToSinglePress(int track_8) { // 响应有效短按操作
	dataLock.claim();

	int noteId = currentLevel.nearestNote(track_8);
	if(noteId == -1) {
		dataLock.free();
		return;
	}

	int tick = currentLevel.currentTick();
	int tickStandard = currentLevel.musicData[noteId].data[0].tickFrom;
	int tickDelay = tick - tickStandard;

	if(tickDelay < -TICK_JUDGEMENT_FAR) {
		dataLock.free();
		return;
	}

	if(tickDelay < -TICK_JUDGEMENT_GOOD) {
		currentLevel.score += SCORE_FAR;
		operationFeedback.newFeedback(JUDGEMENT_EARLY, track_8);
	} else if(tickDelay > TICK_JUDGEMENT_GOOD) {
		currentLevel.score += SCORE_FAR;
		operationFeedback.newFeedback(JUDGEMENT_LATE, track_8);
	} else if(tickDelay > TICK_JUDGEMENT_PERFECT || tickDelay < -TICK_JUDGEMENT_PERFECT) {
		currentLevel.score += SCORE_GOOD;
		operationFeedback.newFeedback(JUDGEMENT_GOOD, track_8);
	} else {
		currentLevel.score += SCORE_PERFECT;
		operationFeedback.newFeedback(JUDGEMENT_PERFECT, track_8);
	}

	currentLevel.combo++;
	if(currentLevel.musicData[noteId].isContinuous)
		currentLevel.noteStatus[noteId] = STATUS_PRESSING;
	else currentLevel.noteStatus[noteId] = STATUS_PRESSED;

	dataLock.free();
}

void responseToKey(unsigned char keyChar, int, int) { // 响应键盘输入
	int key = (int)(keyChar);

	if(globalStat.inLevel) {
		if(keyOperator.checkValidity(key))
			responseToSinglePress(keyOperator.trackCorresponding(key));
	} else if(key > 48 && key < 49 + globalStat.listLength) {
		globalStat.inLevel = 1;
		currentLevel = globalStat.levelList[key - 49];
	}
}

void maintainStat(void) { // 维护数据状态；检测长按 + 判定LOST
	int tick = currentLevel.currentTick();
	int checkTick = 0;
	if(tick > currentLevel.nextCheckTick) {
		checkTick = currentLevel.nextCheckTick;
		currentLevel.nextCheckTick += TICK_PRESS_CHECK;
	}
	for(int i=0; i < currentLevel.noteBuffer.size(); i++) {
		_note currentNote = currentLevel.noteBuffer[i];
		int noteId = currentNote.identifier;
		if(currentNote.isContinuous && checkTick) { // 长按 
			if(currentLevel.noteStatus[noteId] == STATUS_PRESSING || currentLevel.noteStatus[noteId] == STATUS_LOST) {
				int checkedSegment = -1;
				for(int j=0; j < currentNote.data.size(); j++) {
					if(checkTick >= (currentNote.data[j].tickFrom + TICK_PRESS_CHECK_IGNORANCE) && checkTick <= (currentNote.data[j].tickTo - TICK_PRESS_CHECK_IGNORANCE)) {
						checkedSegment = j;
						break;
					}
				}
				if(checkedSegment != -1) {
					int track_8 = currentNote.data[checkedSegment].track + currentNote.isFloating * 4;
					if(keyOperator.checkIfPressing(track_8)) {
						operationFeedback.newFeedback(JUDGEMENT_PERFECT, track_8);
						currentLevel.combo++;
						currentLevel.score += SCORE_PERFECT;
						currentLevel.noteStatus[noteId] = STATUS_PRESSING;
					} else {
						operationFeedback.newFeedback(JUDGEMENT_LOST, track_8);
						currentLevel.combo = 0;
						currentLevel.noteStatus[noteId] = STATUS_LOST;
					}
				}
			}
		}
		if(currentLevel.noteStatus[noteId] == STATUS_NOT_PRESSED) { // LOST
			if(tick - currentNote.data[0].tickFrom > TICK_JUDGEMENT_FAR) {
				operationFeedback.newFeedback(JUDGEMENT_LOST, currentNote.data[0].track + currentNote.isFloating * 4);
				currentLevel.combo = 0;
				currentLevel.noteStatus[noteId] = STATUS_LOST;
			}
		}

	}

	currentLevel.refreshScoreShow();
	currentLevel.maintainBuffer();
}

/* ===LEGACY===
void initLights() {
	GLfloat ambientLight[]= {0.5f, 0.5f, 0.5f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);

	GLfloat diffuseLight[]= {0.9f, 0.9f, 0.9f, 1.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);

	GLfloat specularLight[]= {0.6f, 0.6f, 0.6f, 1.0f};
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

	GLfloat lightPos[]= {3.0f, 1.0f, 7.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	glEnable(GL_LIGHT0);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specularLight);
	glMateriali(GL_FRONT, GL_SHININESS, 100);
}*/

void refreshFrame(void) { // 绘制帧
	showWindowOperator.reshapeShow(0);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	if(globalStat.inLevel) {
		dataLock.claim();
		
		maintainStat();
		vector<_note> noteBuffer = currentLevel.noteBuffer;
		vector<int> statusBuffer = currentLevel.noteStatus;
		_operationFeedback feedbackBuffer = operationFeedback;
		int score = currentLevel.scoreRateShow;
		int combo = currentLevel.combo;
		
		dataLock.free();
		
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		drawShape.track();
		for(int i=0; i < noteBuffer.size(); i++) 
			drawShape.noteVisual(noteBuffer[i], statusBuffer[noteBuffer[i].identifier]);
		drawShape.lines();
		
		glColor3f(1.0f, 1.0f, 1.0f);
		char str[32] = "";
		sprintf(str, "%07d", score);
		drawText(str, -3.8f, 4.55f, -3.0f, 60.0f, (char*)"Agency FB", FW_NORMAL);
		feedbackBuffer.showEffects();
		
		if(combo > 1) {
			glColor4f(0.1f, 0.1f, 0.3f, 0.7f);
			str[0] = '\0';
			sprintf(str, "%d", combo);
			drawText((char*)str, 0.0116f - 0.1049f * strlen(str), 2.6f, 0.0f, 60.0f, (char*)"Consolas", FW_NORMAL);
		}
	}
	else {
		glColor3f(1.0f, 1.0f, 1.0f);
		char str[32] = "";
		for(int i=0; i < globalStat.listLength; i++) {
			sprintf(str, "[%d] - %s", i+1, globalStat.levelList[i].name.c_str());
			drawText(str, -0.85f, 0.75f - 0.2f * (float)i, 0.0f, 42.0f, (char*)"Segoe UI", FW_NORMAL);
			str[0] = '\0';
		}
		
		
	}

	glPopMatrix();

	glutSwapBuffers();
	glutPostRedisplay();
	Sleep(5);
}


int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH);
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(800, 600);
	glutCreateWindow("OpenGL Project");

	glutReshapeFunc(reshape);
	glutDisplayFunc(refreshFrame);
	glutKeyboardFunc(responseToKey);

	globalStat.load();
	glutMainLoop();
}
