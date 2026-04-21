#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QPainterPath>

// 检查是否有 Qt6 Multimedia 模块
#ifdef Qt6Multimedia_FOUND
#include <QMediaPlayer>
#include <QAudioOutput>
#endif

// 游戏状态枚举
enum GameState {
    STATE_MAIN_MENU,      // 主菜单
    STATE_MODE_SELECT,    // 模式选择
    STATE_SETTINGS,       // 设置界面
    STATE_GAME_PLAY,      // 游戏中
    STATE_GAME_RULES,     // 游戏玩法介绍
    STATE_GAME_OVER       // 游戏结束
};

#define MAP_ROW 20
#define MAP_COL 35
#define TILE_SIZE 32
#define MAX_HP 100
#define COLLECT_TIME 2000

// 武器系统定义
#define STONE_SWORD_COST_WOOD 1    // 合成石头剑需要的木头
#define STONE_SWORD_COST_STONE 2   // 合成石头剑需要的石头
#define STONE_SWORD_RANGE 2         // 石头剑攻击范围（默认1）
#define STONE_SWORD_DAMAGE 15       // 石头剑伤害（默认10）

// 盾牌系统定义
#define STONE_SHIELD_COST_STONE 2   // 合成石头盾牌需要的石头
#define SHIELD_DEFENSE 5            // 盾牌防御值（减少受到的伤害）
#define GAME_TIME 120               // 游戏时间（秒）

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onUpdateCollect();
    void onUpdateMovement();  // 处理持续移动

private:
    int m_map[MAP_ROW][MAP_COL] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,2,0,0,0,3,0,0,0,0,4,0,3,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,1},
        {1,0,2,2,2,0,0,4,0,4,0,3,0,2,2,0,0,0,4,0,0,0,2,2,0,0,0,0,4,0,0,0,0,0,1},
        {1,0,2,2,2,0,0,3,0,0,0,4,2,2,0,4,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,0,2,0,0,0,3,4,3,0,0,2,2,0,0,4,0,0,0,2,0,0,0,0,0,4,0,0,0,2,0,0,0,1},
        {1,0,2,0,0,4,4,0,0,0,4,0,2,2,0,0,0,4,4,2,0,0,0,0,0,4,0,0,4,2,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,2,2,0,0,0,0,2,2,0,3,3,0,0,0,0,0,2,2,0,3,3,0,1},
        {1,3,0,0,0,0,0,0,4,0,0,0,2,2,4,0,3,4,0,2,0,0,0,0,0,0,4,0,0,2,0,0,0,0,1},
        {1,3,0,4,0,0,4,0,0,0,0,2,2,0,3,0,0,0,0,0,0,4,0,3,0,0,0,0,0,0,0,4,0,0,1},
        {1,0,0,0,0,0,0,4,0,0,3,0,2,2,0,0,0,0,0,0,0,3,4,4,0,0,0,4,0,0,0,3,4,0,1},
        {1,0,0,0,0,4,0,0,0,3,0,0,2,2,0,0,0,3,0,0,0,0,0,0,0,4,0,0,3,0,0,0,0,0,1},
        {1,0,0,4,4,0,0,0,0,0,0,0,0,2,2,0,3,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,4,4,0,2,3,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,4,0,0,1},
        {1,0,0,0,3,0,0,0,0,3,4,0,3,2,2,2,0,4,4,0,0,0,0,3,0,3,0,0,3,4,0,0,0,0,1},
        {1,0,0,3,0,0,3,0,0,0,0,0,0,2,2,3,0,0,0,0,3,0,0,0,3,0,0,0,0,0,0,0,0,0,1},
        {1,0,4,0,0,4,0,4,0,4,0,0,2,2,0,0,4,0,4,0,0,4,0,4,0,4,0,4,0,0,4,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,1},
        {1,0,0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    };

    // 玩家1
    int m_p1_row = 2;
    int m_p1_col = 3;
    int m_p1_hp = MAX_HP;
    int m_p1_max_hp = MAX_HP; // 玩家1的生命值上限
    bool    m_p1_collecting = false;
    int m_p1_collect_prog = 0;
    int m_p1_collect_r = 0;
    int m_p1_collect_c = 0;
    int m_p1_collect_type = 0;
    int m_p1_wood = 0;    // 玩家1的木头数量
    int m_p1_stone = 0;    // 玩家1的石头数量
    bool m_p1_has_sword = false;  // 玩家1是否拥有石头剑
    bool m_p1_has_shield = false; // 玩家1是否拥有盾牌
    int m_p1_shield_hp = 0; // 玩家1护盾耐久度
    bool m_p1_last_attack_state = false; // 玩家1上次攻击状态
    qint64 m_p1_last_attack_time = 0; // 玩家1上次攻击时间

    // 游戏倒计时相关
    bool m_game_started = false; // 游戏是否已开始
    int m_countdown = 3; // 倒计时秒数
    qint64 m_countdown_start_time = 0; // 倒计时开始时间

    // 玩家1动画相关
    int m_p1_animation_frame = 0;
    int m_p1_animation_timer = 0;
    int m_p1_direction = 0; // 0: 右, 1: 左, 2: 上, 3: 下
    bool m_p1_is_moving = false;
    bool m_p1_is_attacking = false;
    bool m_p1_is_dead = false;
    bool m_p1_is_hit = false; // 玩家1是否受击
    int m_p1_hit_timer = 0; // 玩家1受击闪烁计时器
    int m_p1_hit_offset_x = 0; // 玩家1受击颤动X偏移
    int m_p1_hit_offset_y = 0; // 玩家1受击颤动Y偏移
    int m_p1_death_timer = 0; // 玩家1死亡动画计时器
    float m_p1_death_opacity = 1.0f; // 玩家1死亡时的透明度
    bool m_p1_death_finished = false; // 玩家1死亡动画是否完成
    int m_p1_jump_timer = 0; // 玩家1移动跳动计时器
    int m_p1_jump_offset = 0; // 玩家1跳动Y偏移

    // 玩家2
    int m_p2_row = 17;
    int m_p2_col = 33;
    int m_p2_hp = MAX_HP;
    int m_p2_max_hp = MAX_HP; // 玩家2的生命值上限
    bool    m_p2_collecting = false;
    int m_p2_collect_prog = 0;
    int m_p2_collect_r = 0;
    int m_p2_collect_c = 0;
    int m_p2_collect_type = 0;
    int m_p2_wood = 0;    // 玩家2的木头数量
    int m_p2_stone = 0;    // 玩家2的石头数量
    bool m_p2_has_sword = false;  // 玩家2是否拥有石头剑
    bool m_p2_has_shield = false; // 玩家2是否拥有盾牌
    int m_p2_shield_hp = 0; // 玩家2护盾耐久度
    
    // 玩家2动画相关
    int m_p2_animation_frame = 0;
    int m_p2_animation_timer = 0;
    int m_p2_direction = 0; // 0: 右, 1: 左, 2: 上, 3: 下
    bool m_p2_is_moving = false;
    bool m_p2_is_attacking = false;
    bool m_p2_is_dead = false;
    bool m_p2_is_hit = false; // 玩家2是否受击
    int m_p2_hit_timer = 0; // 玩家2受击闪烁计时器
    int m_p2_hit_offset_x = 0; // 玩家2受击颤动X偏移
    int m_p2_hit_offset_y = 0; // 玩家2受击颤动Y偏移
    int m_p2_death_timer = 0; // 玩家2死亡动画计时器
    float m_p2_death_opacity = 1.0f; // 玩家2死亡时的透明度
    bool m_p2_death_finished = false; // 玩家2死亡动画是否完成
    int m_p2_jump_timer = 0; // 玩家2移动跳动计时器
    int m_p2_jump_offset = 0; // 玩家2跳动Y偏移
    qint64 m_p2_last_attack_time = 0; // 玩家2上次攻击的时间戳

    // 移动状态
    bool m_p1_move_up = false;
    bool m_p1_move_down = false;
    bool m_p1_move_left = false;
    bool m_p1_move_right = false;
    bool m_p2_move_up = false;
    bool m_p2_move_down = false;
    bool m_p2_move_left = false;
    bool m_p2_move_right = false;

    QTimer *m_timer;
    QTimer *m_move_timer;
    
    // 游戏计时器
    int m_game_time; // 剩余时间（秒）
    bool m_game_over; // 游戏是否结束
    qint64 m_start_time; // 游戏开始时间戳
    qint64 m_last_heart_spawn_time; // 上次生成生命道具的时间
    
    // 音频系统
#ifdef Qt6Multimedia_FOUND
    // 攻击音效（未装备武器）- 使用多个实例支持快速连续攻击
    QMediaPlayer* m_attack_sounds[3];
    // 攻击音效（装备武器）- 使用多个实例支持快速连续攻击
    QMediaPlayer* m_attack_weapon_sounds[3];
    QMediaPlayer* m_collect_wood_sound; // 采集树木音效
    QMediaPlayer* m_collect_stone_sound; // 采集石头音效
    QMediaPlayer* m_craft_sound; // 合成音效
    QMediaPlayer* m_death_sound; // 死亡音效
    QMediaPlayer* m_heal_sound; // 吃恢复道具音效
    QMediaPlayer* m_background_music; // 背景音乐
    int m_current_attack_index = 0; // 当前使用的攻击音效索引
    int m_current_attack_weapon_index = 0; // 当前使用的武器攻击音效索引
#endif

    // 生命道具
    struct Heart {
        int row;
        int col;
        bool exists;
    };
    QList<Heart> m_hearts; // 多个生命道具
    
    // 生命恢复特效
    struct HeartParticle {
        float x;      // 相对人物中心的X偏移
        float y;      // 相对人物中心的Y偏移
        float vx;     // X方向速度
        float vy;     // Y方向速度
        int lifetime; // 剩余存活时间
        float scale;  // 缩放大小
    };
    QList<HeartParticle> m_p1_heart_effects; // 玩家1的爱心特效
    QList<HeartParticle> m_p2_heart_effects; // 玩家2的爱心特效
    
    // 攻击特效
    struct AttackEffect {
        float x;      // 相对人物中心的X偏移
        float y;      // 相对人物中心的Y偏移
        int lifetime; // 剩余存活时间
        float angle;  // 旋转角度
        float scale;  // 缩放大小
    };
    QList<AttackEffect> m_p1_attack_effects; // 玩家1的攻击特效
    QList<AttackEffect> m_p2_attack_effects; // 玩家2的攻击特效
    
    // 游戏状态管理
    GameState m_current_state; // 当前游戏状态
    int m_menu_selection; // 菜单选择索引
    float m_bgm_volume; // 背景音乐音量
    float m_sfx_volume; // 音效音量
    qint64 m_game_over_show_time; // 游戏结束结果显示开始时间
    
    bool canMove(int row, int col);
    bool tryStartCollect(int pRow, int pCol, int& outR, int& outC, int& outType);
    void finishCollect(int player, int type);
    void attackPlayer(int attacker);
    bool craftStoneSword(int player);
    bool craftStoneShield(int player);
    void drawWeaponUI(QPainter& painter, int margin, int y_hp, int screenWidth);
    void updateAnimations();
    QPixmap getPlayerSprite(int player);
    void updateGameTimer();
    void checkGameEnd();
    void spawnHeart();
    bool isPathClear(int startRow, int startCol, int endRow, int endCol); // 检查路径是否畅通
    void collectHeart(int player);
    void spawnHeartEffect(int player);
    void updateHeartEffects();
    void spawnAttackEffect(int player);
    void updateAttackEffects();
    
    // 菜单相关方法
    void drawMainMenu(QPainter& painter);
    void drawModeSelect(QPainter& painter);
    void drawSettings(QPainter& painter);
    void drawGameRules(QPainter& painter);
    void drawGameOver(QPainter& painter);
    void handleMenuInput(QKeyEvent* event);
    void handleMenuSelection();
    
};

#endif // WIDGET_H