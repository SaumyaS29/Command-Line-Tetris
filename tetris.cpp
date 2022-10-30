#include <Windows.h>
#include <thread>
#include <chrono>
#include <vector>

#define FIELD_WIDTH 10
#define FIELD_HEIGHT 20

struct tuple {
	int x;
	int y;
};
class Screen
{
private:
	int scr_width;
	int scr_height;
	wchar_t* screen_buffer;
	HANDLE screen;
	DWORD bytes_to_write;
public:
	Screen(int width = 120, int height = 40)
	{
		scr_width = width;
		scr_height = height;
		bytes_to_write = 0;
		screen_buffer = new wchar_t[scr_width * scr_height];
		
		screen = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
		SetConsoleActiveScreenBuffer(screen);
		screen_buffer[scr_width * scr_height - 1] = '\0';
	}
	void clear()
	{
		for (int h = 0; h < scr_height; h++)
		{
			for (int w = 0; w < scr_width; w++)
			{
				screen_buffer[h * scr_width + w] = L' ';
			}
		}
	}
	void draw(const wchar_t &plot_symbol, int x, int y)
	{
		if (x > scr_width || y > scr_height) {
			return;
		}
		screen_buffer[y * scr_width + x] = plot_symbol;
	}
	void present()
	{
		WriteConsoleOutputCharacter(screen, screen_buffer, scr_width * scr_height, { 0,0 }, &bytes_to_write);
	}
	~Screen()
	{
		delete[] screen_buffer;
	}

};


class StateMemo
{
public:
	void setStartPoint(tuple& pt) {
		startpt = { pt.x,pt.y };
	}
	
	void setDispVecs(std::vector<tuple>& dispvec) {
		disvec = dispvec;
	}
	
	void setRotationState(int rtstate) {
		rotstate = rtstate;
	}
	void setFramePoint(tuple& fp) {
		framept = fp;
	}
	
	tuple& getStartPoint() { return startpt; }
	tuple& getFramePt() { return framept; }
	std::vector<tuple>& getDispVecs() { return disvec; }
	int getRotationState() { return rotstate; }

private:
	tuple startpt;
	tuple framept;
	std::vector<tuple>disvec;
	int rotstate;
};


class Tetromino
{
public:
	virtual void initialize(int sx, int sy) = 0;
	void processInput() {
		if (GetAsyncKeyState((unsigned short)'S')&0x8000) {
			vx = 2;
			vy = 0;
		}
		else if (GetAsyncKeyState((unsigned short)'A') & 0x8000) {
			vy = -1;
			vx = 0;
		} 
		else if (GetAsyncKeyState((unsigned short)'D') & 0x8000) {
			vy = 1;
			vx = 0;
		}
		else if (GetAsyncKeyState((unsigned short)'R') & 0x8000) {
			rotated = true;
			vx = 0;
			vy = 0;
		}
		else {
			
			vx = 1;
			vy = 0;
		}
	}
	void update() {
		saveState();
		if (rotated) {
			rotate();
			rotated = false;
		}
		startpoint.x += vx;
		startpoint.y += vy;
		frame.x += vx;
		frame.y += vy;

		computePoints();
		computeTestPoints();
	}
	void rollBack() {
		startpoint = sm->getStartPoint();
		disp_vecs = sm->getDispVecs();
		rotstate = sm->getRotationState();
		frame = sm->getFramePt();
		computePoints();
		computeTestPoints();

	}

	std::vector<tuple>* getPoints() {
		return &tet_points;
	}
	std::vector<tuple>* getTestPoints() {
		return &test_points;
	}
	virtual ~Tetromino() {}
protected:
	tuple startpoint;
	tuple frame;
	std::vector<tuple>tet_points;
	std::vector<tuple>disp_vecs;
	std::vector<tuple>test_points;
	std::vector<tuple>testpts[4];
	int rotstate;
	bool rotated;
	StateMemo* sm;
	int vx;
	int vy;
protected:
	void computePoints() {
		tet_points.clear();
		tet_points.push_back({ startpoint.x,startpoint.y });
		for (auto& v : disp_vecs) {
			tet_points.push_back({ startpoint.x + v.x,startpoint.y + v.y });
		}
	}
	void computeTestPoints()
	{
		test_points.clear();
		tuple pt;
		for (auto& p : testpts[rotstate]) {
			pt.x = frame.x + p.x;
			pt.y = frame.y + p.y;
			test_points.push_back(pt);
		}
	}
	void saveState() {
		sm->setDispVecs(disp_vecs);
		sm->setRotationState(rotstate);
		sm->setStartPoint(startpoint);
		sm->setFramePoint(frame);
	}
	virtual void rotate() {
		switch (rotstate) {
		case 0:
			startpoint.x += 2;
			rotstate = 1;
			break;
		case 1:
			startpoint.y += 2;
			rotstate = 2;
			break;
		case 2:
			startpoint.x -= 2;
			rotstate = 3;
			break;
		case 3:
			startpoint.y -= 2;
			rotstate = 0;
			break;
		}

		for (auto &vec:disp_vecs) {
			int temp = -vec.y;
			vec.y = vec.x;
			vec.x = temp;
		}
		computePoints();
		computeTestPoints();
	}
};


class FieldOfPlay
{
public:
	FieldOfPlay(Screen *scr):_scr(scr)
	{
		std::vector<std::vector<wchar_t> >brd(height, std::vector<wchar_t>(width,' '));
		board = brd;

		for (auto& r : board) {
			r[0] = r[r.size()-1] = '|';
		}
		for (auto& c : board[board.size() - 1]) {
			c = 'I';
		}
	}

	void allowTetromino(Tetromino* t) 
	{
		bool flag=false;
		std::vector<tuple>* coll_pts = t->getTestPoints();
		std::vector<tuple>* points = t->getPoints();
		while(!flag)
		{
			for (auto& p : *points) {
				board[p.x][p.y] = ' ';
			}
			t->processInput();
			t->update();

			for (auto& p : *points) {
				if ((p.x>=height||p.y>=width)||board[p.x][p.y] != ' ') {
					t->rollBack();
					break;
				}
			}

			for (auto& p : *points) {
				board[p.x][p.y] = '#';
			}
			draw();
			_scr->present();
			for (auto& p : *coll_pts) {
				if (board[p.x][p.y] != ' ') {
					flag = true;
					break;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(130));
		}
	}

	void checkFullRows()
	{
		std::vector<int> full_row_list;
		bool full;
		for (int i = 4; i < (height-1); i++) {
			full = true;
			for (int c = 1; c < (width - 1);c++) {
				if (board[i][c] != '#') {
					full = false;
					break;
				}
			}
			if (full) {
				full_row_list.push_back(i);
			}
		}
		if (full_row_list.empty())return;

		for (auto& fr : full_row_list) {
			for (int c = 1; c < (width - 1); c++) {
				board[fr][c] = '-';
			}
		}
		draw();
		_scr->present();
		std::this_thread::sleep_for(std::chrono::seconds(2));
		for (auto& fr : full_row_list) {
			scrollFromN(fr);
		}

	}

	bool isTopReached()
	{
		for (auto& c : board[3]) {
			if (c == '#') return true;
		}
		return false;
	}
private:
	std::vector<std::vector<wchar_t> >board;
	Screen* _scr;
	const int width = FIELD_WIDTH;
	const int height = FIELD_HEIGHT;

	void scrollFromN(int n) {
		for (int i = n; i > 4; i--)
		{
			int j = i - 1;
			std::copy(board[j].begin(), board[j].end(), board[i].begin());
		}
		for (int i = 1; i < (width - 1); i++) {
			board[4][i] = ' ';
		}
	}

	void draw() {
		_scr->clear();
		int x = 0, y = 0;
		for (auto& r : board) {
			y = 0;
			for (auto& c : r) {
				_scr->draw(c, y++, x);
			}
			x++;
		}
	}
};


class LeftLTetromino :public Tetromino
{
public:
	LeftLTetromino(StateMemo* m) {
		sm = m;
		tet_points.reserve(4);
		disp_vecs.reserve(3);
		test_points.reserve(10);
	}
	void initialize(int sx, int sy)override {
		tet_points.clear();
		disp_vecs.assign({{-1,0},{0,1},{0,2} });
		frame = { sx,sy };
		startpoint = { frame.x + 2,frame.y };
		rotstate = 1;
		testpts[1].assign({ {3,0},{3,1},{3,2} });
		testpts[2].assign({ {3,1},{3,2} });
		testpts[3].assign({ {1,0},{1,1},{2,2} });
		testpts[0].assign({ {1,1},{3,0} });

		computePoints();
		computeTestPoints();
	}
};

class RightLTetromino :public Tetromino
{
public:
	RightLTetromino(StateMemo* m) {
		sm = m;
		tet_points.reserve(4);
		disp_vecs.reserve(3);
		test_points.reserve(10);
	}
	void initialize(int sx, int sy)override {
		tet_points.clear();
		disp_vecs.assign({ {0,1},{0,2},{-1,2} });
		frame = { sx,sy };
		startpoint = { frame.x + 2,frame.y };
		rotstate = 1;
		
		testpts[1].assign({{3,0},{3,1},{3,2} });
		testpts[2].assign({ {1,1},{3,2} });
		testpts[3].assign({ {2,0},{1,1},{1,2} });
		testpts[0].assign({ {3,0},{3,1} });

		computePoints();
		computeTestPoints();
	}
};

class ZTetromino :public Tetromino
{
public:
	ZTetromino(StateMemo* s) {
		sm = s;
		tet_points.reserve(4);
		disp_vecs.reserve(3);
		test_points.reserve(10);
	}

	void initialize(int sx, int sy)override {
		tet_points.clear();
		disp_vecs.assign({ {0,-1},{-1,-1},{-1,-2} });
		frame = { sx,sy };
		startpoint = { frame.x + 2,frame.y + 2 };
		rotstate = 2;

		testpts[2].assign({ {2,0},{3,1},{3,2} });
		testpts[3].assign({ {3,1},{2,2} });
		testpts[0].assign({ {1,0},{2,1},{2,2} });
		testpts[1].assign({ {3,0},{2,1} });

		computePoints();
		computeTestPoints();
	}
};

class ReverseZTetromino :public Tetromino
{
public:
	ReverseZTetromino(StateMemo* s)
	{
		sm = s;
		tet_points.reserve(4);
		disp_vecs.reserve(3);
		test_points.reserve(10);
	}

	void initialize(int sx, int sy)override {
		tet_points.clear();
		disp_vecs.assign({ {0,1},{-1,1},{-1,2} });
		frame = { sx,sy };
		startpoint = { frame.x + 2,frame.y };
		rotstate = 1;

		testpts[1].assign({ {3,0},{3,1},{2,2} });
		testpts[2].assign({ {2,1},{3,2} });
		testpts[3].assign({ {2,0},{2,1},{1,2} });
		testpts[0].assign({ {2,0},{3,1} });

		computePoints();
		computeTestPoints();
	}
};

class SquareTetromino :public Tetromino
{
public:
	SquareTetromino(StateMemo* s)
	{
		sm = s;
		tet_points.reserve(4);
		disp_vecs.reserve(3);
		test_points.reserve(2);
		testpts[1].clear();
		testpts[2].clear();
		testpts[3].clear();
	}

	void initialize(int sx, int sy)override {
		tet_points.clear();
		disp_vecs.assign({ {1,0},{0,1},{1,1} });
		frame = { sx,sy };
		startpoint = { frame.x ,frame.y };
		rotstate = 0;

		testpts[0].assign({ {2,0},{2,1} });
		
		computePoints();
		computeTestPoints();
	}
private:
	void rotate()override {}
};

class TetrominoFactory
{
public:
	TetrominoFactory(StateMemo* s) {
		tetromino_list[0] = new LeftLTetromino(s);
		tetromino_list[1] = new RightLTetromino(s);
		tetromino_list[2] = new ZTetromino(s);
		tetromino_list[3] = new ReverseZTetromino(s);
		tetromino_list[4] = new SquareTetromino(s);

		srand(58);
	}
	Tetromino* spawnTetromino() {
		int x = 1;
		int y = rand() % (FIELD_WIDTH - 4);
		if (y == 0) {
			y = 1;
		}
		int index = rand() % 5;
		tetromino_list[index]->initialize(x, y);
		return tetromino_list[index];
	}
	~TetrominoFactory() {
		for (int i = 0; i < 5; i++) {
			delete tetromino_list[i];
		}
	}
private:
	Tetromino* tetromino_list[5];
};
int main()
{
	StateMemo* s = new StateMemo();
	TetrominoFactory tf(s);
	Tetromino* tet;
	Screen* scr = new Screen();
	FieldOfPlay f(scr);
	while (!f.isTopReached()) {
		tet = tf.spawnTetromino();
		f.allowTetromino(tet);
		f.checkFullRows();
	}
	delete s;
	delete scr;
	return 0;
}
