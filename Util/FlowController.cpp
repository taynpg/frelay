// FlowController.cpp
#include "FlowController.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <algorithm>
#include <numeric>

// 默认配置
static const FlowController::Config DEFAULT_CONFIG = {.targetBufferBytes = 8192.0,
                                                      .minDelayMs = 0.5,
                                                      .maxDelayMs = 500.0,
                                                      .kp = 0.8,
                                                      .ki = 0.2,
                                                      .kd = 0.1,
                                                      .alpha = 0.3,
                                                      .errorDeadZone = 1024.0,
                                                      .maxHistorySize = 20,
                                                      .enableAdaptiveGain = true};

FlowController::FlowController() : m_config(DEFAULT_CONFIG), m_baseKp(m_config.kp), m_baseKi(m_config.ki), m_baseKd(m_config.kd)
{
    reset();
}

double FlowController::calculateDelay(double bytesToWrite, qint64 timestampMs)
{
    QMutexLocker locker(&m_mutex);

    // 获取当前时间
    qint64 currentTime = timestampMs;
    if (currentTime == 0) {
        currentTime = QDateTime::currentMSecsSinceEpoch();
    }

    // 计算时间间隔(秒)
    double dt = 0.0;
    if (m_state.lastTimestamp > 0) {
        dt = (currentTime - m_state.lastTimestamp) / 1000.0;
    }

    // 平滑处理输入
    m_state.lastBytes = exponentialMovingAverage(bytesToWrite, m_state.lastBytes, m_config.alpha);

    // 计算误差
    double error = m_state.lastBytes - m_config.targetBufferBytes;

    // 死区处理
    if (std::abs(error) < m_config.errorDeadZone) {
        error = 0.0;
    }

    // 平滑误差(用于统计)
    double smoothedError = exponentialMovingAverage(error, m_stats.smoothedError, m_config.alpha * 0.5);

    // 自适应增益调整
    if (m_config.enableAdaptiveGain && dt > 0) {
        adjustGains(error, dt);
    }

    // 计算PID控制输出(延迟调整量)
    double delayAdjustment = calculatePID(error, dt);

    // 计算新的延迟
    double newDelay = m_state.lastDelay + delayAdjustment;

    // 限制延迟范围
    newDelay = std::max(m_config.minDelayMs, std::min(newDelay, m_config.maxDelayMs));

    // 平滑延迟输出
    double smoothedDelay = exponentialMovingAverage(newDelay, m_state.lastDelay, m_config.alpha * 0.8);

    // 更新状态
    m_state.lastDelay = smoothedDelay;
    m_state.lastError = error;
    m_state.lastTimestamp = currentTime;

    // 更新统计信息
    updateStatistics(smoothedDelay, smoothedError);

    return smoothedDelay;
}

double FlowController::calculatePID(double error, double dt)
{
    // 比例项
    double p = m_config.kp * error;

    // 积分项(带限幅和抗饱和)
    if (dt > 0) {
        m_state.integral += error * dt;

        // 积分限幅
        double maxIntegral = 1000.0;   // 最大积分值
        m_state.integral = std::max(-maxIntegral, std::min(m_state.integral, maxIntegral));

        // 当输出饱和时停止积分
        if ((m_state.lastDelay <= m_config.minDelayMs && error < 0) || (m_state.lastDelay >= m_config.maxDelayMs && error > 0)) {
            m_state.integral -= error * dt;   // 回溯积分
        }
    }

    double i = m_config.ki * m_state.integral;

    // 微分项(计算误差变化率)
    double d = 0.0;
    if (dt > 0) {
        double errorDerivative = (error - m_state.lastError) / dt;

        // 对微分项进行低通滤波
        static double lastDerivative = 0.0;
        double filteredDerivative = exponentialMovingAverage(errorDerivative, lastDerivative, 0.7);
        lastDerivative = filteredDerivative;

        d = m_config.kd * filteredDerivative;
    }

    // 非线性增益调整：大误差时增强响应
    double nonlinearFactor = 1.0;
    double errorAbs = std::abs(error);
    double maxError = m_config.targetBufferBytes * 3.0;

    if (errorAbs > m_config.targetBufferBytes * 2.0) {
        nonlinearFactor = 2.5;   // 大误差，增强响应
    } else if (errorAbs > m_config.targetBufferBytes) {
        nonlinearFactor = 1.5;   // 中误差，中等响应
    } else if (errorAbs < m_config.targetBufferBytes * 0.2) {
        nonlinearFactor = 0.5;   // 小误差，减弱响应避免震荡
    }

    return (p + i + d) * nonlinearFactor;
}

void FlowController::adjustGains(double error, double dt)
{
    // 记录误差历史
    m_errorHistory.append(error);
    if (m_errorHistory.size() > m_config.maxHistorySize) {
        m_errorHistory.removeFirst();
    }

    if (m_errorHistory.size() >= 5) {
        // 计算误差方差
        double sum = 0.0, sumSq = 0.0;
        for (double e : m_errorHistory) {
            sum += e;
            sumSq += e * e;
        }
        double mean = sum / m_errorHistory.size();
        double variance = (sumSq / m_errorHistory.size()) - (mean * mean);

        // 平滑方差
        m_errorVariance = exponentialMovingAverage(variance, m_errorVariance, 0.1);

        // 根据误差方差调整增益
        double targetVariance = m_config.errorDeadZone * 0.5;

        if (m_errorVariance < targetVariance) {
            // 稳定状态：减少增益避免震荡
            m_stableCount++;
            m_unstableCount = std::max(0, m_unstableCount - 1);

            if (m_stableCount > 10) {
                double reduction = 0.95;   // 每次减少5%
                m_config.kp = m_baseKp * std::pow(reduction, m_stableCount - 10);
                m_config.ki = m_baseKi * std::pow(reduction, m_stableCount - 10);
                m_config.kp = std::max(m_baseKp * 0.3, m_config.kp);
                m_config.ki = std::max(m_baseKi * 0.3, m_config.ki);
            }
        } else if (m_errorVariance > targetVariance * 4.0) {
            // 不稳定状态：增加增益快速响应
            m_unstableCount++;
            m_stableCount = 0;

            if (m_unstableCount > 5) {
                double boost = 1.2;   // 每次增加20%
                m_config.kp = m_baseKp * std::pow(boost, std::min(m_unstableCount - 5, 5));
                m_config.ki = m_baseKi * std::pow(boost, std::min(m_unstableCount - 5, 5));
                m_config.kp = std::min(m_baseKp * 3.0, m_config.kp);
                m_config.ki = std::min(m_baseKi * 3.0, m_config.ki);
            }
        } else {
            // 正常状态：使用基础增益
            m_config.kp = m_baseKp;
            m_config.ki = m_baseKi;
            m_stableCount = std::max(0, m_stableCount - 1);
            m_unstableCount = std::max(0, m_unstableCount - 1);
        }
    }
}

double FlowController::exponentialMovingAverage(double newValue, double oldValue, double alpha) const
{
    if (alpha <= 0.0)
        return newValue;
    if (alpha >= 1.0)
        return oldValue;
    return alpha * oldValue + (1.0 - alpha) * newValue;
}

void FlowController::updateStatistics(double delay, double error)
{
    m_stats.currentDelay = delay;
    m_stats.currentError = error;
    m_stats.smoothedError = exponentialMovingAverage(error, m_stats.smoothedError, 0.1);
    m_stats.updateCount++;

    // 记录延迟历史
    m_delayHistory.append(delay);
    if (m_delayHistory.size() > m_config.maxHistorySize) {
        m_delayHistory.removeFirst();
    }

    // 计算统计值
    if (!m_delayHistory.isEmpty()) {
        double sum = 0.0;
        double minVal = m_delayHistory.first();
        double maxVal = m_delayHistory.first();

        for (double val : m_delayHistory) {
            sum += val;
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
        }

        m_stats.avgDelay = sum / m_delayHistory.size();
        m_stats.minDelay = minVal;
        m_stats.maxDelay = maxVal;

        // 计算稳定性(基于延迟的变异系数)
        if (m_stats.avgDelay > 0.0) {
            double sumSqDiff = 0.0;
            for (double val : m_delayHistory) {
                double diff = val - m_stats.avgDelay;
                sumSqDiff += diff * diff;
            }
            double stdDev = std::sqrt(sumSqDiff / m_delayHistory.size());
            double cv = stdDev / m_stats.avgDelay;   // 变异系数

            // 稳定性指标(0-1, 1表示最稳定)
            m_stats.stability = std::max(0.0, 1.0 - cv);
        }
    }
}

void FlowController::setConfig(const Config& config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;
    m_baseKp = config.kp;
    m_baseKi = config.ki;
    m_baseKd = config.kd;
}

FlowController::Config FlowController::getConfig() const
{
    QMutexLocker locker(&m_mutex);
    return m_config;
}

FlowController::Statistics FlowController::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats;
}

FlowController::State FlowController::getState() const
{
    QMutexLocker locker(&m_mutex);
    return m_state;
}

void FlowController::reset()
{
    QMutexLocker locker(&m_mutex);

    m_state = State();
    m_delayHistory.clear();
    m_errorHistory.clear();
    m_stats = Statistics();
    m_errorVariance = 0.0;
    m_stableCount = 0;
    m_unstableCount = 0;
}