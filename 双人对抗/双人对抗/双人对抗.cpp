#include <iostream>
#include <ctime>
#include<cstdlib>
#include<conio.h>  //用于_kibhit()  _getch()   windows控制台输入
#include<windows.h>
using namespace std;

//地图尺寸
const int MAP_WIDTH = 80;
const int MAP_HEIGHT = 25;

//地图符号
const char WALL = '#';         //障碍
const char RIVER = '~';        //河流
const char PLAYER1 = '1';      //玩家
const char PLAYER2 = '2';
const char EMPTY = '.';        //空地
const char TREE = 'T';         //树木
const char MINISTONE = 'm';    //小石子

//动作设置
DWORD p1_pick_start = 0;
bool p1_picking = false;
DWORD p1_chop_start = 0;
bool p1_choping = false;

DWORD p2_pick_start = 0;
bool p2_picking = false;
DWORD p2_chop_start = 0;
bool p2_choping = false;

//道具符号
const char BOW = 'B';           //弓箭
const char SHIELD = 'S';        //护盾
const char HP_POTION = 'H';     //恢复药水

//人物属性
const int MAX_HP=10;            //人物血量最大值
const int MINISTONE_NUM = 0;    //拥有石头数
const int TREE_NUM = 0;         //拥有木材数
bool p1_has_sword = false;      //玩家是否持有武器
bool p2_has_sword = false;

//游戏判定
bool gameOver = false;
int winer = 0;

int main() {
	
	//人物属性初始化
	int p1_HP = MAX_HP;
	int p2_HP = MAX_HP;

	int p1_stone = MINISTONE_NUM;
	int p2_stone = MINISTONE_NUM;

	int p1_tree = TREE_NUM;
	int p2_tree = TREE_NUM;


	//初始化随机数
	srand((unsigned)time(NULL));
	//定义二维数组地图
	char map[MAP_HEIGHT][MAP_WIDTH];

	//地图初始化
	for (int y = 0; y < MAP_HEIGHT; y++) {
		for (int x = 0; x < MAP_WIDTH; x++) {
			map[y][x] = EMPTY;
		}
	}
	for (int x = 0; x < MAP_WIDTH; x++) {
		map[0][x] = WALL;
		map[MAP_HEIGHT - 1][x] = WALL;
	}
	//地图边界围墙
	for (int y = 0; y < MAP_HEIGHT; y++) {
		map[y][0] = WALL;
		map[y][MAP_WIDTH - 1] = WALL;
	}
	//生成树木
	for (int i = 0; i < 40; i++) {
		int x = rand() % MAP_WIDTH;
		int y = rand() % MAP_HEIGHT;
		if (map[y][x] == EMPTY) {
			map[y][x] = TREE;
		}
	}
	//生成小石子
	for (int i = 0; i < 40; i++) {
		int x = rand() % MAP_WIDTH;
		int y = rand() % MAP_HEIGHT;
		if (map[y][x] == EMPTY) {
			map[y][x] = MINISTONE;
		}
	}
	//生成河流
	int riverY = MAP_HEIGHT / 2;
	for (int x = 20; x <= 60; x++) {
		map[riverY][x] = RIVER;
	}
	//创建人物
	int p1_x = 5;
	int p1_y = 5;
	map[p1_y][p1_x] = PLAYER1;
	int p2_x = MAP_WIDTH - 6;
	int p2_y = MAP_HEIGHT - 6;
	map[p2_y][p2_x] = PLAYER2;

	
	//无限刷新页面
	while (!gameOver) {                 //保持游戏运行，按q退出
		system("cls");//清屏
		//渲染地图
		for (int y = 0; y < MAP_HEIGHT; y++) {
			for (int x = 0; x < MAP_WIDTH; x++) {
				cout << map[y][x];
			}
			cout << endl;
		}
			//血量显示      使用“#”代替桃心
			cout << "玩家1血量： ";
			for (int i = 0; i < p1_HP; i++) {
				cout << " # ";
			}
			cout << endl;
			cout << "  石子数:  " << p1_stone << "      " << "木头数:  " << p1_tree << endl;
			cout << "玩家2血量： ";
			for (int i = 0; i < p2_HP; i++) {
				cout << " # ";
			}
			cout << endl;
			cout << "  石子数:  " << p2_stone << "      " << "木头数:  " << p2_tree << endl;
		if (_kbhit()) {              //检测是否有键盘输入，不阻塞循环，保证游戏画面不被卡住
			// 核心修复：用int接收_getch()的返回值，避免键值溢出
			int key = _getch();
			if (key == 'q' || key == 'Q') {
				break;
			}

			// 玩家1移动操作控制（WASD）
			int new_p1_x = p1_x;
			int new_p1_y = p1_y;
			bool p1_moving = false;

			switch (key) {
			case'w':case'W':new_p1_y--; p1_moving = true; break;     //上
			case'a':case'A':new_p1_x--; p1_moving = true; break;     //左
			case's':case'S':new_p1_y++; p1_moving = true; break;     //下
			case'd':case'D':new_p1_x++; p1_moving = true; break;     //右
			case' ':case'f':case'F':    
				//空格键或者F键攻击
				int range = p1_has_sword ? 2 : 1;
				int damage = p1_has_sword ? 2 : 1;

				// 攻击判定
				if (abs(p1_x - p2_x) <= range && abs(p1_y - p2_y) <= range) {
					p2_HP -= damage;
					if (p2_HP < 0) p2_HP = 0; // 防止血量为负
					gameOver = true;
					winer = 1;
				};
			
			}
			//判断周围有石头
			if (key == 'E' || key == 'e'){
				bool has_stone = false;
				for (int dy = -1; dy <= 1; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						int x = p1_x + dx;
						int y = p1_y + dy;
						if (map[y][x] == MINISTONE) {
							has_stone = true;
						}
					}
				}
				if (has_stone && !p1_picking) {
					p1_pick_start = GetTickCount();//开始计时
					p1_picking = true;
				}
				DWORD now = GetTickCount();
				if (p1_picking && now - p1_pick_start >= 1500) {
					for (int dy = -1; dy <= 1; dy++) {
						for (int dx = -1; dx <= 1; dx++) {
							int x = p1_x + dx;
							int y = p1_y + dy;
							if (map[y][x] == MINISTONE) {
								map[y][x] = EMPTY;
								p1_stone++;
								p1_picking = false;
								dx = 2;
								dy = 2;
							}
						}
					}
				}
			}
			else {
				p1_picking = false;
			}
			//判断周围有木头
			if (key == 'C' || key == 'c') {
				bool has_tree = false;
				for (int dy = -1; dy <= 1; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						int x = p1_x + dx;
						int y = p1_y + dy;
						if (map[y][x] == TREE) {
							has_tree = true;
						}
					}
				}
				if (has_tree && !p1_choping) {
					p1_chop_start = GetTickCount();//开始计时
					p1_choping = true;
				}
				DWORD now = GetTickCount();
				if (p1_choping && now - p1_chop_start >= 3000) {
					for (int dy = -1; dy <= 1; dy++) {
						for (int dx = -1; dx <= 1; dx++) {
							int x = p1_x + dx;
							int y = p1_y + dy;
							if (map[y][x] == TREE) {
								map[y][x] = EMPTY;
								p1_tree++;
								p1_choping = false;
								dx = 2;
								dy = 2;
							}
						}
					}
				}
			}
			else {
				p1_choping = false;
			}
			//玩家1合成武器
			if (key == 'r' || key == 'R') {
				if (!p1_has_sword && p1_stone >= 2 && p1_tree >= 1) {
					p1_stone -= 2;
					p1_tree -= 1;
					p1_has_sword = true;
				}
			}
			//玩家1移动逻辑
			if (p1_moving) {
				bool canMove = true;
				//碰撞检测：墙、树、玩家2
				if (map[new_p1_y][new_p1_x] == WALL ||
					map[new_p1_y][new_p1_x] == TREE ||
					map[new_p1_y][new_p1_x] == MINISTONE||
					(new_p1_y == p2_y && new_p1_x == p2_x)) {
					canMove = false;
				}

				if (canMove) {
					//还原旧位置
					map[p1_y][p1_x] = EMPTY;
					if (p1_y == riverY && p1_x >= 20 && p1_x <= 60) {
						map[p1_y][p1_x] = RIVER;
					}
					//更新玩家坐标
					p1_y = new_p1_y;
					p1_x = new_p1_x;
					//绘制新位置
					map[p1_y][p1_x] = PLAYER1;
					//河流减速
					if (p1_y == riverY && p1_x >= 20 && p1_x <= 60) {
						Sleep(30);
					}
				}
			}

			// 玩家2移动控制
			int new_p2_x = p2_x;
			int new_p2_y = p2_y;
			bool p2_moving = false;

			// 方案1：主键盘↑↓←→箭头方向键
			if (key == 0xE0) {
				key = _getch();
				switch (key) {
				case 72: new_p2_y--; p2_moving = true; break;  // 上
				case 80: new_p2_y++; p2_moving = true; break;  // 下
				case 75: new_p2_x--; p2_moving = true; break;  // 左
				case 77: new_p2_x++; p2_moving = true; break;  // 右
				case'/':case'？':                              //“/”或者“？”键攻击
					// / 键或者?键攻击
					int range = p2_has_sword ? 2 : 1;
					int damage = p2_has_sword ? 2 : 1;

					// 攻击判定
					if (abs(p1_x - p2_x) <= range && abs(p1_y - p2_y) <= range) {
						p1_HP -= damage;
						if (p1_HP < 0) p1_HP = 0; // 防止血量为负
						gameOver = true;
						winer = 2;
					}
				}
			}

			// 备用方案2：小键盘8246
			switch (key) {
			case'8':new_p2_y--; p2_moving = true; break;     //上
			case'5':new_p2_y++; p2_moving = true; break;     //下
			case'4':new_p2_x--; p2_moving = true; break;     //左
			case'6':new_p2_x++; p2_moving = true; break;     //右
			case'/':case'？':                              //“/”或者“？”键攻击
				// / 键或者?键攻击
				int range = p2_has_sword ? 2 : 1;
				int damage = p2_has_sword ? 2 : 1;

				// 攻击判定
				if (abs(p1_x - p2_x) <= range && abs(p1_y - p2_y) <= range) {
					p1_HP -= damage;
					if (p1_HP < 0) p1_HP = 0; // 防止血量为负
				}
			}
			//判断周围有石头
			if (key == ',' || key == '<') {
				bool has_stone = false;
				for (int dy = -1; dy <= 1; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						int x = p2_x + dx;
						int y = p2_y + dy;
						if (map[y][x] == MINISTONE) {
							has_stone = true;
						}
					}
				}
				if (has_stone && !p2_picking) {
					p2_pick_start = GetTickCount();//开始计时
					p2_picking = true;
				}
				DWORD now = GetTickCount();
				if (p2_picking && now - p2_pick_start >= 1500) {
					for (int dy = -1; dy <= 1; dy++) {
						for (int dx = -1; dx <= 1; dx++) {
							int x = p2_x + dx;
							int y = p2_y + dy;
							if (map[y][x] == MINISTONE) {
								map[y][x] = EMPTY;
								p2_stone++;
								p2_picking = false;
								dx = 2;
								dy = 2;
							}
						}
					}
				}
			}
			else {
				p2_picking = false;
			}
			//判断周围有木头
			if (key == '.' || key == '>') {
				bool has_tree = false;
				for (int dy = -1; dy <= 1; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						int x = p2_x + dx;
						int y = p2_y + dy;
						if (map[y][x] == TREE) {
							has_tree = true;
						}
					}
				}
				if (has_tree && !p2_choping) {
					p2_chop_start = GetTickCount();//开始计时
					p2_choping = true;
				}
				DWORD now = GetTickCount();
				if (p2_choping && now - p2_chop_start >= 3000) {
					for (int dy = -1; dy <= 1; dy++) {
						for (int dx = -1; dx <= 1; dx++) {
							int x = p2_x + dx;
							int y = p2_y + dy;
							if (map[y][x] == TREE) {
								map[y][x] = EMPTY;
								p2_tree++;
								p2_choping = false;
								dx = 2;
								dy = 2;
							}
						}
					}
				}
			}
			else {
				p2_choping = false;
			}
			//玩家2合成武器
			if (key == 'm' || key == 'M') {
				if (!p2_has_sword && p2_stone >= 2 && p2_tree >= 1) {
					p2_stone -= 2;
					p2_tree -= 1;
					p2_has_sword = true;
				}
			}
			//玩家2移动逻辑
			if (p2_moving) {
				bool canMove2 = true;
				//碰撞检测：墙、树、玩家1
				if (map[new_p2_y][new_p2_x] == WALL ||
					map[new_p2_y][new_p2_x] == TREE ||
					map[new_p2_y][new_p2_x] == MINISTONE ||
					(new_p2_y == p1_y && new_p2_x == p1_x)) {
					canMove2 = false;
				}

				if (canMove2) {
					//还原旧位置
					map[p2_y][p2_x] = EMPTY;
					if (p2_y == riverY && p2_x >= 20 && p2_x <= 60) {
						map[p2_y][p2_x] = RIVER;
					}
					//更新玩家坐标
					p2_y = new_p2_y;
					p2_x = new_p2_x;
					//绘制新位置
					map[p2_y][p2_x] = PLAYER2;
					//河流减速
					if (p2_y == riverY && p2_x >= 20 && p2_x <= 60) {
						Sleep(30);
					}
				}
			}
		}//大if的
		Sleep(100);   //控制刷新速度，减弱白屏闪烁 
		cout << "游戏结束" << endl;
		if (winer == 1) {
			cout << "玩家1获胜！！！";
		}
		if (winer == 2) {
			cout << "玩家2获胜!!!";
		}
	}//main的

	return 0;
}