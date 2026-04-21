#include "widget.h"
#include <QPainter>
#include <QFont>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <ctime>
#include <QDateTime>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    // 设置窗口大小与屏幕相适应，稍小于屏幕
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();
    
    // 设置窗口大小为屏幕的90%
    int windowWidth = screenWidth * 0.9;
    int windowHeight = screenHeight * 0.9;
    
    setFixedSize(windowWidth, windowHeight);
    setWindowTitle("双人发育对战·武器与盾牌系统");
    setFocusPolicy(Qt::StrongFocus);

    // 初始化随机数种子
    srand(time(nullptr));

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Widget::onUpdateCollect);
    connect(m_timer, &QTimer::timeout, this, &Widget::updateAnimations);
    m_timer->start(30);
    
    // 初始化移动定时器，设置为240毫秒间隔，速度更慢更自然
    m_move_timer = new QTimer(this);
    connect(m_move_timer, &QTimer::timeout, this, &Widget::onUpdateMovement);
    m_move_timer->start(240);
    
    // 初始化游戏计时器
    m_game_time = GAME_TIME;
    m_game_over = false;
    m_start_time = QDateTime::currentMSecsSinceEpoch();
    m_last_heart_spawn_time = 0;

    // 初始化生命道具列表
    m_hearts.clear();

    // 初始化音频系统
#ifdef Qt6Multimedia_FOUND
    // 初始化多个攻击音效播放器以支持快速连续攻击
    for (int i = 0; i < 3; i++) {
        m_attack_sounds[i] = new QMediaPlayer(this);
        m_attack_sounds[i]->setSource(QUrl("qrc:/assets/audio/attack1.mp3"));
        QAudioOutput* output = new QAudioOutput(this);
        output->setVolume(0.7);
        m_attack_sounds[i]->setAudioOutput(output);

        m_attack_weapon_sounds[i] = new QMediaPlayer(this);
        m_attack_weapon_sounds[i]->setSource(QUrl("qrc:/assets/audio/attack2.mp3"));
        QAudioOutput* weapon_output = new QAudioOutput(this);
        weapon_output->setVolume(0.8);
        m_attack_weapon_sounds[i]->setAudioOutput(weapon_output);
    }
    m_collect_wood_sound = new QMediaPlayer(this);
    m_collect_wood_sound->setSource(QUrl("qrc:/assets/audio/collect1.mp3"));
    m_collect_stone_sound = new QMediaPlayer(this);
    m_collect_stone_sound->setSource(QUrl("qrc:/assets/audio/collect2.mp3"));
    m_craft_sound = new QMediaPlayer(this);
    m_craft_sound->setSource(QUrl("qrc:/assets/audio/craft.mp3"));
    m_death_sound = new QMediaPlayer(this);
    m_death_sound->setSource(QUrl("qrc:/assets/audio/death.mp3"));
    m_heal_sound = new QMediaPlayer(this);
    m_heal_sound->setSource(QUrl("qrc:/assets/audio/heal.mp3"));
    m_background_music = new QMediaPlayer(this);
    m_background_music->setSource(QUrl("qrc:/assets/audio/bgm.mp3"));

    // 设置音量
    QAudioOutput* collect_wood_output = new QAudioOutput(this);
    collect_wood_output->setVolume(0.5);
    m_collect_wood_sound->setAudioOutput(collect_wood_output);

    QAudioOutput* collect_stone_output = new QAudioOutput(this);
    collect_stone_output->setVolume(0.5);
    m_collect_stone_sound->setAudioOutput(collect_stone_output);

    QAudioOutput* craft_output = new QAudioOutput(this);
    craft_output->setVolume(0.6);
    m_craft_sound->setAudioOutput(craft_output);

    QAudioOutput* death_output = new QAudioOutput(this);
    death_output->setVolume(0.8);
    m_death_sound->setAudioOutput(death_output);

    QAudioOutput* heal_output = new QAudioOutput(this);
    heal_output->setVolume(0.6);
    m_heal_sound->setAudioOutput(heal_output);

    // 设置背景音乐音量（较小，不影响音效）
    QAudioOutput* background_output = new QAudioOutput(this);
    background_output->setVolume(0.2);
    m_background_music->setAudioOutput(background_output);
    // 设置背景音乐循环播放
    m_background_music->setLoops(QMediaPlayer::Infinite);
#endif

    // 初始化游戏状态
    m_current_state = STATE_MAIN_MENU;
    m_menu_selection = 0;
    m_bgm_volume = 0.2f; // 初始背景音乐音量
    m_sfx_volume = 0.7f; // 初始音效音量

    // 开始播放背景音乐
#ifdef Qt6Multimedia_FOUND
    m_background_music->play();
#endif

    // 初始化倒计时
    m_game_started = false;
    m_countdown = 3;
    m_countdown_start_time = QDateTime::currentMSecsSinceEpoch();

    // 连接计时器信号
    connect(m_timer, &QTimer::timeout, this, &Widget::updateGameTimer);
} 

Widget::~Widget()
{
#ifdef Qt6Multimedia_FOUND
    // 释放音频播放器资源
    for (int i = 0; i < 3; i++) {
        delete m_attack_sounds[i];
        delete m_attack_weapon_sounds[i];
    }
    delete m_collect_wood_sound;
    delete m_collect_stone_sound;
    delete m_craft_sound;
    delete m_death_sound;
    delete m_heal_sound;
    delete m_background_music;
#endif
}

bool Widget::canMove(int row, int col)
{
    // 检查边界
    if (row < 0 || row >= MAP_ROW || col < 0 || col >= MAP_COL)
        return false;
    // 只允许移动到空地(0)或河流(2)，不能在墙壁(1)、石头(4)、树木(3)上停留
    if (m_map[row][col] != 0 && m_map[row][col] != 2)
        return false;
    // 不能移动到另一个玩家的位置
    if ((row == m_p1_row && col == m_p1_col) || (row == m_p2_row && col == m_p2_col))
        return false;
    return true;
}

void Widget::attackPlayer(int attacker)
{
    // 检查攻击间隔
    const int ATTACK_COOLDOWN = 500; // 500ms攻击间隔
    qint64 current_time = QDateTime::currentMSecsSinceEpoch();
    
    // 播放攻击音效（使用多个播放器实例支持快速连续攻击，避免卡顿）
#ifdef Qt6Multimedia_FOUND
    if (attacker == 1 && m_p1_has_sword) {
        m_attack_weapon_sounds[m_current_attack_weapon_index]->stop();
        m_attack_weapon_sounds[m_current_attack_weapon_index]->play();
        m_current_attack_weapon_index = (m_current_attack_weapon_index + 1) % 3;
    } else if (attacker == 2 && m_p2_has_sword) {
        m_attack_weapon_sounds[m_current_attack_weapon_index]->stop();
        m_attack_weapon_sounds[m_current_attack_weapon_index]->play();
        m_current_attack_weapon_index = (m_current_attack_weapon_index + 1) % 3;
    } else {
        m_attack_sounds[m_current_attack_index]->stop();
        m_attack_sounds[m_current_attack_index]->play();
        m_current_attack_index = (m_current_attack_index + 1) % 3;
    }
#endif
    
    if (attacker == 1) {
        if (current_time - m_p1_last_attack_time < ATTACK_COOLDOWN) {
            return; // 攻击间隔未到，直接返回
        }
        m_p1_last_attack_time = current_time;
    } else if (attacker == 2) {
        if (current_time - m_p2_last_attack_time < ATTACK_COOLDOWN) {
            return; // 攻击间隔未到，直接返回
        }
        m_p2_last_attack_time = current_time;
    }
    
    int dx = abs(m_p1_col - m_p2_col);
    int dy = abs(m_p1_row - m_p2_row);
    
    // 确定攻击范围和伤害
    int attackRange = 1;
    int damage = 10;
    
    if (attacker == 1 && m_p1_has_sword) {
        attackRange = STONE_SWORD_RANGE;
        damage = STONE_SWORD_DAMAGE;
    } else if (attacker == 2 && m_p2_has_sword) {
        attackRange = STONE_SWORD_RANGE;
        damage = STONE_SWORD_DAMAGE;
    }
    
    // 无论是否有敌人，都设置攻击动画和特效
    if (attacker == 1) {
        m_p1_is_attacking = true;
        m_p1_animation_frame = 0;
        spawnAttackEffect(1);
    } else {
        m_p2_is_attacking = true;
        m_p2_animation_frame = 0;
        spawnAttackEffect(2);
    }
    
    if (dx <= attackRange && dy <= attackRange)
    {
        // 检查路径是否畅通（无树木和石头阻挡）
        if (!isPathClear(m_p1_row, m_p1_col, m_p2_row, m_p2_col)) {
            return; // 路径被阻挡，无法攻击
        }

        if (attacker == 1 && m_p2_hp > 0) {
            // 玩家2的防御
            int finalDamage = damage;
            if (m_p2_has_shield && m_p2_shield_hp > 0) {
                // 先扣除护盾耐久度
                m_p2_shield_hp -= damage;
                finalDamage = 0; // 护盾抵消全部伤害

                // 如果护盾耐久度小于等于0，护盾消失
                if (m_p2_shield_hp <= 0) {
                    m_p2_shield_hp = 0;
                    m_p2_has_shield = false;
                }
            }
            if (finalDamage > 0) {
                m_p2_hp = qMax(0, m_p2_hp - finalDamage);
                // 触发玩家2受击特效
                m_p2_is_hit = true;
                m_p2_hit_timer = 15; // 约450ms的闪烁时间
            }

            // 检查玩家2是否死亡
            if (m_p2_hp == 0) {
                m_p2_is_dead = true;
                m_p2_animation_frame = 0;
                m_p2_death_timer = 60; // 死亡动画持续2秒（60帧）
                m_p2_death_opacity = 1.0f;
                m_p2_death_finished = false;
                // 播放死亡音效
#ifdef Qt6Multimedia_FOUND
                m_death_sound->stop();
                m_death_sound->play();
#endif
                // 不立即结束游戏，等待死亡动画完成
            }
        }
        if (attacker == 2 && m_p1_hp > 0) {
            // 玩家1的防御
            int finalDamage = damage;
            if (m_p1_has_shield && m_p1_shield_hp > 0) {
                // 先扣除护盾耐久度
                m_p1_shield_hp -= damage;
                finalDamage = 0; // 护盾抵消全部伤害

                // 如果护盾耐久度小于等于0，护盾消失
                if (m_p1_shield_hp <= 0) {
                    m_p1_shield_hp = 0;
                    m_p1_has_shield = false;
                }
            }
            if (finalDamage > 0) {
                m_p1_hp = qMax(0, m_p1_hp - finalDamage);
                // 触发玩家1受击特效
                m_p1_is_hit = true;
                m_p1_hit_timer = 15; // 约450ms的闪烁时间
            }

            // 检查玩家1是否死亡
            if (m_p1_hp == 0) {
                m_p1_is_dead = true;
                m_p1_animation_frame = 0;
                m_p1_death_timer = 60; // 死亡动画持续2秒（60帧）
                m_p1_death_opacity = 1.0f;
                m_p1_death_finished = false;
                // 播放死亡音效
#ifdef Qt6Multimedia_FOUND
                m_death_sound->stop();
                m_death_sound->play();
#endif
                // 不立即结束游戏，等待死亡动画完成
            }
        }
        update();
    }
}

bool Widget::isPathClear(int startRow, int startCol, int endRow, int endCol)
{
    // 使用 Bresenham 直线算法检查两点之间的路径
    int dx = abs(endCol - startCol);
    int dy = abs(endRow - startRow);
    int sx = startCol < endCol ? 1 : -1;
    int sy = startRow < endRow ? 1 : -1;
    int err = dx - dy;
    
    int x = startCol;
    int y = startRow;
    
    // 跳过起点，从下一个点开始检查
    bool first = true;
    while (true) {
        // 跳过起点
        if (!first) {
            // 检查是否到达终点
            if (x == endCol && y == endRow) {
                break;
            }
            // 检查当前点是否有障碍物
            if (m_map[y][x] == 2 || m_map[y][x] == 3) {
                return false; // 有树木或石头阻挡
            }
        }
        first = false;
        
        // 继续算法
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
    
    return true; // 路径畅通
}

bool Widget::tryStartCollect(int pRow, int pCol, int& outR, int& outC, int& outType)
{
    int dr[4] = {-1,1,0,0};
    int dc[4] = {0,0,-1,1};
    for (int i=0; i<4; ++i)
    {
        int r = pRow + dr[i];
        int c = pCol + dc[i];
        if (r>=0&&r<MAP_ROW&&c>=0&&c<MAP_COL)
        {
            if (m_map[r][c]==3 || m_map[r][c]==4)
            {
                outR = r;
                outC = c;
                outType = m_map[r][c];
                return true;
            }
        }
    }
    return false;
}

void Widget::finishCollect(int player, int type)
{
    const int RESOURCE_LIMIT = 15;
    
    if (player == 1) {
        if (type == 3 && m_p1_wood < RESOURCE_LIMIT) m_p1_wood++;
        if (type == 4 && m_p1_stone < RESOURCE_LIMIT) m_p1_stone++;
    } else if (player == 2) {
        if (type == 3 && m_p2_wood < RESOURCE_LIMIT) m_p2_wood++;
        if (type == 4 && m_p2_stone < RESOURCE_LIMIT) m_p2_stone++;
    }
    
    // 播放采集音效
#ifdef Qt6Multimedia_FOUND
    if (type == 3) {
        m_collect_wood_sound->stop();
        m_collect_wood_sound->play();
    } else if (type == 4) {
        m_collect_stone_sound->stop();
        m_collect_stone_sound->play();
    }
#endif

    // 不再立即生成资源，改为定时生成
}

bool Widget::craftStoneSword(int player)
{
    if (player == 1) {
        if (m_p1_wood >= STONE_SWORD_COST_WOOD && m_p1_stone >= STONE_SWORD_COST_STONE) {
            m_p1_wood -= STONE_SWORD_COST_WOOD;
            m_p1_stone -= STONE_SWORD_COST_STONE;
            m_p1_has_sword = true;
            // 播放合成音效
#ifdef Qt6Multimedia_FOUND
            m_craft_sound->stop();
            m_craft_sound->play();
#endif
            return true;
        }
    } else if (player == 2) {
        if (m_p2_wood >= STONE_SWORD_COST_WOOD && m_p2_stone >= STONE_SWORD_COST_STONE) {
            m_p2_wood -= STONE_SWORD_COST_WOOD;
            m_p2_stone -= STONE_SWORD_COST_STONE;
            m_p2_has_sword = true;
            // 播放合成音效
#ifdef Qt6Multimedia_FOUND
            m_craft_sound->stop();
            m_craft_sound->play();
#endif
            return true;
        }
    }
    return false;
}

bool Widget::craftStoneShield(int player)
{
    const int SHIELD_MAX_HP = 30;
    
    if (player == 1) {
        if (m_p1_stone >= STONE_SHIELD_COST_STONE) {
            m_p1_stone -= STONE_SHIELD_COST_STONE;
            m_p1_has_shield = true;
            m_p1_shield_hp = SHIELD_MAX_HP;
            // 播放合成音效
#ifdef Qt6Multimedia_FOUND
            m_craft_sound->stop();
            m_craft_sound->play();
#endif
            return true;
        }
    } else if (player == 2) {
        if (m_p2_stone >= STONE_SHIELD_COST_STONE) {
            m_p2_stone -= STONE_SHIELD_COST_STONE;
            m_p2_has_shield = true;
            m_p2_shield_hp = SHIELD_MAX_HP;
            // 播放合成音效
#ifdef Qt6Multimedia_FOUND
            m_craft_sound->stop();
            m_craft_sound->play();
#endif
            return true;
        }
    }
    return false;
}

void Widget::drawWeaponUI(QPainter& painter, int margin, int y_hp, int screenWidth)
{
    // 绘制合成提示
    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei", 10));
    
    // 玩家1合成信息
    QString p1_craft_info = QString("P1 合成石头剑: %1木头 + %2石头")
                            .arg(STONE_SWORD_COST_WOOD)
                            .arg(STONE_SWORD_COST_STONE);
    painter.drawText(margin, y_hp + 70, p1_craft_info);
    
    // 玩家1盾牌合成信息
    QString p1_shield_info = QString("P1 合成石头盾牌: %1石头")
                            .arg(STONE_SHIELD_COST_STONE);
    painter.drawText(margin, y_hp + 90, p1_shield_info);
    
    // 玩家1武器状态
    if (m_p1_has_sword) {
        painter.setPen(Qt::green);
        painter.drawText(margin, y_hp + 110, "P1: 已装备石头剑");
    } else {
        painter.setPen(Qt::red);
        painter.drawText(margin, y_hp + 110, "P1: 未装备武器");
    }
    
    // 玩家1盾牌状态
    if (m_p1_has_shield) {
        painter.setPen(Qt::green);
        painter.drawText(margin, y_hp + 130, QString("P1: 石头盾牌 %1/30").arg(m_p1_shield_hp));
    } else {
        painter.setPen(Qt::red);
        painter.drawText(margin, y_hp + 130, "P1: 未装备盾牌");
    }
    
    // 玩家1按键提示
    painter.setPen(Qt::yellow);
    painter.drawText(margin, y_hp + 150, "P1: 按 Q 合成石头剑 | 按 R 合成石头盾牌");
    
    // 玩家2合成信息
    QString p2_craft_info = QString("P2 合成石头剑: %1木头 + %2石头")
                            .arg(STONE_SWORD_COST_WOOD)
                            .arg(STONE_SWORD_COST_STONE);
    painter.setPen(Qt::white);
    painter.drawText(screenWidth - margin - 260, y_hp + 70, p2_craft_info);
    
    // 玩家2盾牌合成信息
    QString p2_shield_info = QString("P2 合成石头盾牌: %1石头")
                            .arg(STONE_SHIELD_COST_STONE);
    painter.drawText(screenWidth - margin - 260, y_hp + 90, p2_shield_info);
    
    // 玩家2武器状态
    if (m_p2_has_sword) {
        painter.setPen(Qt::green);
        painter.drawText(screenWidth - margin - 260, y_hp + 110, "P2: 已装备石头剑");
    } else {
        painter.setPen(Qt::red);
        painter.drawText(screenWidth - margin - 260, y_hp + 110, "P2: 未装备武器");
    }
    
    // 玩家2盾牌状态
    if (m_p2_has_shield) {
        painter.setPen(Qt::green);
        painter.drawText(screenWidth - margin - 260, y_hp + 130, QString("P2: 石头盾牌 %1/30").arg(m_p2_shield_hp));
    } else {
        painter.setPen(Qt::red);
        painter.drawText(screenWidth - margin - 260, y_hp + 130, "P2: 未装备盾牌");
    }
    
    // 玩家2按键提示
    painter.setPen(Qt::yellow);
    painter.drawText(screenWidth - margin - 260, y_hp + 150, "P2: 按 M 合成石头剑 | 按 N 合成石头盾牌");
}

// 更新动画
void Widget::updateAnimations()
{
    // 玩家1受击特效更新
    if (m_p1_is_hit) {
        m_p1_hit_timer--;
        // 颤动效果：随机偏移
        m_p1_hit_offset_x = (rand() % 5) - 2; // -2到2的随机偏移
        m_p1_hit_offset_y = (rand() % 5) - 2;
        if (m_p1_hit_timer <= 0) {
            m_p1_is_hit = false;
            m_p1_hit_offset_x = 0;
            m_p1_hit_offset_y = 0;
        }
    }

    // 玩家2受击特效更新
    if (m_p2_is_hit) {
        m_p2_hit_timer--;
        // 颤动效果：随机偏移
        m_p2_hit_offset_x = (rand() % 5) - 2; // -2到2的随机偏移
        m_p2_hit_offset_y = (rand() % 5) - 2;
        if (m_p2_hit_timer <= 0) {
            m_p2_is_hit = false;
            m_p2_hit_offset_x = 0;
            m_p2_hit_offset_y = 0;
        }
    }

    // 玩家1死亡动画更新
    if (m_p1_is_dead && !m_p1_death_finished) {
        m_p1_death_timer--;
        // 逐渐降低透明度
        m_p1_death_opacity = m_p1_death_timer / 60.0f;
        if (m_p1_death_timer <= 0) {
            m_p1_death_finished = true;
            m_p1_death_opacity = 0.0f;
            // 死亡动画完成，检查游戏结束
            checkGameEnd();
        }
    }

    // 玩家2死亡动画更新
    if (m_p2_is_dead && !m_p2_death_finished) {
        m_p2_death_timer--;
        // 逐渐降低透明度
        m_p2_death_opacity = m_p2_death_timer / 60.0f;
        if (m_p2_death_timer <= 0) {
            m_p2_death_finished = true;
            m_p2_death_opacity = 0.0f;
            // 死亡动画完成，检查游戏结束
            checkGameEnd();
        }
    }

    // 玩家1动画更新
    if (m_p1_is_dead) {
        // 死亡动画
        m_p1_animation_timer++;
        if (m_p1_animation_timer >= 10) {
            m_p1_animation_timer = 0;
            m_p1_animation_frame++;
            if (m_p1_animation_frame >= 6) { // 死亡动画6帧
                m_p1_animation_frame = 5; // 停留在最后一帧
            }
        }
    } else if (m_p1_is_attacking) {
        // 攻击动画
        m_p1_animation_timer++;
        if (m_p1_animation_timer >= 5) {
            m_p1_animation_timer = 0;
            m_p1_animation_frame++;
            if (m_p1_animation_frame >= 8) { // 攻击动画8帧
                m_p1_animation_frame = 0;
                m_p1_is_attacking = false;
            }
        }
    } else if (m_p1_is_moving) {
        // 行走动画
        m_p1_animation_timer++;
        if (m_p1_animation_timer >= 8) {
            m_p1_animation_timer = 0;
            m_p1_animation_frame++;
            if (m_p1_animation_frame >= 8) { // 行走动画8帧
                m_p1_animation_frame = 0;
            }
        }
        // 移动跳动效果
        m_p1_jump_timer++;
        // 使用正弦函数创建平滑的跳动效果，幅度为2像素
        m_p1_jump_offset = (int)(2 * sin(m_p1_jump_timer * 0.3));
    } else {
        // 站立状态
        m_p1_animation_frame = 0;
        m_p1_jump_timer = 0;
        m_p1_jump_offset = 0;
    }
    
    // 玩家2动画更新
    if (m_p2_is_dead) {
        // 死亡动画
        m_p2_animation_timer++;
        if (m_p2_animation_timer >= 10) {
            m_p2_animation_timer = 0;
            m_p2_animation_frame++;
            if (m_p2_animation_frame >= 6) { // 死亡动画6帧
                m_p2_animation_frame = 5; // 停留在最后一帧
            }
        }
    } else if (m_p2_is_attacking) {
        // 攻击动画
        m_p2_animation_timer++;
        if (m_p2_animation_timer >= 5) {
            m_p2_animation_timer = 0;
            m_p2_animation_frame++;
            if (m_p2_animation_frame >= 8) { // 攻击动画8帧
                m_p2_animation_frame = 0;
                m_p2_is_attacking = false;
            }
        }
    } else if (m_p2_is_moving) {
        // 行走动画
        m_p2_animation_timer++;
        if (m_p2_animation_timer >= 8) {
            m_p2_animation_timer = 0;
            m_p2_animation_frame++;
            if (m_p2_animation_frame >= 8) { // 行走动画8帧
                m_p2_animation_frame = 0;
            }
        }
        // 移动跳动效果
        m_p2_jump_timer++;
        // 使用正弦函数创建平滑的跳动效果，幅度为2像素
        m_p2_jump_offset = (int)(2 * sin(m_p2_jump_timer * 0.3));
    } else {
        // 站立状态
        m_p2_animation_frame = 0;
        m_p2_jump_timer = 0;
        m_p2_jump_offset = 0;
    }
}

// 获取玩家精灵
QPixmap Widget::getPlayerSprite(int player)
{
    QPixmap sprite;
    if (player == 1) {
        if (m_p1_is_dead) {
            // 死亡动画
            sprite = QPixmap(QString(":/assets/images/death_%1.png").arg(m_p1_animation_frame + 1));
        } else if (m_p1_is_attacking) {
            // 攻击动画
            sprite = QPixmap(QString(":/assets/images/attack_%1.png").arg(m_p1_animation_frame + 1));
        } else if (m_p1_is_moving) {
            // 行走动画
            sprite = QPixmap(QString(":/assets/images/walk_%1.png").arg(m_p1_animation_frame + 1));
        } else {
            // 站立
            sprite = QPixmap(":/assets/images/player1.png");
        }
    } else {
        if (m_p2_is_dead) {
            // 死亡动画
            sprite = QPixmap(QString(":/assets/images/player2_death_%1.png").arg(m_p2_animation_frame + 1));
        } else if (m_p2_is_attacking) {
            // 攻击动画
            sprite = QPixmap(QString(":/assets/images/player2_attack_%1.png").arg(m_p2_animation_frame + 1));
        } else if (m_p2_is_moving) {
            // 行走动画
            sprite = QPixmap(QString(":/assets/images/player2_walk_%1.png").arg(m_p2_animation_frame + 1));
        } else {
            // 站立
            sprite = QPixmap(":/assets/images/player2.png");
        }
    }

    // 如果加载失败，返回默认玩家图片
    if (sprite.isNull()) {
        sprite = QPixmap(player == 1 ? ":/assets/images/player1.png" : ":/assets/images/player2.png");
    }

    return sprite;
}

// 更新游戏计时器
void Widget::updateGameTimer()
{
    // 处理倒计时
    if (!m_game_started) {
        qint64 current_time = QDateTime::currentMSecsSinceEpoch();
        int elapsed = (current_time - m_countdown_start_time) / 1000;
        m_countdown = 3 - elapsed;

        if (m_countdown <= 0) {
            m_countdown = 0;
            m_game_started = true;
            // 游戏开始时重置游戏时间
            m_start_time = QDateTime::currentMSecsSinceEpoch();
#ifdef Qt6Multimedia_FOUND
            // 开始播放背景音乐
            m_background_music->play();
#endif
        }
        update(); // 触发重绘以更新倒计时显示
        return;
    }

    if (!m_game_over) {
        // 使用时间戳计算剩余时间，避免累积误差
        qint64 current_time = QDateTime::currentMSecsSinceEpoch();
        int elapsed_seconds = (current_time - m_start_time) / 1000;
        m_game_time = GAME_TIME - elapsed_seconds;
        
        if (m_game_time <= 0) {
            m_game_time = 0;
            m_game_over = true;
            checkGameEnd();
        }
        
        // 每20秒在河流上生成一个生命道具
        qint64 heart_interval = 20000; // 20秒
        if (current_time - m_last_heart_spawn_time >= heart_interval) {
            spawnHeart();
            m_last_heart_spawn_time = current_time;
        }
        
        updateHeartEffects();
        updateAttackEffects();
        
        update();
    }
}

// 检查游戏结束条件
void Widget::checkGameEnd()
{
    if (m_p1_hp <= 0 || m_p2_hp <= 0 || m_game_time <= 0) {
        m_game_over = true;
        // 切换到游戏结束状态
        m_current_state = STATE_GAME_OVER;
        m_menu_selection = 0;
        update();
    }
}

// 在河流上随机生成生命道具
void Widget::spawnHeart()
{
    QList<QPair<int, int>> river_positions;
    for (int r = 0; r < MAP_ROW; r++) {
        for (int c = 0; c < MAP_COL; c++) {
            if (m_map[r][c] == 2) {
                bool has_heart = false;
                // 使用传统 for 循环避免 Qt 容器分离警告
                for (int i = 0; i < m_hearts.size(); i++) {
                    const Heart& heart = m_hearts[i];
                    if (heart.exists && heart.row == r && heart.col == c) {
                        has_heart = true;
                        break;
                    }
                }
                if (!has_heart) {
                    river_positions.append(QPair<int, int>(r, c));
                }
            }
        }
    }
    
    if (!river_positions.isEmpty()) {
        int index = rand() % river_positions.size();
        Heart new_heart;
        new_heart.row = river_positions[index].first;
        new_heart.col = river_positions[index].second;
        new_heart.exists = true;
        m_hearts.append(new_heart);
        update();
    }
}

// 玩家收集生命道具
void Widget::collectHeart(int player)
{
    const int HEART_HEAL = 25;
    
    if (player == 1) {
        if (m_p1_hp < m_p1_max_hp) {
            m_p1_hp += HEART_HEAL;
            if (m_p1_hp > m_p1_max_hp) {
                m_p1_hp = m_p1_max_hp;
            }
        } else {
            m_p1_max_hp += HEART_HEAL;
        }
    } else if (player == 2) {
        if (m_p2_hp < m_p2_max_hp) {
            m_p2_hp += HEART_HEAL;
            if (m_p2_hp > m_p2_max_hp) {
                m_p2_hp = m_p2_max_hp;
            }
        } else {
            m_p2_max_hp += HEART_HEAL;
        }
    }
    
    // 播放吃恢复道具音效
#ifdef Qt6Multimedia_FOUND
    m_heal_sound->stop();
    m_heal_sound->play();
#endif
    
    int player_row = (player == 1) ? m_p1_row : m_p2_row;
    int player_col = (player == 1) ? m_p1_col : m_p2_col;
    
    for (int i = 0; i < m_hearts.size(); i++) {
        if (m_hearts[i].exists && m_hearts[i].row == player_row && m_hearts[i].col == player_col) {
            m_hearts[i].exists = false;
            m_hearts.removeAt(i);
            break;
        }
    }
    
    spawnHeartEffect(player);
    update();
}

void Widget::spawnHeartEffect(int player)
{
    QList<HeartParticle>& effects = (player == 1) ? m_p1_heart_effects : m_p2_heart_effects;
    effects.clear();
    
    for (int i = 0; i < 8; i++) {
        HeartParticle p;
        p.x = (float)(rand() % 40 - 20);
        p.y = (float)(rand() % 40 - 20);
        p.vx = (float)(rand() % 20 - 10) / 10.0f;
        p.vy = -(float)(rand() % 15 + 5) / 10.0f;
        p.lifetime = 30 + rand() % 20;
        p.scale = 0.5f + (float)(rand() % 50) / 100.0f;
        effects.append(p);
    }
}

void Widget::updateHeartEffects()
{
    for (int i = 0; i < m_p1_heart_effects.size(); i++) {
        HeartParticle& p = m_p1_heart_effects[i];
        p.x += p.vx;
        p.y += p.vy;
        p.vy += 0.05f;
        p.lifetime--;
        p.scale *= 0.98f;
    }
    for (int i = m_p1_heart_effects.size() - 1; i >= 0; i--) {
        if (m_p1_heart_effects[i].lifetime <= 0) {
            m_p1_heart_effects.removeAt(i);
        }
    }
    
    for (int i = 0; i < m_p2_heart_effects.size(); i++) {
        HeartParticle& p = m_p2_heart_effects[i];
        p.x += p.vx;
        p.y += p.vy;
        p.vy += 0.05f;
        p.lifetime--;
        p.scale *= 0.98f;
    }
    for (int i = m_p2_heart_effects.size() - 1; i >= 0; i--) {
        if (m_p2_heart_effects[i].lifetime <= 0) {
            m_p2_heart_effects.removeAt(i);
        }
    }
}

void Widget::spawnAttackEffect(int player)
{
    QList<AttackEffect>& effects = (player == 1) ? m_p1_attack_effects : m_p2_attack_effects;
    effects.clear();
    
    // 检查玩家是否拥有武器
    bool hasWeapon = (player == 1) ? m_p1_has_sword : m_p2_has_sword;
    
    // 生成两段弯刀，一端在人物，另一端绕着人物转
    // 第一段弯刀 - 起点在人物，绕着人物旋转
    AttackEffect e1;
    e1.x = 0;  // 起点在人物中心
    e1.y = 0;
    e1.lifetime = 15;
    e1.angle = 0.0f; // 初始角度
    e1.scale = hasWeapon ? 1.3f : 1.0f; // 有武器时特效更大
    effects.append(e1);
    
    // 第二段弯刀 - 与第一段成角度
    AttackEffect e2;
    e2.x = 0;  // 起点在人物中心
    e2.y = 0;
    e2.lifetime = 15;
    e2.angle = 180.0f; // 与第一段相差180度
    e2.scale = hasWeapon ? 1.3f : 1.0f; // 有武器时特效更大
    effects.append(e2);
}

void Widget::updateAttackEffects()
{
    for (int i = 0; i < m_p1_attack_effects.size(); i++) {
        AttackEffect& e = m_p1_attack_effects[i];
        e.lifetime--;
        e.angle += 15.0f;
        e.scale *= 0.95f;
    }
    for (int i = m_p1_attack_effects.size() - 1; i >= 0; i--) {
        if (m_p1_attack_effects[i].lifetime <= 0) {
            m_p1_attack_effects.removeAt(i);
        }
    }
    
    for (int i = 0; i < m_p2_attack_effects.size(); i++) {
        AttackEffect& e = m_p2_attack_effects[i];
        e.lifetime--;
        e.angle += 15.0f;
        e.scale *= 0.95f;
    }
    for (int i = m_p2_attack_effects.size() - 1; i >= 0; i--) {
        if (m_p2_attack_effects[i].lifetime <= 0) {
            m_p2_attack_effects.removeAt(i);
        }
    }
}

// 处理持续移动逻辑
void Widget::onUpdateMovement()
{
    if (m_game_over) return;
    
    bool needUpdate = false;
    
    // 玩家1移动处理 - 只有在不采集时才能移动
    if ((m_p1_move_up || m_p1_move_down || m_p1_move_left || m_p1_move_right) && !m_p1_collecting) {
        m_p1_is_moving = true;
        int newRow = m_p1_row;
        int newCol = m_p1_col;
        
        // 计算移动方向
        if (m_p1_move_up) newRow--;
        if (m_p1_move_down) newRow++;
        if (m_p1_move_left) newCol--;
        if (m_p1_move_right) newCol++;
        
        // 检查移动是否合法
        if (canMove(newRow, newCol)) {
            // 检查是否在河流中，如果在河流中，只有50%的概率移动（速度减半）
            bool inWater = (m_map[m_p1_row][m_p1_col] == 2) || (m_map[newRow][newCol] == 2);
            if (!inWater || (rand() % 2 == 0)) {
                m_p1_row = newRow;
                m_p1_col = newCol;
                needUpdate = true;
            }
        }
    } else {
        m_p1_is_moving = false;
    }
    
    // 玩家2移动处理 - 只有在不采集时才能移动
    if ((m_p2_move_up || m_p2_move_down || m_p2_move_left || m_p2_move_right) && !m_p2_collecting) {
        m_p2_is_moving = true;
        int newRow = m_p2_row;
        int newCol = m_p2_col;
        
        // 计算移动方向
        if (m_p2_move_up) newRow--;
        if (m_p2_move_down) newRow++;
        if (m_p2_move_left) newCol--;
        if (m_p2_move_right) newCol++;
        
        // 检查移动是否合法
        if (canMove(newRow, newCol)) {
            // 检查是否在河流中，如果在河流中，只有50%的概率移动（速度减半）
            bool inWater = (m_map[m_p2_row][m_p2_col] == 2) || (m_map[newRow][newCol] == 2);
            if (!inWater || (rand() % 2 == 0)) {
                m_p2_row = newRow;
                m_p2_col = newCol;
                needUpdate = true;
            }
        }
    } else {
        m_p2_is_moving = false;
    }
    
    // 检查玩家是否收集生命道具
    for (int i = 0; i < m_hearts.size(); i++) {
        if (m_hearts[i].exists) {
            if (m_p1_row == m_hearts[i].row && m_p1_col == m_hearts[i].col) {
                collectHeart(1);
                break;
            } else if (m_p2_row == m_hearts[i].row && m_p2_col == m_hearts[i].col) {
                collectHeart(2);
                break;
            }
        }
    }
    
    if (needUpdate) {
        update();
    }
}

// 统一更新采集进度（双人完全独立）
void Widget::onUpdateCollect()
{
    if (m_game_over) return;
    
    bool needUpdate = false;

    // 玩家1采集逻辑（不受P2任何影响）
    if (m_p1_collecting)
    {
        m_p1_collect_prog += 30;
        needUpdate = true;
        if (m_p1_collect_prog >= COLLECT_TIME)
        {
            finishCollect(1, m_p1_collect_type);
            m_map[m_p1_collect_r][m_p1_collect_c] = 0;
            m_p1_collecting = false;
            m_p1_collect_prog = 0;
        }
    }

    // 玩家2采集逻辑（不受P1任何影响）
    if (m_p2_collecting)
    {
        m_p2_collect_prog += 30;
        needUpdate = true;
        if (m_p2_collect_prog >= COLLECT_TIME)
        {
            finishCollect(2, m_p2_collect_type);
            m_map[m_p2_collect_r][m_p2_collect_c] = 0;
            m_p2_collecting = false;
            m_p2_collect_prog = 0;
        }
    }

    if (needUpdate)
        update();
}

// ====================== 核心修复：按键全独立if判断，无else互斥 ======================
void Widget::keyPressEvent(QKeyEvent *event)
{
    // 处理Esc键退出全屏
    if (event->key() == Qt::Key_Escape) {
        showNormal(); // 退出全屏模式
        return;
    }

    // 忽略自动重复事件，防止按键重复触发导致攻击间隔混乱
    if (event->isAutoRepeat()) {
        QWidget::keyPressEvent(event);
        return;
    }

    // 菜单状态下处理菜单输入
    if (m_current_state != STATE_GAME_PLAY) {
        handleMenuInput(event);
        return;
    }

    // 游戏未开始，忽略所有输入
    if (!m_game_started)
        return;

    if (m_p1_hp <= 0 || m_p2_hp <= 0 || m_game_over)
        return;

    // ---------- 玩家1 独立按键（WASD/空格/E/Q） ----------
    if (event->key() == Qt::Key_W) {
        m_p1_move_up = true;
    }
    if (event->key() == Qt::Key_S) {
        m_p1_move_down = true;
    }
    if (event->key() == Qt::Key_A) {
        m_p1_move_left = true;
    }
    if (event->key() == Qt::Key_D) {
        m_p1_move_right = true;
    }
    if (event->key() == Qt::Key_Space) {
        attackPlayer(1);
    }
    if (event->key() == Qt::Key_E && !m_p1_collecting) {
        if (tryStartCollect(m_p1_row, m_p1_col, m_p1_collect_r, m_p1_collect_c, m_p1_collect_type)) {
            m_p1_collecting = true;
            m_p1_collect_prog = 0;
        }
    }
    if (event->key() == Qt::Key_Q) {
        craftStoneSword(1);
        update();
    }
    if (event->key() == Qt::Key_R) {
        craftStoneShield(1);
        update();
    }

    // ---------- 玩家2 独立按键（方向键/回车/Ctrl/M） ----------
    if (event->key() == Qt::Key_Up) {
        m_p2_move_up = true;
    }
    if (event->key() == Qt::Key_Down) {
        m_p2_move_down = true;
    }
    if (event->key() == Qt::Key_Left) {
        m_p2_move_left = true;
    }
    if (event->key() == Qt::Key_Right) {
        m_p2_move_right = true;
    }
    if (event->key() == Qt::Key_Return) {
        attackPlayer(2);
    }
    if (event->key() == Qt::Key_Control && !m_p2_collecting) {
        if (tryStartCollect(m_p2_row, m_p2_col, m_p2_collect_r, m_p2_collect_c, m_p2_collect_type)) {
            m_p2_collecting = true;
            m_p2_collect_prog = 0;
        }
    }
    if (event->key() == Qt::Key_M) {
        craftStoneSword(2);
        update();
    }
    if (event->key() == Qt::Key_N) {
        craftStoneShield(2);
        update();
    }
}

// ====================== 核心修复：松开只停止对应玩家采集 ======================
void Widget::keyReleaseEvent(QKeyEvent *event)
{
    // 忽略自动重复事件，防止按键重复触发导致采集中断
    if (event->isAutoRepeat()) {
        QWidget::keyReleaseEvent(event);
        return;
    }

    // 只停止玩家1采集
    if (event->key() == Qt::Key_E) {
        m_p1_collecting = false;
        m_p1_collect_prog = 0;
        update();
    }
    // 玩家1移动按键松开
    if (event->key() == Qt::Key_W) {
        m_p1_move_up = false;
    }
    if (event->key() == Qt::Key_S) {
        m_p1_move_down = false;
    }
    if (event->key() == Qt::Key_A) {
        m_p1_move_left = false;
    }
    if (event->key() == Qt::Key_D) {
        m_p1_move_right = false;
    }

    // 只停止玩家2采集
    if (event->key() == Qt::Key_Control) {
        m_p2_collecting = false;
        m_p2_collect_prog = 0;
        update();
    }
    // 玩家2移动按键松开
    if (event->key() == Qt::Key_Up) {
        m_p2_move_up = false;
    }
    if (event->key() == Qt::Key_Down) {
        m_p2_move_down = false;
    }
    if (event->key() == Qt::Key_Left) {
        m_p2_move_left = false;
    }
    if (event->key() == Qt::Key_Right) {
        m_p2_move_right = false;
    }

    QWidget::keyReleaseEvent(event);
}

// 处理鼠标点击选择菜单
void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    
    int mouseY = event->pos().y();
    int itemHeight = 40;
    
    switch (m_current_state) {
    case STATE_MAIN_MENU: {
        QString menuItems[] = {"开始游戏", "模式选择", "设置", "退出游戏"};
        int itemCount = 4;
        int startY = height() / 2 - 60;
        
        for (int i = 0; i < itemCount; i++) {
            int y = startY + i * itemHeight;
            if (mouseY >= y && mouseY < y + itemHeight) {
                m_menu_selection = i;
                handleMenuSelection();
                break;
            }
        }
        break;
    }
    case STATE_MODE_SELECT: {
        QString menuItems[] = {"双人对战", "人机对战", "返回菜单"};
        int itemCount = 3;
        int startY = height() / 2 - 45;
        
        for (int i = 0; i < itemCount; i++) {
            int y = startY + i * itemHeight;
            if (mouseY >= y && mouseY < y + itemHeight) {
                m_menu_selection = i;
                handleMenuSelection();
                break;
            }
        }
        break;
    }
    case STATE_SETTINGS: {
        QString menuItems[] = {"背景音乐", "音效音量", "返回菜单"};
        int itemCount = 3;
        int startY = height() / 2 - 45;
        
        for (int i = 0; i < itemCount; i++) {
            int y = startY + i * itemHeight;
            if (mouseY >= y && mouseY < y + itemHeight) {
                m_menu_selection = i;
                handleMenuSelection();
                break;
            }
        }
        break;
    }
    case STATE_GAME_OVER: {
        QString menuItems[] = {"再来一局", "回到主菜单", "退出游戏"};
        int itemCount = 3;
        int startY = height() / 2 - 200 + 150;
        
        for (int i = 0; i < itemCount; i++) {
            int y = startY + i * itemHeight;
            if (mouseY >= y && mouseY < y + itemHeight) {
                m_menu_selection = i;
                handleMenuSelection();
                break;
            }
        }
        break;
    }
    default:
        break;
    }
    
    update();
}

// 绘图（双人独立进度条）
void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    
    // 根据当前游戏状态绘制不同界面
    switch (m_current_state) {
    case STATE_MAIN_MENU:
        drawMainMenu(painter);
        return;
    case STATE_MODE_SELECT:
        drawModeSelect(painter);
        return;
    case STATE_SETTINGS:
        drawSettings(painter);
        return;
    case STATE_GAME_RULES:
        drawGameRules(painter);
        return;
    case STATE_GAME_OVER:
        drawGameOver(painter);
        return;
    case STATE_GAME_PLAY:
        // 继续绘制游戏界面
        break;
    }
    
    // 游戏中界面绘制

    // 加载图片资源
    QPixmap pixEmpty1(":/assets/images/empty1.png");
    QPixmap pixEmpty2(":/assets/images/empty2.png");
    QPixmap pixEmpty3(":/assets/images/empty3.png");
    QPixmap pixEmpty4(":/assets/images/empty4.png");
    QPixmap pixEmpty5(":/assets/images/empty5.png");
    QPixmap pixRiver(":/assets/images/river.png");
    QPixmap pixStone(":/assets/images/stone.png");
    QPixmap pixWall(":/assets/images/wall.png");
    QPixmap pixTree(":/assets/images/tree.png");
    QPixmap pixP1(":/assets/images/player1.png");
    QPixmap pixP2(":/assets/images/player2.png");

    // 缩放图片
    pixEmpty1 = pixEmpty1.scaled(TILE_SIZE, TILE_SIZE);
    pixEmpty2 = pixEmpty2.scaled(TILE_SIZE, TILE_SIZE);
    pixEmpty3 = pixEmpty3.scaled(TILE_SIZE, TILE_SIZE);
    pixEmpty4 = pixEmpty4.scaled(TILE_SIZE, TILE_SIZE);
    pixEmpty5 = pixEmpty5.scaled(TILE_SIZE, TILE_SIZE);
    pixRiver = pixRiver.scaled(TILE_SIZE, TILE_SIZE);
    pixStone = pixStone.scaled(TILE_SIZE, TILE_SIZE);
    pixWall = pixWall.scaled(TILE_SIZE, TILE_SIZE);
    pixTree = pixTree.scaled(TILE_SIZE, TILE_SIZE);
    pixP1 = pixP1.scaled(TILE_SIZE, TILE_SIZE);
    pixP2 = pixP2.scaled(TILE_SIZE, TILE_SIZE);

    // 计算地图绘制偏移量，在四周留出边距
    int margin = 50; // 边距大小
    int mapWidth = MAP_COL * TILE_SIZE;
    int mapHeight = MAP_ROW * TILE_SIZE;
    int screenWidth = width();
    int screenHeight = height();
    
    // 计算地图位置，让地图向上移动，给下方的人物属性留出空间
    int offsetX = (screenWidth - mapWidth) / 2;
    // 让地图再向上移动一些，给下方的人物属性留出足够空间
    int offsetY = (screenHeight - mapHeight - 200) / 2; // 减去200像素给人物属性
    
    // 确保边距足够
    offsetX = qMax(margin, offsetX);
    offsetY = qMax(margin, offsetY);

    // 画地图
    for (int r=0; r<MAP_ROW; r++)
        for (int c=0; c<MAP_COL; c++) {
            int x = offsetX + c*TILE_SIZE, y = offsetY + r*TILE_SIZE;
            switch(m_map[r][c]){
            case 0: {
                // 统一使用empty2草地纹理
                painter.drawPixmap(x,y,pixEmpty2);
                break;
            }
            case 1:painter.drawPixmap(x,y,pixWall);break;
            case 2:painter.drawPixmap(x,y,pixRiver);break;
            case 3:painter.drawPixmap(x,y,pixTree);break;
            case 4:painter.drawPixmap(x,y,pixStone);break;
            }
        }

    // 绘制所有生命道具
    QPixmap heart_pixmap(":/assets/images/heart.png");
    heart_pixmap = heart_pixmap.scaled(TILE_SIZE, TILE_SIZE);
    for (int i = 0; i < m_hearts.size(); i++) {
        if (m_hearts[i].exists) {
            painter.drawPixmap(offsetX + m_hearts[i].col*TILE_SIZE, offsetY + m_hearts[i].row*TILE_SIZE, heart_pixmap);
        }
    }

    // 画玩家 - 使用getPlayerSprite获取动画精灵
    QPixmap newPixP1 = getPlayerSprite(1);
    QPixmap newPixP2 = getPlayerSprite(2);

    // 确保图片有效
    if (!newPixP1.isNull()) {
        newPixP1 = newPixP1.scaled(TILE_SIZE, TILE_SIZE);
    }
    if (!newPixP2.isNull()) {
        newPixP2 = newPixP2.scaled(TILE_SIZE, TILE_SIZE);
    }

    // 如果图片无效，使用默认玩家图片
    if (newPixP1.isNull()) {
        newPixP1 = QPixmap(":/assets/images/player1.png").scaled(TILE_SIZE, TILE_SIZE);
    }
    if (newPixP2.isNull()) {
        newPixP2 = QPixmap(":/assets/images/player2.png").scaled(TILE_SIZE, TILE_SIZE);
    }

    // 绘制玩家1（带受击颤动效果和移动跳动）
    int p1_draw_x = offsetX + m_p1_col*TILE_SIZE + m_p1_hit_offset_x;
    int p1_draw_y = offsetY + m_p1_row*TILE_SIZE + m_p1_hit_offset_y + m_p1_jump_offset;
    if (m_p1_is_dead && !m_p1_death_finished) {
        // 死亡动画：逐渐消失
        painter.setOpacity(m_p1_death_opacity);
        painter.drawPixmap(p1_draw_x, p1_draw_y, newPixP1);
        painter.setOpacity(1.0);
    } else if (!m_p1_death_finished) { // 死亡动画完成后不绘制
        painter.drawPixmap(p1_draw_x, p1_draw_y, newPixP1);
    }

    // 绘制玩家2（带受击颤动效果和移动跳动）
    int p2_draw_x = offsetX + m_p2_col*TILE_SIZE + m_p2_hit_offset_x;
    int p2_draw_y = offsetY + m_p2_row*TILE_SIZE + m_p2_hit_offset_y + m_p2_jump_offset;
    if (m_p2_is_dead && !m_p2_death_finished) {
        // 死亡动画：逐渐消失
        painter.setOpacity(m_p2_death_opacity);
        painter.drawPixmap(p2_draw_x, p2_draw_y, newPixP2);
        painter.setOpacity(1.0);
    } else if (!m_p2_death_finished) { // 死亡动画完成后不绘制
        painter.drawPixmap(p2_draw_x, p2_draw_y, newPixP2);
    }

    // 绘制玩家1受击红色叠加效果
    if (m_p1_is_hit) {
        // 直接绘制半透明红色矩形覆盖角色
        painter.save();
        painter.setOpacity(0.5);
        painter.setBrush(QColor(255, 0, 0, 128)); // 半透明红色
        painter.setPen(Qt::NoPen);
        painter.drawRect(p1_draw_x, p1_draw_y, TILE_SIZE, TILE_SIZE);
        painter.restore();
    }

    // 绘制玩家2受击红色叠加效果
    if (m_p2_is_hit) {
        // 直接绘制半透明红色矩形覆盖角色
        painter.save();
        painter.setOpacity(0.5);
        painter.setBrush(QColor(255, 0, 0, 128)); // 半透明红色
        painter.setPen(Qt::NoPen);
        painter.drawRect(p2_draw_x, p2_draw_y, TILE_SIZE, TILE_SIZE);
        painter.restore();
    }

    // 绘制玩家1护盾特效
    if (m_p1_has_shield && !m_p1_is_dead) {
        int px = offsetX + m_p1_col * TILE_SIZE + TILE_SIZE/2 + m_p1_hit_offset_x;
        int py = offsetY + m_p1_row * TILE_SIZE + TILE_SIZE/2 + m_p1_hit_offset_y + m_p1_jump_offset;
        int shieldRadius = TILE_SIZE/2 + 4; // 护盾半径，略大于角色

        // 保存当前状态
        painter.save();
        painter.translate(px, py);

        // 绘制淡蓝色半透明光罩
        QPen pen(QColor(120, 180, 255, 180), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        // 绘制圆形护盾
        painter.drawEllipse(-shieldRadius, -shieldRadius, shieldRadius*2, shieldRadius*2);

        // 绘制边缘微弱光晕（内圈）
        QPen glowPen(QColor(150, 200, 255, 120), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(glowPen);
        painter.drawEllipse(-shieldRadius + 2, -shieldRadius + 2, (shieldRadius-2)*2, (shieldRadius-2)*2);

        // 恢复状态
        painter.restore();
    }

    // 绘制玩家2护盾特效
    if (m_p2_has_shield && !m_p2_is_dead) {
        int px = offsetX + m_p2_col * TILE_SIZE + TILE_SIZE/2 + m_p2_hit_offset_x;
        int py = offsetY + m_p2_row * TILE_SIZE + TILE_SIZE/2 + m_p2_hit_offset_y + m_p2_jump_offset;
        int shieldRadius = TILE_SIZE/2 + 4; // 护盾半径，略大于角色

        // 保存当前状态
        painter.save();
        painter.translate(px, py);

        // 绘制淡蓝色半透明光罩
        QPen pen(QColor(120, 180, 255, 180), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        // 绘制圆形护盾
        painter.drawEllipse(-shieldRadius, -shieldRadius, shieldRadius*2, shieldRadius*2);

        // 绘制边缘微弱光晕（内圈）
        QPen glowPen(QColor(150, 200, 255, 120), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(glowPen);
        painter.drawEllipse(-shieldRadius + 2, -shieldRadius + 2, (shieldRadius-2)*2, (shieldRadius-2)*2);

        // 恢复状态
        painter.restore();
    }

    // 绘制玩家1爱心特效
    if (!m_p1_heart_effects.isEmpty() && !m_p1_is_dead) {
        QPixmap heart_small = heart_pixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        for (const HeartParticle& p : m_p1_heart_effects) {
            int px = offsetX + m_p1_col * TILE_SIZE + TILE_SIZE/2 + (int)p.x + m_p1_hit_offset_x;
            int py = offsetY + m_p1_row * TILE_SIZE + TILE_SIZE/2 + (int)p.y + m_p1_hit_offset_y + m_p1_jump_offset;
            int size = (int)(16 * p.scale);
            painter.drawPixmap(px - size/2, py - size/2, heart_small.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    // 绘制玩家2爱心特效
    if (!m_p2_heart_effects.isEmpty() && !m_p2_is_dead) {
        QPixmap heart_small = heart_pixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        for (const HeartParticle& p : m_p2_heart_effects) {
            int px = offsetX + m_p2_col * TILE_SIZE + TILE_SIZE/2 + (int)p.x + m_p2_hit_offset_x;
            int py = offsetY + m_p2_row * TILE_SIZE + TILE_SIZE/2 + (int)p.y + m_p2_hit_offset_y + m_p2_jump_offset;
            int size = (int)(16 * p.scale);
            painter.drawPixmap(px - size/2, py - size/2, heart_small.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    // 绘制玩家1攻击特效
    if (!m_p1_attack_effects.isEmpty() && !m_p1_is_dead) {
        for (const AttackEffect& e : m_p1_attack_effects) {
            int px = offsetX + m_p1_col * TILE_SIZE + TILE_SIZE/2 + (int)e.x + m_p1_hit_offset_x;
            int py = offsetY + m_p1_row * TILE_SIZE + TILE_SIZE/2 + (int)e.y + m_p1_hit_offset_y + m_p1_jump_offset;
            int baseBladeLength = m_p1_has_sword ? 50 : 35; // 有武器时弯刀更长
            int bladeLength = (int)(baseBladeLength * e.scale);
            
            // 计算透明度（根据生命周期）
            int alpha = 255 * e.lifetime / 15;
            if (alpha > 255) alpha = 255;
            if (alpha < 0) alpha = 0;
            
            // 保存当前状态
            painter.save();
            
            // 移动到特效位置
            painter.translate(px, py);
            
            // 旋转
            painter.rotate(e.angle);
            
            // 绘制灰色弯刀 - 靠近人物一端加粗
            // 外层淡色（较细）
            QPen penOuter(QColor(100, 100, 100, alpha), 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
            painter.setPen(penOuter);
            painter.setBrush(Qt::NoBrush);
            
            QPainterPath pathOuter;
            pathOuter.moveTo(0, 0);
            pathOuter.quadTo(bladeLength/2, -6, bladeLength, 0);
            painter.drawPath(pathOuter);
            
            // 中层（正常粗细）
            QPen penMid(QColor(130, 130, 130, alpha), 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
            painter.setPen(penMid);
            
            QPainterPath pathMid;
            pathMid.moveTo(0, 0);
            pathMid.quadTo(bladeLength/2, -4, bladeLength, 0);
            painter.drawPath(pathMid);
            
            // 内层（加粗）
            QPen penInner(QColor(150, 150, 150, alpha), 3, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
            painter.setPen(penInner);
            
            QPainterPath pathInner;
            pathInner.moveTo(0, 0);
            pathInner.quadTo(bladeLength/2, -2, bladeLength, 0);
            painter.drawPath(pathInner);
            
            // 恢复状态
            painter.restore();
        }
    }

    // 绘制玩家2攻击特效
    if (!m_p2_attack_effects.isEmpty() && !m_p2_is_dead) {
        for (const AttackEffect& e : m_p2_attack_effects) {
            int px = offsetX + m_p2_col * TILE_SIZE + TILE_SIZE/2 + (int)e.x + m_p2_hit_offset_x;
            int py = offsetY + m_p2_row * TILE_SIZE + TILE_SIZE/2 + (int)e.y + m_p2_hit_offset_y + m_p2_jump_offset;
            int baseBladeLength = m_p2_has_sword ? 50 : 35; // 有武器时弯刀更长
            int bladeLength = (int)(baseBladeLength * e.scale);
            
            // 计算透明度（根据生命周期）
            int alpha = 255 * e.lifetime / 15;
            if (alpha > 255) alpha = 255;
            if (alpha < 0) alpha = 0;
            
            // 保存当前状态
            painter.save();
            
            // 移动到特效位置
            painter.translate(px, py);
            
            // 旋转
            painter.rotate(e.angle);
            
            // 绘制紫色弯刀 - 靠近人物一端加粗
            // 外层淡色（较细）
            QPen penOuter(QColor(100, 30, 150, alpha), 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
            painter.setPen(penOuter);
            painter.setBrush(Qt::NoBrush);
            
            QPainterPath pathOuter;
            pathOuter.moveTo(0, 0);
            pathOuter.quadTo(bladeLength/2, -6, bladeLength, 0);
            painter.drawPath(pathOuter);
            
            // 中层（正常粗细）
            QPen penMid(QColor(130, 40, 175, alpha), 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
            painter.setPen(penMid);
            
            QPainterPath pathMid;
            pathMid.moveTo(0, 0);
            pathMid.quadTo(bladeLength/2, -4, bladeLength, 0);
            painter.drawPath(pathMid);
            
            // 内层（加粗）
            QPen penInner(QColor(150, 50, 200, alpha), 3, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
            painter.setPen(penInner);
            
            QPainterPath pathInner;
            pathInner.moveTo(0, 0);
            pathInner.quadTo(bladeLength/2, -2, bladeLength, 0);
            painter.drawPath(pathInner);
            
            // 恢复状态
            painter.restore();
        }
    }

    // 玩家1进度条（蓝色）
    if (m_p1_collecting) {
        int x = offsetX + m_p1_collect_c * TILE_SIZE;
        int y = offsetY + m_p1_collect_r * TILE_SIZE - 8;
        painter.fillRect(x,y,TILE_SIZE,6,Qt::black);
        int w = (m_p1_collect_prog * TILE_SIZE) / COLLECT_TIME;
        painter.fillRect(x,y,w,6,Qt::cyan);
    }

    // 玩家2进度条（黄色）
    if (m_p2_collecting) {
        int x = offsetX + m_p2_collect_c * TILE_SIZE;
        int y = offsetY + m_p2_collect_r * TILE_SIZE - 8;
        painter.fillRect(x,y,TILE_SIZE,6,Qt::black);
        int w = (m_p2_collect_prog * TILE_SIZE) / COLLECT_TIME;
        painter.fillRect(x,y,w,6,Qt::yellow);
    }

    // 血条
    QFont font("Microsoft YaHei",11);
    painter.setFont(font);
    int y_hp = offsetY + mapHeight + 20; // 血条显示在地图下方

    // 玩家1护盾耐久条（在上）
    if (m_p1_has_shield && m_p1_shield_hp > 0) {
        painter.setBrush(Qt::darkGray);
        painter.drawRect(margin, y_hp - 15, 240, 10);
        painter.setBrush(QColor(100, 180, 255));
        int p1_shield_width = (int)((double)m_p1_shield_hp * 240 / 30);
        painter.drawRect(margin, y_hp - 15, p1_shield_width, 10);
        painter.setPen(Qt::white);
        painter.drawText(margin + 5, y_hp - 7, QString("护盾:%1/30").arg(m_p1_shield_hp));
    }

    // 玩家1血条（在下）
    painter.setBrush(Qt::red);
    painter.drawRect(margin,y_hp,240,20);
    painter.setBrush(Qt::green);
    int p1_hp_width = (int)((double)m_p1_hp * 240 / m_p1_max_hp);
    painter.drawRect(margin,y_hp,p1_hp_width,20);
    painter.setPen(Qt::white);
    painter.drawText(margin,y_hp+15,QString("P1 HP:%1/%2").arg(m_p1_hp).arg(m_p1_max_hp));

    // 玩家2护盾耐久条（在上）
    if (m_p2_has_shield && m_p2_shield_hp > 0) {
        painter.setBrush(Qt::darkGray);
        painter.drawRect(screenWidth - margin - 240, y_hp - 15, 240, 10);
        painter.setBrush(QColor(100, 180, 255));
        int p2_shield_width = (int)((double)m_p2_shield_hp * 240 / 30);
        painter.drawRect(screenWidth - margin - 240, y_hp - 15, p2_shield_width, 10);
        painter.setPen(Qt::white);
        painter.drawText(screenWidth - margin - 235, y_hp - 7, QString("护盾:%1/30").arg(m_p2_shield_hp));
    }

    // 玩家2血条（在下）
    painter.setBrush(Qt::red);
    painter.drawRect(screenWidth - margin - 240,y_hp,240,20);
    painter.setBrush(Qt::green);
    int p2_hp_width = (int)((double)m_p2_hp * 240 / m_p2_max_hp);
    painter.drawRect(screenWidth - margin - 240,y_hp,p2_hp_width,20);
    painter.setPen(Qt::white);
    painter.drawText(screenWidth - margin - 240,y_hp+15,QString("P2 HP:%1/%2").arg(m_p2_hp).arg(m_p2_max_hp));

    // 玩家1背包（左侧显示）
    painter.setPen(Qt::cyan);
    painter.drawText(margin,y_hp+40,QString("P1 木头:%1  石头:%2").arg(m_p1_wood).arg(m_p1_stone));

    // 玩家2背包（右侧显示）
    painter.setPen(Qt::yellow);
    painter.drawText(screenWidth - margin - 200,y_hp+40,QString("P2 木头:%1  石头:%2").arg(m_p2_wood).arg(m_p2_stone));

    // 武器系统UI
    drawWeaponUI(painter, margin, y_hp, screenWidth);

    // 游戏开始倒计时
    if (!m_game_started) {
        qint64 current_time = QDateTime::currentMSecsSinceEpoch();
        int elapsed = (current_time - m_countdown_start_time) / 1000;
        m_countdown = 3 - elapsed;

        if (m_countdown <= 0) {
            m_game_started = true;
        }

        // 绘制倒计时背景
        painter.fillRect(0, 0, width(), height(), QColor(200, 200, 200, 200));

        // 绘制倒计时数字
        painter.setPen(Qt::black);
        painter.setFont(QFont("Microsoft YaHei", 120, QFont::Bold));

        if (m_countdown > 0) {
            painter.drawText(rect(), Qt::AlignCenter, QString::number(m_countdown));
        } else {
            painter.setFont(QFont("Microsoft YaHei", 60, QFont::Bold));
            painter.drawText(rect(), Qt::AlignCenter, "游戏开始！");
        }
        return;
    }

    // 显示游戏计时器
    int minutes = static_cast<int>(m_game_time) / 60;
    int seconds = static_cast<int>(m_game_time) % 60;
    QString time_text = QString("时间: %1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, time_text);


}

// 绘制游戏结束窗口
void Widget::drawGameOver(QPainter& painter)
{
    // 设置背景
    painter.fillRect(0, 0, width(), height(), QColor(50, 50, 50, 180));
    
    // 绘制一个黑色半透明的矩形作为窗口
    QRect gameOverRect(width() / 2 - 300, height() / 2 - 200, 600, 400);
    painter.fillRect(gameOverRect, QColor(0, 0, 0, 200));
    painter.setPen(QColor(255, 255, 255));
    painter.drawRect(gameOverRect);
    
    // 设置字体
    QFont titleFont("Microsoft YaHei", 36, QFont::Bold);
    QFont resultFont("Microsoft YaHei", 20);
    QFont menuFont("Microsoft YaHei", 24);
    
    // 绘制标题
    painter.setFont(titleFont);
    painter.setPen(Qt::red);
    painter.drawText(gameOverRect, Qt::AlignTop | Qt::AlignHCenter, "游戏结束");
    
    // 绘制游戏结果
    painter.setFont(resultFont);
    painter.setPen(Qt::white);
    QString result_text;
    if (m_p1_hp <= 0) {
        result_text = "玩家2胜利！";
    } else if (m_p2_hp <= 0) {
        result_text = "玩家1胜利！";
    } else {
        int p1_total = m_p1_wood + m_p1_stone;
        int p2_total = m_p2_wood + m_p2_stone;
        
        if (p1_total > p2_total) {
            result_text = "玩家1胜利！\n资源更多: " + QString::number(p1_total) + " vs " + QString::number(p2_total);
        } else if (p2_total > p1_total) {
            result_text = "玩家2胜利！\n资源更多: " + QString::number(p2_total) + " vs " + QString::number(p1_total);
        } else {
            result_text = "平局！\n资源相同: " + QString::number(p1_total);
        }
    }
    QRect resultRect(gameOverRect.left(), gameOverRect.top() + 80, gameOverRect.width(), 60);
    painter.drawText(resultRect, Qt::AlignHCenter | Qt::AlignVCenter, result_text);
    
    // 绘制菜单选项
    painter.setFont(menuFont);
    
    QString menuItems[] = {"再来一局", "回到主菜单", "退出游戏"};
    int itemCount = 3;
    int startY = gameOverRect.top() + 150;
    int itemHeight = 40;
    
    for (int i = 0; i < itemCount; i++) {
        int y = startY + i * itemHeight;
        QRect itemRect(gameOverRect.left(), y, gameOverRect.width(), itemHeight);
        
        if (m_menu_selection == i) {
            painter.setPen(Qt::yellow);
            painter.drawText(itemRect, Qt::AlignHCenter | Qt::AlignVCenter, menuItems[i]);
        } else {
            painter.setPen(Qt::white);
            painter.drawText(itemRect, Qt::AlignHCenter | Qt::AlignVCenter, menuItems[i]);
        }
    }
    
    // 绘制操作提示
    painter.setFont(QFont("Microsoft YaHei", 12));
    painter.setPen(Qt::gray);
    painter.drawText(gameOverRect, Qt::AlignBottom | Qt::AlignHCenter, "使用 ↑↓ 键选择，Enter 确认");
}

// 绘制主菜单界面
void Widget::drawMainMenu(QPainter& painter)
{
    // 设置背景
    painter.fillRect(0, 0, width(), height(), QColor(50, 50, 50));
    
    // 设置字体
    QFont titleFont("Microsoft YaHei", 36, QFont::Bold);
    QFont menuFont("Microsoft YaHei", 24);
    
    // 绘制标题
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, "双人发育对战");
    
    // 绘制菜单选项
    painter.setFont(menuFont);
    
    QString menuItems[] = {"开始游戏", "模式选择", "设置", "退出游戏"};
    int itemCount = 4;
    int startY = height() / 2 - 60;
    int itemHeight = 40;
    
    for (int i = 0; i < itemCount; i++) {
        int y = startY + i * itemHeight;
        
        // 绘制选中项
        if (m_menu_selection == i) {
            painter.setPen(Qt::yellow);
            painter.drawText(0, y, width(), itemHeight, Qt::AlignHCenter | Qt::AlignVCenter, menuItems[i]);
        } else {
            painter.setPen(Qt::white);
            painter.drawText(0, y, width(), itemHeight, Qt::AlignHCenter | Qt::AlignVCenter, menuItems[i]);
        }
    }
    
    // 绘制操作提示
    painter.setFont(QFont("Microsoft YaHei", 12));
    painter.setPen(Qt::gray);
    painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, "使用 ↑↓ 键选择，Enter 确认，ESC 返回");
}

// 绘制模式选择界面
void Widget::drawModeSelect(QPainter& painter)
{
    // 设置背景
    painter.fillRect(0, 0, width(), height(), QColor(50, 50, 50));
    
    // 设置字体
    QFont titleFont("Microsoft YaHei", 36, QFont::Bold);
    QFont menuFont("Microsoft YaHei", 24);
    
    // 绘制标题
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, "模式选择");
    
    // 绘制菜单选项
    painter.setFont(menuFont);
    
    QString menuItems[] = {"双人对战", "人机对战", "返回菜单"};
    int itemCount = 3;
    int startY = height() / 2 - 45;
    int itemHeight = 40;
    
    for (int i = 0; i < itemCount; i++) {
        int y = startY + i * itemHeight;
        
        // 绘制选中项
        if (m_menu_selection == i) {
            painter.setPen(Qt::yellow);
            painter.drawText(0, y, width(), itemHeight, Qt::AlignHCenter | Qt::AlignVCenter, menuItems[i]);
        } else {
            painter.setPen(Qt::white);
            painter.drawText(0, y, width(), itemHeight, Qt::AlignHCenter | Qt::AlignVCenter, menuItems[i]);
        }
    }
    
    // 绘制操作提示
    painter.setFont(QFont("Microsoft YaHei", 12));
    painter.setPen(Qt::gray);
    painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, "使用 ↑↓ 键选择，Enter 确认，ESC 返回");
}

// 绘制设置界面
void Widget::drawSettings(QPainter& painter)
{
    // 设置背景
    painter.fillRect(0, 0, width(), height(), QColor(50, 50, 50));
    
    // 设置字体
    QFont titleFont("Microsoft YaHei", 36, QFont::Bold);
    QFont menuFont("Microsoft YaHei", 20);
    
    // 绘制标题
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, "设置");
    
    // 绘制设置选项
    painter.setFont(menuFont);
    
    QString menuItems[] = {"背景音乐音量: " + QString::number(static_cast<int>(m_bgm_volume * 100)),
                          "音效音量: " + QString::number(static_cast<int>(m_sfx_volume * 100)),
                          "返回菜单"};
    int itemCount = 3;
    int startY = height() / 2 - 45;
    int itemHeight = 40;
    
    for (int i = 0; i < itemCount; i++) {
        int y = startY + i * itemHeight;
        
        // 绘制选中项
        if (m_menu_selection == i) {
            painter.setPen(Qt::yellow);
            painter.drawText(0, y, width(), itemHeight, Qt::AlignHCenter | Qt::AlignVCenter, menuItems[i]);
        } else {
            painter.setPen(Qt::white);
            painter.drawText(0, y, width(), itemHeight, Qt::AlignHCenter | Qt::AlignVCenter, menuItems[i]);
        }
    }
    
    // 绘制操作提示
    painter.setFont(QFont("Microsoft YaHei", 12));
    painter.setPen(Qt::gray);
    painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, "使用 ↑↓ 键选择，Enter 确认，ESC 返回，←→ 键调整音量");
}

// 绘制游戏玩法介绍界面
void Widget::drawGameRules(QPainter& painter)
{
    // 设置背景
    painter.fillRect(0, 0, width(), height(), QColor(50, 50, 50));
    
    // 设置字体
    QFont titleFont("Microsoft YaHei", 36, QFont::Bold);
    QFont contentFont("Microsoft YaHei", 16);
    
    // 绘制标题
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, "游戏玩法");
    
    // 绘制内容
    painter.setFont(contentFont);
    painter.setPen(Qt::white);
    
    QString rules[] = {
        "按键说明:",
        "玩家1: W/A/S/D 移动, 空格 攻击, E 采集, Q 合成剑, R 合成盾",
        "玩家2: 方向键 移动, Enter 攻击, Ctrl 采集, M 合成剑, N 合成盾",
        "",
        "玩法介绍:",
        "1. 采集树木和石头获取资源",
        "2. 使用资源合成武器和盾牌",
        "3. 攻击对手，减少其生命值",
        "4. 击杀对手或在时间结束时资源更多者获胜",
        "5. 护盾可以抵挡伤害，耐久度耗尽后消失",
        "6. 河流中移动速度减半",
        "",
        "胜利判定:",
        "- 击杀对手直接获胜",
        "- 时间结束时资源总数多的玩家获胜",
        "- 资源数量相同则平局"
    };
    
    int lineHeight = 25;
    int startY = height() / 2 - 150;
    
    for (int i = 0; i < 17; i++) {
        int y = startY + i * lineHeight;
        painter.drawText(rect(), Qt::AlignHCenter | Qt::AlignVCenter, rules[i]);
    }
    
    // 绘制返回提示
    painter.setFont(QFont("Microsoft YaHei", 12));
    painter.setPen(Qt::gray);
    painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, "按 ESC 返回主菜单");
}

// 处理菜单选择
void Widget::handleMenuSelection()
{
    switch (m_current_state) {
    case STATE_MAIN_MENU:
        switch (m_menu_selection) {
        case 0: // 开始游戏
            m_current_state = STATE_GAME_PLAY;
            // 重置游戏状态
            m_game_started = false;
            m_game_over = false;
            m_p1_hp = MAX_HP;
            m_p1_max_hp = MAX_HP;
            m_p2_hp = MAX_HP;
            m_p2_max_hp = MAX_HP;
            m_p1_wood = 0;
            m_p1_stone = 0;
            m_p2_wood = 0;
            m_p2_stone = 0;
            m_p1_has_sword = false;
            m_p1_has_shield = false;
            m_p2_has_sword = false;
            m_p2_has_shield = false;
            m_p1_shield_hp = 0;
            m_p2_shield_hp = 0;
            m_countdown = 3;
            m_countdown_start_time = QDateTime::currentMSecsSinceEpoch();
            update();
            break;
            
        case 1: // 模式选择
            m_current_state = STATE_MODE_SELECT;
            m_menu_selection = 0;
            update();
            break;
            
        case 2: // 设置
            m_current_state = STATE_SETTINGS;
            m_menu_selection = 0;
            update();
            break;
            
        case 3: // 退出游戏
            close();
            break;
        }
        break;
        
    case STATE_MODE_SELECT:
        switch (m_menu_selection) {
        case 0: // 双人对战
            // 开始双人对战
            m_current_state = STATE_GAME_PLAY;
            // 重置游戏状态
            m_game_started = false;
            m_game_over = false;
            m_p1_hp = MAX_HP;
            m_p1_max_hp = MAX_HP;
            m_p2_hp = MAX_HP;
            m_p2_max_hp = MAX_HP;
            m_p1_wood = 0;
            m_p1_stone = 0;
            m_p2_wood = 0;
            m_p2_stone = 0;
            m_p1_has_sword = false;
            m_p1_has_shield = false;
            m_p2_has_sword = false;
            m_p2_has_shield = false;
            m_p1_shield_hp = 0;
            m_p2_shield_hp = 0;
            m_countdown = 3;
            m_countdown_start_time = QDateTime::currentMSecsSinceEpoch();
            update();
            break;
            
        case 1: // 人机对战
            // 暂未实现，返回主菜单
            m_current_state = STATE_MAIN_MENU;
            m_menu_selection = 0;
            update();
            break;
            
        case 2: // 返回菜单
            m_current_state = STATE_MAIN_MENU;
            m_menu_selection = 0;
            update();
            break;
        }
        break;
        
    case STATE_SETTINGS:
        switch (m_menu_selection) {
        case 0: // 背景音乐音量
            // 已在上下键处理
            break;
            
        case 1: // 音效音量
            // 已在上下键处理
            break;
            
        case 2: // 返回菜单
            m_current_state = STATE_MAIN_MENU;
            m_menu_selection = 0;
            update();
            break;
        }
        break;
        
    case STATE_GAME_OVER:
        switch (m_menu_selection) {
        case 0: // 再来一局
            m_current_state = STATE_GAME_PLAY;
            // 重置游戏状态
            m_game_started = false;
            m_game_over = false;
            m_p1_hp = MAX_HP;
            m_p1_max_hp = MAX_HP;
            m_p2_hp = MAX_HP;
            m_p2_max_hp = MAX_HP;
            m_p1_wood = 0;
            m_p1_stone = 0;
            m_p2_wood = 0;
            m_p2_stone = 0;
            m_p1_has_sword = false;
            m_p1_has_shield = false;
            m_p2_has_sword = false;
            m_p2_has_shield = false;
            m_p1_shield_hp = 0;
            m_p2_shield_hp = 0;
            m_countdown = 3;
            m_countdown_start_time = QDateTime::currentMSecsSinceEpoch();
            m_game_time = GAME_TIME;
            m_start_time = QDateTime::currentMSecsSinceEpoch();
            m_hearts.clear();
            m_p1_heart_effects.clear();
            m_p2_heart_effects.clear();
            m_p1_attack_effects.clear();
            m_p2_attack_effects.clear();
            m_p1_is_dead = false;
            m_p1_death_finished = false;
            m_p1_death_opacity = 1.0f;
            m_p1_death_timer = 0;
            m_p2_is_dead = false;
            m_p2_death_finished = false;
            m_p2_death_opacity = 1.0f;
            m_p2_death_timer = 0;
            // 重启定时器
            m_timer->start(30);
            m_move_timer->start(240);
#ifdef Qt6Multimedia_FOUND
            // 重新开始背景音乐
            m_background_music->play();
#endif
            update();
            break;
            
        case 1: // 回到主菜单
            m_current_state = STATE_MAIN_MENU;
            m_menu_selection = 0;
            update();
            break;
            
        case 2: // 退出游戏
            close();
            break;
        }
        break;
    }
}

// 处理菜单输入
void Widget::handleMenuInput(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        // 上移选择
        m_menu_selection--;
        if (m_menu_selection < 0) {
            // 根据当前状态确定最大选项数
            if (m_current_state == STATE_MAIN_MENU) {
                m_menu_selection = 3; // 4个选项
            } else if (m_current_state == STATE_MODE_SELECT || m_current_state == STATE_SETTINGS || m_current_state == STATE_GAME_OVER) {
                m_menu_selection = 2; // 3个选项
            }
        }
        update();
        break;
        
    case Qt::Key_Down:
        // 下移选择
        m_menu_selection++;
        if (m_current_state == STATE_MAIN_MENU) {
            if (m_menu_selection > 3) m_menu_selection = 0;
        } else if (m_current_state == STATE_MODE_SELECT || m_current_state == STATE_SETTINGS || m_current_state == STATE_GAME_OVER) {
            if (m_menu_selection > 2) m_menu_selection = 0;
        }
        update();
        break;
        
    case Qt::Key_Left:
        // 左移调整音量
        if (m_current_state == STATE_SETTINGS) {
            if (m_menu_selection == 0) {
                m_bgm_volume = qMax(0.0f, m_bgm_volume - 0.1f);
            } else if (m_menu_selection == 1) {
                m_sfx_volume = qMax(0.0f, m_sfx_volume - 0.1f);
            }
            update();
        }
        break;
        
    case Qt::Key_Right:
        // 右移调整音量
        if (m_current_state == STATE_SETTINGS) {
            if (m_menu_selection == 0) {
                m_bgm_volume = qMin(1.0f, m_bgm_volume + 0.1f);
            } else if (m_menu_selection == 1) {
                m_sfx_volume = qMin(1.0f, m_sfx_volume + 0.1f);
            }
            update();
        }
        break;
        
    case Qt::Key_Return:
    case Qt::Key_Enter:
        // 确认选择
        handleMenuSelection();
        break;
        
    case Qt::Key_Escape:
        // 返回上一级
        if (m_current_state == STATE_MAIN_MENU) {
            // 退出游戏
            close();
        } else if (m_current_state == STATE_GAME_RULES) {
            // 返回主菜单
            m_current_state = STATE_MAIN_MENU;
            m_menu_selection = 0;
            update();
        } else {
            // 返回主菜单
            m_current_state = STATE_MAIN_MENU;
            m_menu_selection = 0;
            update();
        }
        break;
    }
}