// FlowController.h
#ifndef FLOWCONTROLLER_H
#define FLOWCONTROLLER_H

#include <QMutex>
#include <QObject>
#include <QVector>
#include <cmath>

class FlowController
{
public:
    // 配置参数
    struct Config {
        double targetBufferBytes = 8192.0;   // 目标缓冲区大小(字节)
        double minDelayMs = 0.5;             // 最小延迟(ms)
        double maxDelayMs = 500.0;           // 最大延迟(ms)
        double kp = 0.8;                     // 比例系数
        double ki = 0.2;                     // 积分系数
        double kd = 0.1;                     // 微分系数
        double alpha = 0.3;                  // 平滑系数(0-1)
        double errorDeadZone = 1024.0;       // 误差死区(字节)
        int maxHistorySize = 20;             // 历史数据最大长度
        bool enableAdaptiveGain = true;      // 启用自适应增益
    };

    // 统计信息
    struct Statistics {
        double currentDelay = 0.0;    // 当前延迟
        double avgDelay = 0.0;        // 平均延迟
        double minDelay = 0.0;        // 最小延迟
        double maxDelay = 0.0;        // 最大延迟
        double currentError = 0.0;    // 当前误差
        double smoothedError = 0.0;   // 平滑后的误差
        qint64 updateCount = 0;       // 更新次数
        double stability = 1.0;       // 稳定性指标(0-1)
    };

    FlowController();

    // 设置配置参数
    void setConfig(const Config& config);
    Config getConfig() const;

    // 核心计算函数 - 根据当前缓冲区大小计算延迟
    double calculateDelay(double bytesToWrite, qint64 timestampMs = 0);

    // 重置控制器状态
    void reset();

    // 获取统计信息
    Statistics getStatistics() const;

    // 获取当前控制器状态
    struct State {
        double lastDelay = 0.0;     // 上一次计算的延迟
        double lastError = 0.0;     // 上一次的误差
        double integral = 0.0;      // 积分项累计
        double lastBytes = 0.0;     // 上一次的缓冲区大小
        qint64 lastTimestamp = 0;   // 上一次调用的时间戳
    };

    State getState() const;

private:
    // 指数加权移动平均(EMA)
    double exponentialMovingAverage(double newValue, double oldValue, double alpha) const;

    // 计算PID控制输出
    double calculatePID(double error, double dt);

    // 自适应调整增益系数
    void adjustGains(double error, double dt);

    // 更新统计信息
    void updateStatistics(double delay, double error);

    mutable QMutex m_mutex;
    Config m_config;
    State m_state;

    // 历史记录
    QVector<double> m_delayHistory;
    QVector<double> m_errorHistory;
    Statistics m_stats;

    // 自适应增益相关
    double m_baseKp, m_baseKi, m_baseKd;
    double m_errorVariance = 0.0;
    int m_stableCount = 0;
    int m_unstableCount = 0;
};
#endif   // FLOWCONTROLLER_H